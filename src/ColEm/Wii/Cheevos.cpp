/*
 * RetroAchievements support for webrcade ColecoVision (ColEm) emulator.
 * Uses the rcheevos library with the WRC (WebRcade) HTTP proxy pattern.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

#include "../../ColEm/Coleco.h"

#include "../../../rcheevos/include/rc_runtime.h"
#include "../../../rcheevos/include/rc_runtime_types.h"
#include "../../../rcheevos/include/rc_hash.h"
#include "../../../rcheevos/include/rc_api_user.h"
#include "../../../rcheevos/include/rc_api_runtime.h"
#include "../../../rcheevos/include/rc_consoles.h"
#include "../../../rcheevos/include/rc_error.h"

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static rc_runtime_t s_runtime;
static int  s_initialized = 0;
static int  s_game_loaded = 0;
static char s_username[64] = {0};
static char s_token[64]    = {0};
static char s_game_hash[33] = {0};
static char s_pending_hash[33] = {0};
static unsigned s_game_id = 0;
static int  s_ping_countdown = 0;

#define PING_INTERVAL_FRAMES (120 * 60)  /* 2 minutes at 60fps */

/* ------------------------------------------------------------------ */
/* WRC HTTP proxy                                                       */
/* ------------------------------------------------------------------ */

#define WRC_MAX_PENDING 64

/* Parallel arrays — no heap struct, handler pointer lives in static memory */
static char  *s_pending_url[WRC_MAX_PENDING];
static char  *s_pending_post[WRC_MAX_PENDING];
static void  *s_pending_data[WRC_MAX_PENDING];
static void (*s_pending_handler[WRC_MAX_PENDING])(int, const char *, int, void *);
static int    s_req_seq = 0;

static void wrc_dispatch(rc_api_request_t *req,
                         void (*handler)(int, const char *, int, void *),
                         void *ud)
{
    int req_id = s_req_seq++;
    int slot   = req_id % WRC_MAX_PENDING;

    /* Free any stale entry in this slot */
    free(s_pending_url[slot]);
    free(s_pending_post[slot]);

    s_pending_url[slot]     = req->url       ? strdup(req->url)       : NULL;
    s_pending_post[slot]    = req->post_data ? strdup(req->post_data) : NULL;
    s_pending_data[slot]    = ud;
    s_pending_handler[slot] = handler;

    rc_api_destroy_request(req);

    EM_ASM({
        var req_id   = $0;
        var url      = UTF8ToString($1);
        var postData = $2 ? UTF8ToString($2) : null;
        if (window.emulator && window.emulator.raHttpRequest)
            window.emulator.raHttpRequest(req_id, url, postData);
    }, req_id, s_pending_url[slot], s_pending_post[slot]);
}

/* Called from JS when the HTTP response arrives */
extern "C" EMSCRIPTEN_KEEPALIVE
void rcheevos_wrc_response(int req_id, int status,
                           const char *data, int data_len)
{
    int slot = req_id % WRC_MAX_PENDING;
    void (*handler)(int, const char *, int, void *) = s_pending_handler[slot];
    void *ud = s_pending_data[slot];

    s_pending_handler[slot] = NULL;
    s_pending_data[slot] = NULL;

    /* IMPORTANT: free url/post AFTER the handler returns.
     * Freeing them before the handler risks dlmalloc recycling those addresses
     * for allocations made by the handler (including the response body buffer
     * from JS), causing the handler to read from freed memory. */
    if (handler) {
        handler(status, data, data_len, ud);
    }
    /* NULL handler is normal for fire-and-forget requests (e.g. ping) */

    free(s_pending_url[slot]);  s_pending_url[slot]  = NULL;
    free(s_pending_post[slot]); s_pending_post[slot] = NULL;
}

/* ------------------------------------------------------------------ */
/* Memory reader (peek callback for rc_runtime_do_frame)              */
/* ------------------------------------------------------------------ */

/* ColecoVision rcheevos address space:
 *   RA 0x0000..0x03FF  -> real Z80 0x6000..0x63FF (1 KB system RAM)
 * In ColEm the 8 × 1 KB banks live at RAM+0x1A000 (RAM_OS7).
 * Bank 0 = 0x6000..0x63FF, so RA address maps directly to RAM_OS7[address].
 */
extern byte *RAM;   /* declared in Coleco.c */

static unsigned coleco_peek(unsigned address, unsigned num_bytes, void * /*ud*/)
{
    if (!RAM) return 0;
    /* Only system RAM is exposed (0x000..0x3FF) */
    const byte *base = RAM + 0x1A000;
    unsigned result = 0;
    for (unsigned i = 0; i < num_bytes && i < 4; i++) {
        unsigned addr = (address + i) & 0x3FF;
        result |= ((unsigned)base[addr]) << (8 * i);
    }
    return result;
}

/* ------------------------------------------------------------------ */
/* Forward declarations                                                 */
/* ------------------------------------------------------------------ */

static void on_resolve_hash_response(int status, const char *body, int body_len, void *ud);
static void award_achievement(unsigned id);

/* ------------------------------------------------------------------ */
/* Event handler (achievement triggered, etc.)                         */
/* ------------------------------------------------------------------ */

static void on_runtime_event(const rc_runtime_event_t *evt)
{
    if (evt->type == RC_RUNTIME_EVENT_ACHIEVEMENT_TRIGGERED) {
        printf("[RA] Achievement triggered: %u\n", evt->id);
        award_achievement(evt->id);
    }
}

/* ------------------------------------------------------------------ */
/* Login flow                                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    char username[64];
    char token[64];
} login_cb_data_t;

static void on_login_response(int status, const char *body, int body_len, void *ud)
{
    (void)ud;
    if (status != 200 || !body) {
        printf("[RA] Login HTTP error %d\n", status);
        return;
    }
    rc_api_login_response_t resp;
    memset(&resp, 0, sizeof(resp));
    int ret = rc_api_process_login_response(&resp, body);
    if (ret != RC_OK || !resp.response.succeeded) {
        printf("[RA] Login failed: %s\n", resp.response.error_message ? resp.response.error_message : "unknown");
        rc_api_destroy_login_response(&resp);
        return;
    }
    strncpy(s_username, resp.username, sizeof(s_username) - 1);
    strncpy(s_token, resp.api_token, sizeof(s_token) - 1);
    printf("[RA] Logged in as %s\n", s_username);
    rc_api_destroy_login_response(&resp);

    /* If a game hash was queued before login completed, start loading now */
    if (s_pending_hash[0]) {
        char hash[33];
        strncpy(hash, s_pending_hash, 32);
        hash[32] = '\0';
        s_pending_hash[0] = '\0';
        strncpy(s_game_hash, hash, 32);
        s_game_hash[32] = '\0';
        s_game_loaded = 0;
        s_game_id = 0;
        rc_runtime_reset(&s_runtime);
        printf("[RA] Loading game hash: %s\n", s_game_hash);
        rc_api_resolve_hash_request_t params;
        memset(&params, 0, sizeof(params));
        params.game_hash = s_game_hash;
        rc_api_request_t req;
        memset(&req, 0, sizeof(req));
        if (rc_api_init_resolve_hash_request(&req, &params) != RC_OK) return;
        wrc_dispatch(&req, on_resolve_hash_response, NULL);
    }
}

extern "C" EMSCRIPTEN_KEEPALIVE
void EmCheevosLogin(const char *username, const char *token)
{
    if (!s_initialized) return;

    /* Store credentials only after successful login response.
     * Use temporaries here so EmCheevosLoadGame doesn't see them as "logged in" yet. */
    char tmp_user[64] = {0};
    char tmp_token[64] = {0};
    strncpy(tmp_user,  username ? username : "", sizeof(tmp_user)  - 1);
    strncpy(tmp_token, token    ? token    : "", sizeof(tmp_token) - 1);

    /* Send a login-with-token request just to validate and get the canonical username */
    rc_api_login_request_t params;
    memset(&params, 0, sizeof(params));
    params.username  = tmp_user;
    params.api_token = tmp_token;

    rc_api_request_t req;
    memset(&req, 0, sizeof(req));
    if (rc_api_init_login_request(&req, &params) != RC_OK) return;

    wrc_dispatch(&req, on_login_response, NULL);
}

/* ------------------------------------------------------------------ */
/* Resolve hash → game ID                                              */
/* ------------------------------------------------------------------ */

static void start_session(void);
static void fetch_game_data(void);
static void fetch_unlocks(void);

static void on_resolve_hash_response(int status, const char *body, int body_len, void *ud)
{
    (void)ud;
    if (status != 200 || !body) return;

    rc_api_resolve_hash_response_t resp;
    memset(&resp, 0, sizeof(resp));
    int parse_result = rc_api_process_resolve_hash_response(&resp, body);
    if (parse_result != RC_OK || !resp.response.succeeded) {
        printf("[RA] Hash not found on server\n");
        rc_api_destroy_resolve_hash_response(&resp);
        return;
    }
    s_game_id = resp.game_id;
    printf("[RA] Game ID: %u\n", s_game_id);
    rc_api_destroy_resolve_hash_response(&resp);

    if (s_game_id == 0) return;

    start_session();
    fetch_game_data();
}

/* ------------------------------------------------------------------ */
/* Start session                                                        */
/* ------------------------------------------------------------------ */

static void on_start_session_response(int status, const char *body, int body_len, void *ud)
{
    (void)ud;
    (void)status;
    (void)body;
    (void)body_len;
    /* Nothing to do; fire-and-forget */
}

static void start_session(void)
{
    rc_api_start_session_request_t params;
    memset(&params, 0, sizeof(params));
    params.username  = s_username;
    params.api_token = s_token;
    params.game_id   = s_game_id;

    rc_api_request_t req;
    memset(&req, 0, sizeof(req));
    if (rc_api_init_start_session_request(&req, &params) != RC_OK) return;
    wrc_dispatch(&req, on_start_session_response, NULL);
}

/* ------------------------------------------------------------------ */
/* Fetch game data (achievement definitions)                           */
/* ------------------------------------------------------------------ */

static void on_fetch_game_data_response(int status, const char *body, int body_len, void *ud)
{
    (void)ud;
    if (status != 200 || !body) return;

    rc_api_fetch_game_data_response_t resp;
    memset(&resp, 0, sizeof(resp));
    if (rc_api_process_fetch_game_data_response(&resp, body) != RC_OK || !resp.response.succeeded) {
        printf("[RA] fetch_game_data failed: %s\n",
               resp.response.error_message ? resp.response.error_message : "?");
        rc_api_destroy_fetch_game_data_response(&resp);
        return;
    }

    printf("[RA] Game: %s, %u achievements\n", resp.title, resp.num_achievements);

    /* Activate core achievements */
    for (unsigned i = 0; i < resp.num_achievements; i++) {
        const rc_api_achievement_definition_t *a = &resp.achievements[i];
        if (a->category != RC_ACHIEVEMENT_CATEGORY_CORE) continue;
        int ret = rc_runtime_activate_achievement(&s_runtime, a->id, a->definition, NULL, 0);
        if (ret != RC_OK) {
            printf("[RA] Failed to activate achievement %u: %s\n", a->id, rc_error_str(ret));
        }
    }

    /* Activate rich presence */
    if (resp.rich_presence_script && resp.rich_presence_script[0]) {
        rc_runtime_activate_richpresence(&s_runtime, resp.rich_presence_script, NULL, 0);
    }
    s_game_loaded = 1;
    s_ping_countdown = PING_INTERVAL_FRAMES;

    rc_api_destroy_fetch_game_data_response(&resp);

    rc_runtime_reset(&s_runtime);

    /* Fetch unlocks only after achievements are activated so deactivation works correctly */
    fetch_unlocks();
}

static void fetch_game_data(void)
{
    rc_api_fetch_game_data_request_t params;
    memset(&params, 0, sizeof(params));
    params.username  = s_username;
    params.api_token = s_token;
    params.game_id   = s_game_id;

    rc_api_request_t req;
    memset(&req, 0, sizeof(req));
    if (rc_api_init_fetch_game_data_request(&req, &params) != RC_OK) return;
    wrc_dispatch(&req, on_fetch_game_data_response, NULL);
}

/* ------------------------------------------------------------------ */
/* Fetch user unlocks                                                   */
/* ------------------------------------------------------------------ */

static void on_fetch_unlocks_response(int status, const char *body, int body_len, void *ud)
{
    (void)ud;
    if (status != 200 || !body) return;

    rc_api_fetch_user_unlocks_response_t resp;
    memset(&resp, 0, sizeof(resp));
    if (rc_api_process_fetch_user_unlocks_response(&resp, body) != RC_OK) {
        rc_api_destroy_fetch_user_unlocks_response(&resp);
        return;
    }

    /* Deactivate already-earned achievements so they don't fire again */
    for (unsigned i = 0; i < resp.num_achievement_ids; i++) {
        printf("[RA] already unlocked: achievement %u\n", resp.achievement_ids[i]);
        rc_runtime_deactivate_achievement(&s_runtime, resp.achievement_ids[i]);
    }
    printf("[RA] %u achievements already unlocked\n", resp.num_achievement_ids);

    rc_api_destroy_fetch_user_unlocks_response(&resp);
}

static void fetch_unlocks(void)
{
    rc_api_fetch_user_unlocks_request_t params;
    memset(&params, 0, sizeof(params));
    params.username  = s_username;
    params.api_token = s_token;
    params.game_id   = s_game_id;
    params.hardcore  = 0;

    rc_api_request_t req;
    memset(&req, 0, sizeof(req));
    if (rc_api_init_fetch_user_unlocks_request(&req, &params) != RC_OK) return;
    wrc_dispatch(&req, on_fetch_unlocks_response, NULL);
}

/* ------------------------------------------------------------------ */
/* Award achievement                                                    */
/* ------------------------------------------------------------------ */

static void on_award_achievement_response(int status, const char *body, int body_len, void *ud)
{
    (void)ud;
    (void)status;
    (void)body;
    (void)body_len;
    /* JS side handles the toast; nothing extra needed here */
}

static void award_achievement(unsigned id)
{
    rc_api_award_achievement_request_t params;
    memset(&params, 0, sizeof(params));
    params.username       = s_username;
    params.api_token      = s_token;
    params.achievement_id = id;
    params.hardcore       = 0;
    params.game_hash      = s_game_hash;

    rc_api_request_t req;
    memset(&req, 0, sizeof(req));
    if (rc_api_init_award_achievement_request(&req, &params) != RC_OK) return;
    wrc_dispatch(&req, on_award_achievement_response, NULL);
}

/* ------------------------------------------------------------------ */
/* Exported C functions called from JS                                 */
/* ------------------------------------------------------------------ */

extern "C" EMSCRIPTEN_KEEPALIVE
void EmCheevosHashRom(const char *path, char *hash_out)
{
    /* hash_out must be at least 33 bytes */
    hash_out[0] = '\0';
    rc_hash_generate_from_file(hash_out, RC_CONSOLE_COLECOVISION, path);
}

extern "C" EMSCRIPTEN_KEEPALIVE
void EmCheevosInit(void)
{
    if (s_initialized) {
        rc_runtime_destroy(&s_runtime);
    }
    memset(s_pending_url,     0, sizeof(s_pending_url));
    memset(s_pending_post,    0, sizeof(s_pending_post));
    memset(s_pending_data,    0, sizeof(s_pending_data));
    memset(s_pending_handler, 0, sizeof(s_pending_handler));
    rc_runtime_init(&s_runtime);
    s_initialized = 1;
    s_game_loaded = 0;
    s_game_id = 0;
    s_pending_hash[0] = '\0';
    printf("[RA] Cheevos initialized\n");
}

extern "C" EMSCRIPTEN_KEEPALIVE
void EmCheevosLoadGame(const char *hash)
{
    if (!s_initialized || !hash || !hash[0]) return;

    /* If login hasn't completed yet, queue the hash and wait */
    if (!s_username[0] || !s_token[0]) {
        strncpy(s_pending_hash, hash, 32);
        s_pending_hash[32] = '\0';
        printf("[RA] Queued game hash (waiting for login): %s\n", s_pending_hash);
        return;
    }

    strncpy(s_game_hash, hash, 32);
    s_game_hash[32] = '\0';
    s_game_loaded = 0;
    s_game_id = 0;

    rc_runtime_reset(&s_runtime);

    printf("[RA] Loading game hash: %s\n", s_game_hash);

    rc_api_resolve_hash_request_t params;
    memset(&params, 0, sizeof(params));
    params.game_hash = s_game_hash;

    rc_api_request_t req;
    memset(&req, 0, sizeof(req));
    if (rc_api_init_resolve_hash_request(&req, &params) != RC_OK) return;
    wrc_dispatch(&req, on_resolve_hash_response, NULL);
}

extern "C" EMSCRIPTEN_KEEPALIVE
unsigned EmCheevosGetRPWrapper(void)
{
    return (unsigned)(uintptr_t)s_runtime.richpresence;
}

extern "C" EMSCRIPTEN_KEEPALIVE
void EmCheevosFrame(void)
{
    if (!s_initialized || !s_game_loaded) return;
    if (!RAM) return;

    /* Collect events from this frame */
    rc_runtime_do_frame(&s_runtime, on_runtime_event, coleco_peek, NULL, NULL);

    /* Send rich presence ping every PING_INTERVAL_FRAMES frames */
    if (--s_ping_countdown <= 0) {
        s_ping_countdown = PING_INTERVAL_FRAMES;

        char rp_buf[256] = {0};
        rc_runtime_get_richpresence(&s_runtime, rp_buf, sizeof(rp_buf), coleco_peek, NULL, NULL);

        rc_api_ping_request_t ping_params;
        memset(&ping_params, 0, sizeof(ping_params));
        ping_params.username      = s_username;
        ping_params.api_token     = s_token;
        ping_params.game_id       = s_game_id;
        ping_params.rich_presence = rp_buf[0] ? rp_buf : NULL;

        rc_api_request_t req;
        memset(&req, 0, sizeof(req));
        if (rc_api_init_ping_request(&req, &ping_params) == RC_OK) {
            wrc_dispatch(&req, NULL, NULL);
        }
    }
}
