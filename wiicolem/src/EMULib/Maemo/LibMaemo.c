/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibMaemo.c                       **/
/**                                                         **/
/** This file contains Maemo-dependent implementation       **/
/** parts of the emulation library.                         **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "Sound.h"
#include "Console.h"
#include "LibARM.h"

#include <hildon/hildon-program.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-file-system-model.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gtk/gtkmain.h>
#include <gdk/gdk.h>
#include <libosso.h>

#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "omapfb.h"

#define SCREEN_W     800
#define SCREEN_H     480
#define SCREEN_D     (sizeof(pixel)*8)
#define SCREEN_SIZE  (SCREEN_W*SCREEN_H*SCREEN_D/8) 
#define FPS_COLOR    PIXEL(255,0,255)
#define CUE_COLOR    PIXEL(127,127,127)

extern int MasterSwitch; /* Switches to turn channels on/off */
extern int MasterVolume; /* Master volume                    */

static volatile int TimerReady = 0;   /* 1: Sync timer ready */
static volatile int FocusOUT   = 0;   /* 1: GTK focus out    */
static volatile unsigned int JoyState = 0; /* Joystick state */
static volatile unsigned int LastKey  = 0; /* Last key prsd  */
static volatile unsigned int KeyModes = 0; /* SHIFT/CTRL/ALT */
static volatile unsigned int MseState = 0; /* Mouse state    */

static int Effects    = EFF_SCALE|EFF_SAVECPU; /* EFF_* bits */
static int TimerON    = 0;  /* 1: sync timer is running      */
static int FullScreen = 0;  /* 1: full screen mode           */
static int RedrawAll  = 0;  /* 1: Redraw whole window/screen */
int  ARGC             = 0;  /* Copies of argc/argv           */
char **ARGV           = 0;

static HildonProgram *HApp; /* Hildon application handle     */
static HildonWindow *HWnd;  /* Hildon window handle          */
static osso_context_t *Osso;/* Maemo services (DBus, etc.)   */
static GConfClient *Conf;   /* Configuration manager         */
static GtkMenu *Menu;       /* Top menu widget               */

static Image GTKImg;        /* Display widget image buffer   */
static GtkWidget *Widget;   /* Main display widget           */
static GdkGC *Gc;           /* Drawing context for Widget    */

static Image FBImg;         /* Frame buffer image buffer     */
static int FBFD;            /* Frame buffer file handle      */

static Image OutImg;        /* Output image buffer (cropped) */
static int OutWidth;        /* Preferred output image width  */
static int OutHeight;       /* Preferred output image height */

static int FrameCount;      /* Frame counter for EFF_SHOWFPS */
static int FrameRate;       /* Last frame rate value         */
static time_t TimeCount;    /* Time counter for EFF_SHOWFPS  */

static int (*FileHandler)(const char *Filename);

static void DrawVLine(Image *Img,int X,int Y1,int Y2,int Color);
static void DrawHLine(Image *Img,int X1,int X2,int Y,int Color);
static void DrawCues(Image *Img,int ShowDialpad,int Color);
static gint DBusHandler(const gchar *Interface,const gchar *Method,GArray *Args,gpointer Data,osso_rpc_t *Result);
static gint GTKKeyHandler(GtkWidget *SrcWidget,GdkEventKey *Event,gpointer Arg);
static gint GTKFocusHandler(GtkWidget *SrcWidget,GdkEventFocus *Event,gpointer Arg);
static gint GTKPenHandler(GtkWidget *SrcWidget,GdkEventCrossing *Event,gpointer Arg);
static void GTKMenuHandler(GtkAction *Action,int CmdID);
static void GTKQuitHandler(void);
static void TimerHandler(int Arg);

/** TimerHandler() *******************************************/
/** The main timer handler used by SetSyncTimer().          **/
/*************************************************************/
void TimerHandler(int Arg)
{
  /* Mark sync timer as "ready" */
  ++TimerReady;
  /* Repeat signal next time */
  signal(Arg,TimerHandler);
}

/** InitMaemo() **********************************************/
/** Initialize Unix/GTK resources and set initial window    **/
/** title and dimensions.                                   **/
/*************************************************************/
int InitMaemo(const char *Title,int Width,int Height,const char *Service)
{
  static gint8 DashList[] = { 1,4 };
  GtkWidget *Item;
  GdkColor Color;
  char *Object;
  int J;

  /* Initialize variables */
  OutImg.Data = 0;
  OutWidth    = Width;
  OutHeight   = Height;
  FBImg.Data  = 0;
  FBFD        = -1;
  GTKImg.Data = 0;
  Widget      = 0;
  Gc          = 0;
  FocusOUT    = 0;
  TimerON     = 0;
  TimerReady  = 0;
  FullScreen  = 0;
  RedrawAll   = 0;
  JoyState    = 0;
  MseState    = 0;
  LastKey     = 0;
  KeyModes    = 0;
  Osso        = 0;
  Conf        = 0;
  FileHandler = 0;
  FrameCount  = 0;
  FrameRate   = 0;
  TimeCount   = time(0);

  /* Initialize GTK and GLib type system */
  gtk_init(&ARGC,&ARGV);
  g_type_init();

  /* Initialize DBus access */
  Osso = osso_initialize(Service,"1.0.0",TRUE,0);
  if(!Osso) return(0);

  /* If DBus service name given... */
  if(Service)
  {
    /* Create Osso object name */
    Object = strdup(Service);
    if(!Object) { osso_deinitialize(Osso);Osso=0;return(0); }
    for(J=0;Object[J];++J) if(Object[J]=='.') Object[J]='/';
    /* Attach DBus message handler */
    if(osso_rpc_set_cb_f(Osso,Service,Object,Service,DBusHandler,0)!=OSSO_OK)
    { free(Object);osso_deinitialize(Osso);Osso=0;return(0); }
    /* Done with Osso object name */
    free(Object);
  }

  /* Create Hildon application, window, and top menu */
  HApp = HILDON_PROGRAM(hildon_program_get_instance());
  g_set_application_name(Title);
  HWnd = HILDON_WINDOW(hildon_window_new());
  hildon_program_add_window(HApp,HWnd);
  Menu = GTK_MENU(gtk_menu_new());
  hildon_window_set_menu(HWnd,Menu);

  /* Make window background black */
  Color.red=Color.green=Color.blue=0;
  gtk_widget_modify_bg(GTK_WIDGET(HWnd),GTK_STATE_NORMAL,&Color);

  /* Create new widget image buffer */
  if(!NewImage(&GTKImg,SCREEN_W,SCREEN_H)) return(0);

  /* Create widget from the image */
  Widget=gtk_image_new_from_image(GTKImg.GImg,0);
  /* Widget must exist now */
  if(!Widget) { FreeImage(&GTKImg);return(0); }

  /* Add widget to Hildon window */
  gtk_container_add(GTK_CONTAINER(HWnd),Widget);

  /* Add QUIT menu */
  if(Menu&&(Item=gtk_menu_item_new_with_label("Quit")))
  {
    gtk_menu_append(Menu,Item);
    g_signal_connect(G_OBJECT(Item),"activate",G_CALLBACK(GTKQuitHandler),0);
    gtk_widget_show_all(GTK_WIDGET(Menu));
  }

  /* Connect signals */
  g_signal_connect(G_OBJECT(HWnd),"delete_event",G_CALLBACK(GTKQuitHandler),0);
  g_signal_connect(G_OBJECT(HWnd),"key_press_event",G_CALLBACK(GTKKeyHandler),0);
  g_signal_connect(G_OBJECT(HWnd),"key_release_event",G_CALLBACK(GTKKeyHandler),0);
  g_signal_connect(G_OBJECT(HWnd),"focus_in_event",G_CALLBACK(GTKFocusHandler),0);
  g_signal_connect(G_OBJECT(HWnd),"focus_out_event",G_CALLBACK(GTKFocusHandler),0);
  g_signal_connect(G_OBJECT(HWnd),"enter_notify_event",G_CALLBACK(GTKPenHandler),0);
  g_signal_connect(G_OBJECT(HWnd),"leave_notify_event",G_CALLBACK(GTKPenHandler),0);

  /* Show menu and window */
  gtk_widget_show_all(GTK_WIDGET(Menu));
  gtk_widget_show_all(GTK_WIDGET(HWnd));

  /* Get configuration client handle */
  Conf=gconf_client_get_default();

  /* Create drawing context */
  Gc=gdk_gc_new(GDK_DRAWABLE(gtk_widget_get_parent_window(Widget)));

  /* We are going to use gray dotted lines */
  Color.red=Color.blue=Color.green=0x7FFF;
  gdk_gc_set_rgb_fg_color(Gc,&Color);
  gdk_gc_set_line_attributes(Gc,0,GDK_LINE_ON_OFF_DASH,GDK_CAP_BUTT,GDK_JOIN_MITER);
  gdk_gc_set_dashes(Gc,0,DashList,sizeof(DashList));
  gdk_gc_set_subwindow(Gc,GDK_INCLUDE_INFERIORS);

  /* Open frame buffer device */
  FBFD = open("/dev/fb0",O_RDWR);
  if(FBFD>=0)
  {
    /* Map hardware frame buffer */
    FBImg.Data = (pixel *)mmap(0,SCREEN_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,FBFD,0);
    if(!FBImg.Data) { close(FBFD);FBFD=-1; }
    else
    {
      /* Initialize frame buffer image */
      FBImg.Cropped = 1;
      FBImg.W = SCREEN_W;
      FBImg.H = SCREEN_H;
      FBImg.L = SCREEN_W;
      FBImg.D = SCREEN_D;
    }
  }

  /* Create output image buffer */
  GTKResizeVideo(Width,Height);

  /* Done */
  return(1);
}

/** TrashMaemo() *********************************************/
/** Free resources allocated in InitMaemo()                 **/
/*************************************************************/
void TrashMaemo(void)
{
  /* Free all allocated resources */
  if(Gc)   { gdk_gc_destroy(Gc);Gc=0; }
  if(Conf) { g_object_unref(Conf);Conf=0; }
  if(Osso) { osso_deinitialize(Osso);Osso=0; }

  /* Unmap hardware frame buffer */
  if(FBImg.Data) munmap(FBImg.Data,FBImg.L*FBImg.H*(FBImg.D>>3));
  /* Close frame buffer device */
  if(FBFD>=0) { close(FBFD);FBFD=-1; }

  /* Free image buffers */
  FreeImage(&OutImg);
  FreeImage(&GTKImg);
  FreeImage(&FBImg);
}

/** ShowVideo() **********************************************/
/** Show "active" image at the actual screen or window.     **/
/*************************************************************/
int ShowVideo(void)
{
  Image *Output,CueImg;
  int DW,DH,SW,SH;

  /* Have to have a window and a valid video buffer */
  if(!Widget||!Gc||!VideoImg||!VideoImg->GImg) return(0);

  /* When in full screen mode, use hardware frame buffer */
  Output = FullScreen&&FBImg.Data&&(FBFD>=0)? &FBImg:&OutImg;

  /* May have to wait for FB->LCD data transfer to complete */
  if(Output==&FBImg) ioctl(FBFD,OMAPFB_SYNC_GFX);

  /* When redrawing whole screen, clear buffers */
  if(RedrawAll) ClearImage(Output==&FBImg? &FBImg:&GTKImg,PIXEL(0,0,0));

  /* Process image as requested */
  if(Effects&EFF_SOFTEN)
  {
    SoftenImage(Output,VideoImg,VideoX,VideoY,VideoW,VideoH);
    DW = Output->W;
    DH = Output->H;
  }
  else if((Effects&EFF_TVLINES)||((Effects&EFF_SCALE)&&(Output!=&FBImg)))
  {
    DW = ARMScaleImage(Output,VideoImg,VideoX,VideoY,VideoW,VideoH,0);
    DH = DW>>16;
    DW&= 0xFFFF;
  }
  else
  {
    IMGCopy(
      Output,(Output->W-VideoW)>>1,(Output->H-VideoH)>>1,
      VideoImg,VideoX,VideoY,VideoW,VideoH,-1
    );
    DW = VideoW;
    DH = VideoH;
  }

  /* Apply TV scanlines if requested */
  if(Effects&EFF_TVLINES)
  {
    /* Make sure DW is a multiple of 16 pixels for optimization */
    DW = (DW+15)&~15;
    TelevizeImage(Output,(Output->W-DW)>>1,(Output->H-DH)>>1,DW,DH);
  }

  /* Count framerate */
  if(++FrameCount>=100)
  {
    FrameRate  = FrameCount/(time(0)-TimeCount);
    TimeCount  = time(0);
    FrameCount = 0;
    FrameRate  = FrameRate>999? 999:FrameRate;
  }

  /* Show framerate if requested */
  if((Effects&EFF_SHOWFPS)&&(FrameRate>0))
  {
    char S[8];
    sprintf(S,"%dfps",FrameRate);
    PrintXY(
      Output,S,
      ((Output->W-DW)>>1)+8,((Output->H-DH)>>1)+8,
      FPS_COLOR,-1
    );
  }

  /* Wait for sync timer if requested */
  if(Effects&EFF_SYNC) WaitSyncTimer();

  /* When drawing directly to hardware frame buffer... */
  if(Output==&FBImg)
  {
    static struct omapfb_update_window FBUpdate;

    /* For simple scaling, use hardware scaler */
    if((Effects&(EFF_SOFTEN|EFF_TVLINES|EFF_SCALE))==EFF_SCALE)
    {
      SW = FBImg.W/DW;
      SH = FBImg.H/DH;
      SH = SW<SH? SW:SH;
      SW = DW*SH;
      SH = DH*(DW/DH)*SW/DW;
      SH = SH<=FBImg.H? SH:FBImg.H;
      /* Overlay cue lines for pen input, if requested */
      if(Effects&EFF_PENCUES) 
      {
        /* Compute cue lines size */
        DW = FBImg.W*DW/SW;
        DH = FBImg.H*DH/SH;
        /* Draw cue lines */
        CropImage(&CueImg,&FBImg,(FBImg.W-DW)>>1,(FBImg.H-DH)>>1,DW,DH);
        DrawCues(&CueImg,Effects&EFF_DIALCUES,CUE_COLOR);
        /* Will be scaling to the whole buffer */
        SW = FBImg.W;
        SH = FBImg.H;        
      }
    }
    else
    {
      /* Overlay cue lines for pen input, if requested */
      if(Effects&EFF_PENCUES) 
      {
        /* Draw cue lines */
        DrawCues(&FBImg,Effects&EFF_DIALCUES,CUE_COLOR);
        /* Will be transferring the whole buffer */
        DW = FBImg.W;
        DH = FBImg.H;        
      }
      /* If redrawing whole screen... */
      if(RedrawAll) { DW=FBImg.W;DH=FBImg.H; }
      /* No scaling */
      SW = DW;
      SH = DH;
    }
    /* Set up frame buffer update */
//printf("%dx%d ==> %dx%d ==> %dx%d\n",VideoW,VideoH,DW,DH,SW,SH);
    FBUpdate.width      = DW;
    FBUpdate.height     = DH;
    FBUpdate.x          = (FBImg.W-DW)>>1;
    FBUpdate.y          = (FBImg.H-DH)>>1;
    FBUpdate.out_width  = SW;
    FBUpdate.out_height = SH;
    FBUpdate.out_x      = (FBImg.W-SW)>>1;
    FBUpdate.out_y      = (FBImg.H-SH)>>1;
    FBUpdate.format     = OMAPFB_COLOR_RGB565;
    /* If VSync requested... */
    if(Effects&EFF_VSYNC)
    {
      /* Modify transfer flags */
      FBUpdate.format|= OMAPFB_FORMAT_FLAG_TEARSYNC
                     |  OMAPFB_FORMAT_FLAG_FORCE_VSYNC; 
      /* Wait for VSync */
      ioctl(FBFD,OMAPFB_VSYNC);
    }
    /* Request FB transfer to LCD video memory */
    ioctl(FBFD,OMAPFB_UPDATE_WINDOW,&FBUpdate);
  }
  else
  {
    SW = Widget->allocation.width;
    SH = Widget->allocation.height;
    /* Overlay cue lines for pen input, if requested */
    if(Effects&EFF_PENCUES) 
    {
      CropImage(&CueImg,&GTKImg,(GTKImg.W-SW)>>1,(GTKImg.H-SH)>>1,SW,SH);
      DrawCues(&CueImg,Effects&EFF_DIALCUES,CUE_COLOR);
      /* Updating full rectangle */
      DW = SW;
      DH = SH;
    }
    /* If redrawing whole screen... */
    if(RedrawAll) { DW=SW;DH=SH; }
    /* Drawing to the widget (why need to add 32 to width?) */
    gtk_widget_queue_draw_area(Widget,(SW-DW-32)>>1,(SH-DH)>>1,DW+32,DH);
    /* Make sure drawing completes */
    gdk_window_process_updates(gtk_widget_get_parent_window(Widget),TRUE);
  }

  /* Done */
  if(RedrawAll) --RedrawAll;
  return(1);
}

/** NewImage() ***********************************************/
/** Create a new image of the given size. Returns pointer   **/
/** to the image data on success, 0 on failure.             **/
/*************************************************************/
pixel *NewImage(Image *Img,int Width,int Height)
{
  GdkVisual *GV;

#if defined(BPP8) || defined(BPP16) || defined(BPP24) || defined(BPP32)
  GV = gdk_visual_get_best_with_depth(SCREEN_D);
#else
  GV = gdk_visual_get_best();
#endif

  /* Create new GdkImage object */
  Img->GImg    = GV? gdk_image_new(GDK_IMAGE_FASTEST,GV,Width,Height):0;
  Img->Cropped = 0;

  /* If GdkImage created... */
  if(Img->GImg)
  {
    Img->Data = (pixel *)Img->GImg->mem;
    Img->W    = Img->GImg->width;
    Img->H    = Img->GImg->height;
    Img->L    = Img->GImg->bpl/Img->GImg->bpp;
    Img->D    = Img->GImg->bits_per_pixel;
  }
  else
  {
    Img->Data = 0;
    Img->W    = 0;
    Img->H    = 0;
    Img->L    = 0;
    Img->D    = 0;
  }

  /* Done */
  return(Img->Data);
}

/** FreeImage() **********************************************/
/** Free previously allocated image.                        **/
/*************************************************************/
void FreeImage(Image *Img)
{
  /* Free GdkImage object */
  if(Img->GImg) gdk_image_destroy(Img->GImg);

  /* Image gone */
  Img->GImg = 0;
  Img->Data = 0;
  Img->W    = 0;
  Img->H    = 0;
  Img->L    = 0;
  Img->D    = 0;
}

/** CropImage() **********************************************/
/** Create a subimage Dst of the image Src. Returns Dst.    **/
/*************************************************************/
Image *CropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  GenericCropImage(Dst,Src,X,Y,W,H);
  Dst->GImg = 0;
  return(Dst);
}

/** SetVideo() ***********************************************/
/** Set part of the image as "active" for display.          **/
/*************************************************************/
void SetVideo(Image *Img,int X,int Y,int W,int H)
{
  /* Call default SetVideo() function */
  GenericSetVideo(Img,X,Y,W,H);
  /* Automatically resize video output */
  GTKResizeVideo(OutWidth,OutHeight);
}

/** GetJoystick() ********************************************/
/** Get the state of joypad buttons (1="pressed"). Refer to **/
/** the BTN_* #defines for the button mappings.             **/
/*************************************************************/
unsigned int GetJoystick(void)
{
  /* Process any accumulated keypresses, etc. */
  GTKProcessEvents();

  /* Return joystick state, with modal keys */
  return(JoyState|KeyModes);
}

/** GetMouse() ***********************************************/
/** Get mouse position and button states in the following   **/
/** format: RMB.LMB.Y[29-16].X[15-0].                       **/
/*************************************************************/
unsigned int GetMouse(void)
{
  gint X,Y;

  /* Process events */
  GTKProcessEvents();
  /* Have to have a widget */
  if(!Widget) return(0);
  /* Get pointer coordinates */
  gtk_widget_get_pointer(Widget,&X,&Y);
  /* Translate, truncate, and scale coordinates */
  X = VideoW*(X-((Widget->allocation.width-OutImg.W)>>1))/OutImg.W;
  Y = VideoH*(Y-((Widget->allocation.height-OutImg.H)>>1))/OutImg.H;
  /* Do not return clicks outside of the widget */
  if((X<0)||(Y<0)||(X>=VideoW)||(Y>=VideoH)) MseState=0;
  X = X<0? 0:X>=VideoW? VideoW-1:X;
  Y = Y<0? 0:Y>=VideoH? VideoH-1:Y;
  /* Combine with mouse keys */
  return((X&0xFFFF)|((Y&0x3FFF)<<16)|MseState);
}

/** GetKey() *************************************************/
/** Get currently pressed key or 0 if none pressed. Returns **/
/** CON_* definitions for arrows and special keys.          **/
/*************************************************************/
unsigned int GetKey(void)
{
  unsigned int J;

  GTKProcessEvents();
  J=LastKey;
  LastKey=0;
  return(J);
}

/** WaitKey() ************************************************/
/** Wait for a key to be pressed. Returns CON_* definitions **/
/** for arrows and special keys.                            **/
/*************************************************************/
unsigned int WaitKey(void)
{
  unsigned int J;

  /* Swallow current keypress */
  GetKey();
  /* Wait in 100ms increments for a new keypress */
  while(!(J=GetKey())&&VideoImg) usleep(100000);
  /* Return key code */
  return(J);
}

/** WaitKeyOrMouse() *****************************************/
/** Wait for a key or a mouse button to be pressed. Returns **/
/** the same result as GetMouse(). If no mouse buttons      **/
/** reported to be pressed, do GetKey() to fetch a key.     **/
/*************************************************************/
unsigned int WaitKeyOrMouse(void)
{
  unsigned int J;

  /* Swallow current keypress */
  GetKey();
  /* Make sure mouse keys are not pressed */
  while(GetMouse()&0xC0000000) usleep(100000);
  /* Wait in 100ms increments for a key or mouse click */
  while(!(J=GetKey())&&!(GetMouse()&0xC0000000)&&VideoImg) usleep(100000);
  /* Place key back into the buffer and return mouse state */
  LastKey=J;
  return(GetMouse());
}

/** WaitSyncTimer() ******************************************/
/** Wait for the timer to become ready.                     **/
/*************************************************************/
void WaitSyncTimer(void)
{
  /* Wait in 1ms increments until timer becomes ready */
  while(!TimerReady&&TimerON&&VideoImg) usleep(1000);
  /* Warn of missed timer events */
  if((TimerReady>1)&&(Effects&EFF_VERBOSE))
    printf("WaitSyncTimer(): Missed %d timer events.\n",TimerReady-1);
  /* Reset timer */
  TimerReady=0;
}

/** SyncTimerReady() *****************************************/
/** Return 1 if sync timer ready, 0 otherwise.              **/
/*************************************************************/
int SyncTimerReady(void)
{
  /* Return whether timer is ready or not */
  return(TimerReady||!TimerON||!VideoImg);
}

/** SetSyncTimer() *******************************************/
/** Set synchronization timer to a given frequency in Hz.   **/
/*************************************************************/
int SetSyncTimer(int Hz)
{
  struct itimerval TimerValue;

  /* Compute and set timer period */
  TimerValue.it_interval.tv_sec  =
  TimerValue.it_value.tv_sec     = 0;
  TimerValue.it_interval.tv_usec =
  TimerValue.it_value.tv_usec    = Hz? 1000000L/Hz:0;

  /* Set timer */
  if(setitimer(ITIMER_REAL,&TimerValue,NULL)) return(0);

  /* Set timer signal */
  signal(SIGALRM,TimerHandler);

  /* Done */
  TimerON=Hz;
  return(1);
}

/** ChangeDir() **********************************************/
/** This function is a wrapper for chdir().                 **/
/*************************************************************/
int ChangeDir(const char *Name) { return(chdir(Name)); }

/** GTKProcessEvents() ***************************************/
/** Process GTK event messages.                             **/
/*************************************************************/
void GTKProcessEvents(void)
{
  do
  {
    /* Process all accumulated GTK events */
    while(gtk_events_pending()) gtk_main_iteration_do(FALSE);
    /* Wait in 1s increments for the focus to return */
    if(FocusOUT) sleep(1);
  }
  while(FocusOUT);
}

/** GTKSetEffects() ******************************************/
/** Set visual effects applied to video in ShowVideo().     **/
/*************************************************************/
void GTKSetEffects(int NewEffects)
{
  /* Will be redrawing whole screen next time */
  RedrawAll=1;
  /* Set new effects */
  Effects=NewEffects;
}

/** GTKQuitHandler() *****************************************/
/** Process application quit.                               **/
/*************************************************************/
void GTKQuitHandler(void)
{
  GdkEventKey Event;

  /* We are quitting */
  VideoImg     = 0;
  FocusOUT     = 0;
  /* Simulate F12 (quit) keypress */
  Event.type   = GDK_KEY_PRESS;
  Event.keyval = GDK_F12;
  GTKKeyHandler(Widget,&Event,0);
}

/** GTKMenuHandler() *****************************************/
/** Forward user menu selections to keyboard handler, as    **/
/** keys.                                                   **/
/*************************************************************/
void GTKMenuHandler(GtkAction *Action,int CmdID)
{
  /* If key handler installed, feed it with menu command ID */
  if(KeyHandler) (*KeyHandler)((CmdID&CON_KEYCODE)|KeyModes);
  /* Send key press */
  LastKey = CmdID&CON_KEYCODE;
}

/** GTKFocusHandler() ****************************************/
/** Process focus events.                                   **/
/*************************************************************/
gint GTKFocusHandler(GtkWidget *SrcWidget,GdkEventFocus *Event,gpointer Arg)
{
  static int J;

  if(Event->in)
  {
    /* When focus was out... */
    if(FocusOUT)
    {
      /* Audio back on */
      PauseAudio(0);
      SetChannels(MasterVolume,J);
      /* Focus is now in */
      FocusOUT=0;
    }
  }
  else
  {
    /* When save-CPU option enabled... */
    if(Effects&EFF_SAVECPU)
    {
      /* Audio off */
      J=MasterSwitch;
      SetChannels(MasterVolume,0);
      PauseAudio(1);
      /* Focus is now out */
      FocusOUT=1;
    }
  }

  /* Event handled */
  return(TRUE);
}

/** GTKPenHandler() ******************************************/
/** Process pen events.                                     **/
/*************************************************************/
gint GTKPenHandler(GtkWidget *SrcWidget,GdkEventCrossing *Event,gpointer Arg)
{
  /* Simulate mouse button press/release */
  if(Event->type!=GDK_LEAVE_NOTIFY) MseState&=~0x40000000;
  else
  {
    /* Mouse button now pressed */
    MseState|=0x40000000;
    /* Get pointer coordinates */
    switch(PenJoystick())
    {
      case BTN_FIREB:
      case BTN_EXIT:  LastKey=CON_EXIT;break;
      case BTN_FIREA:
      case BTN_START: LastKey=CON_OK;break;
      case BTN_LEFT:  LastKey=CON_LEFT;break;
      case BTN_RIGHT: LastKey=CON_RIGHT;break;
      case BTN_UP:    LastKey=CON_UP;break;
      case BTN_DOWN:  LastKey=CON_DOWN;break;
    }
  }

  /* Event handled */
  return(TRUE);
}

/** GTKKeyHandler() ******************************************/
/** Process key presses/releases.                           **/
/*************************************************************/
gint GTKKeyHandler(GtkWidget *SrcWidget,GdkEventKey *Event,gpointer Arg)
{
  unsigned int J;
  GdkWindow *W;

  /* If key pressed... */
  if(Event->type==GDK_KEY_PRESS)
  {
//printf("keyval=%X, code=%X\n",Event->keyval,Event->hardware_keycode);
    /* Special keys pressed... */
    switch(J=Event->keyval)
    {
      case GDK_Left:         JoyState|=BTN_LEFT;LastKey=CON_LEFT;break;
      case GDK_Right:        JoyState|=BTN_RIGHT;LastKey=CON_RIGHT;break;
      case GDK_Up:           JoyState|=BTN_UP;LastKey=CON_UP;break;
      case GDK_Down:         JoyState|=BTN_DOWN;LastKey=CON_DOWN;break;
      case GDK_Shift_L:
      case GDK_Shift_R:      KeyModes|=CON_SHIFT;break;
      case GDK_Alt_L:
      case GDK_Alt_R:        KeyModes|=CON_ALT;break;
      case GDK_Control_L:
      case GDK_Control_R:    KeyModes|=CON_CONTROL;break;
      case GDK_Caps_Lock:    KeyModes|=CON_CAPS;break;
      case GDK_Escape:       JoyState|=BTN_EXIT;LastKey=CON_EXIT;break;
      case ',':
      case GDK_Tab:          JoyState|=BTN_SELECT;break;
      case GDK_BackSpace:    LastKey=8;break;
      case '.':
      case GDK_KP_Enter:     JoyState|=BTN_START;LastKey=CON_OK;break;
      case GDK_F4:           LastKey=CON_MENU;break;

      case GDK_Return:
        /* This is DPad center, have to use modifiers */
        /* to protect against accidental keypresses   */
        if(KeyModes&(CON_CONTROL|CON_SHIFT|CON_ALT)) JoyState|=BTN_START;
        LastKey=CON_OK;
        break;

      case 'q': case 'e': case 't': case 'u': case 'o':
        JoyState|=BTN_FIREL;break;
      case 'w': case 'r': case 'y': case 'i': case 'p':
        JoyState|=BTN_FIRER;break;
      case 'a': case 's': case 'd': case 'f': case 'g':
      case 'h': case 'j': case 'k': case 'l': case ' ':
        JoyState|=BTN_FIREA;break;
      case 'z': case 'x': case 'c': case 'v': case 'b':
      case 'n': case 'm':
        JoyState|=BTN_FIREB;break;

      case GDK_F6: /* Maemo FullScreen */
         W=gtk_widget_get_parent_window(Widget);
         /* Toggle full-screen mode */
         FullScreen=!FullScreen;
         if(FullScreen) gdk_window_fullscreen(W);
         else           gdk_window_unfullscreen(W);
         /* Automatically resize video output */
         GTKResizeVideo(OutWidth,OutHeight);
         break;

      case GDK_F7: /* Maemo VolumeUP */
        /* Volume up */
        if(Effects&EFF_NOVOLUME) LastKey=CON_VOLUP;
        else SetChannels(MasterVolume<247? MasterVolume+8:255,MasterSwitch);
        break;

      case GDK_F8: /* Maemo VolumeDOWN */
        /* Volume down */
        if(Effects&EFF_NOVOLUME) LastKey=CON_VOLDOWN;
        else SetChannels(MasterVolume>8? MasterVolume-8:0,MasterSwitch);
        break;

      case GDK_F12: /* Quit application */
        /* Remove video buffer to let EMULib know we are quitting */
        VideoImg=0;
        break;
    }

    /* Process ASCII keys */
    if((J>=' ')&&(J<0x7F)) LastKey=toupper(J);
    /* Call user handler */
    if(J&&KeyHandler) (*KeyHandler)(J|KeyModes);
  }

  /* If key released... */
  if(Event->type==GDK_KEY_RELEASE)
  {
    /* Special keys released... */
    switch(J=Event->keyval)
    {
      case GDK_Left:         JoyState&=~BTN_LEFT;break;
      case GDK_Right:        JoyState&=~BTN_RIGHT;break;
      case GDK_Up:           JoyState&=~BTN_UP;break;
      case GDK_Down:         JoyState&=~BTN_DOWN;break;
      case GDK_Shift_L:
      case GDK_Shift_R:      KeyModes&=~CON_SHIFT;break;
      case GDK_Alt_L:
      case GDK_Alt_R:        KeyModes&=~CON_ALT;break;
      case GDK_Control_L:
      case GDK_Control_R:    KeyModes&=~CON_CONTROL;break;
      case GDK_Caps_Lock:    KeyModes&=~CON_CAPS;break;
      case GDK_Escape:       JoyState&=~BTN_EXIT;break;
      case ',':
      case GDK_Tab:          JoyState&=~BTN_SELECT;break;
      case '.':
      case GDK_Return:
      case GDK_KP_Enter:     JoyState&=~BTN_START;break;

      case 'q': case 'e': case 't': case 'u': case 'o':
        JoyState&=~BTN_FIREL;break;
      case 'w': case 'r': case 'y': case 'i': case 'p':
        JoyState&=~BTN_FIRER;break;
      case 'a': case 's': case 'd': case 'f': case 'g':
      case 'h': case 'j': case 'k': case 'l': case ' ':
        JoyState&=~BTN_FIREA;break;
      case 'z': case 'x': case 'c': case 'v': case 'b':
      case 'n': case 'm':
        JoyState&=~BTN_FIREB;break;
    }

    /* Call user handler */
    if(J&&KeyHandler) (*KeyHandler)(J|CON_RELEASE|KeyModes);
  }

  /* Event handled */
  return(TRUE);
}

/** GTKAddMenu() *********************************************/
/** Add a menu with a command ID.                           **/
/*************************************************************/
int GTKAddMenu(const char *Label,int CmdID)
{
  GtkWidget *Item;

  if(!Menu) return(0);
  Item = Label? gtk_menu_item_new_with_label(Label)
              : gtk_separator_menu_item_new();
  if(!Item) return(0);
  gtk_menu_insert(Menu,Item,g_list_length(GTK_MENU_SHELL(Menu)->children)-1);
  g_signal_connect(G_OBJECT(Item),"activate",G_CALLBACK(GTKMenuHandler),(void *) CmdID);
  gtk_widget_show_all(GTK_WIDGET(Menu));
  return(1);
}

/** GTKAskFilename() *****************************************/
/** Ask user to select a file and return the file name, or  **/
/** 0 on cancel.                                            **/
/*************************************************************/
const char *GTKAskFilename(GtkFileChooserAction Action)
{
  gchar *FileName,*PathName;
  GtkWidget *Dialog;

  /* Must have a window */
  if(!HWnd) return(0);

  /* Create file selection dialog */
  Dialog = hildon_file_chooser_dialog_new(GTK_WINDOW(HWnd),Action);
  if(!Dialog) return(0);

  /* Set initial path */
  PathName = (gchar *)GTKGetString("path");
  if(PathName)
  {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(Dialog),PathName);
    g_free(PathName);
  }

  /* Show and run dialog */
  gtk_widget_show_all(GTK_WIDGET(Dialog));
  FileName = gtk_dialog_run(GTK_DIALOG(Dialog))==GTK_RESPONSE_OK?
    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Dialog)):0;

  /* If selection made, memorize path for the next time */
  if(FileName)
  {
    PathName = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(Dialog));
    if(PathName) { GTKSetString("path",PathName);g_free(PathName); }
  }

  /* Done */
  gtk_widget_destroy(Dialog);
  return(FileName);
}

/** GTKMessage() *********************************************/
/** Show text message to the user.                          **/
/*************************************************************/
void GTKMessage(const char *Text)
{
  GtkWidget *W;

  /* Create message widget */
  if(!HWnd||!(W=hildon_note_new_information(GTK_WINDOW(HWnd),Text))) return;
  /* Show message */
  gtk_dialog_run(GTK_DIALOG(W));
  gtk_widget_destroy(W);
}

/** DBusHandler() ********************************************/
/** Process DBus messages.                                  **/
/*************************************************************/
gint DBusHandler(const gchar *Interface,const gchar *Method,GArray *Args,gpointer Data,osso_rpc_t *Result)
{
  const char *Hex = "0123456789ABCDEF";
  osso_rpc_t V;
  char *S,*D,*P,*T;

  if(Method&&Args&&!strcmp(Method,"mime_open"))
  {
    /* Get first argument (filename) */
    V = g_array_index(Args,osso_rpc_t,0);

    /* If filename valid and belongs to local filesystem... */
    if((V.type==DBUS_TYPE_STRING)&&V.value.s&&!memcmp(V.value.s,"file://",7))
    {
      /* Convert name to plain ASCII */
      for(D=V.value.s,S=D+7;*S;)
        if(S[0]!='%') *D++=*S++;
        else
        {
          P = strchr(Hex,toupper(S[1]));
          T = P? strchr(Hex,toupper(S[2])):0;
          if(!P||!T) *D++=*S++; else { *D++=((P-Hex)<<4)+(T-Hex);S+=3; }
        }
      *D='\0';

      /* If file handler present, call it */
      if(FileHandler&&(*FileHandler)(V.value.s))
      {
        /* Bring application to front */
        if(HWnd) gtk_window_present(GTK_WINDOW(HWnd));
        /* Successfully done with the message */
        Result->type    = DBUS_TYPE_BOOLEAN;
        Result->value.b = TRUE;
        return(OSSO_OK);
      }
    }

    /* Message processing failed */
    return(OSSO_ERROR);
  }

  /* Message not recognized */
  return(DBUS_TYPE_INVALID);
}

/** DrawVLine()/DrawHLine() **********************************/
/** Draw dotted lines used to show cues for PenJoystick().  **/
/*************************************************************/
void DrawVLine(Image *Img,int X,int Y1,int Y2,int Color)
{
  pixel *P;
  int J;

  if(Y1>Y2) { J=Y1;Y1=Y2;Y2=J; }
  P = (void *)((char *)Img->Data+(Img->L*Y1+X)*(Img->D>>3));  
  for(J=Y1;J<=Y2;J+=4) { *P=Color;P+=Img->L<<2; }
}

void DrawHLine(Image *Img,int X1,int X2,int Y,int Color)
{
  pixel *P;
  int J;

  if(X1>X2) { J=X1;X1=X2;X2=J; }
  P = (void *)((char *)Img->Data+(Img->L*Y+X1)*(Img->D>>3));  
  for(J=X1;J<=X2;J+=4) { *P=Color;P+=4; }
}

/** DrawCues() ***********************************************/
/** Overlay dotted cue lines for using PenJoystick() onto a **/
/** given image. Show dialpad cues if requested.            **/
/*************************************************************/
void DrawCues(Image *Img,int ShowDialpad,int Color)
{
  pixel *P;
  gint W,H,W9,W3,H3;

  P  = Img->Data;
  W  = Img->W;
  H  = Img->H;
  W9 = W/9;
  W3 = W/3;
  H3 = H/3;

  /* Vertical edges */
  DrawVLine(Img,W3,0,H-1,Color);
  DrawVLine(Img,W-W3,0,H-1,Color);

  /* Corner buttons */
  DrawHLine(Img,0,W3,H>>4,Color);
  DrawHLine(Img,W-W3,W-1,H>>4,Color);
  DrawHLine(Img,0,W3,H-(H>>4),Color);
  DrawHLine(Img,W-W3,W-1,H-(H>>4),Color);

  /* Fire buttons (with overlap) */
  DrawHLine(Img,W-W3,W-1,(H>>1)-(H>>4),Color);
  DrawHLine(Img,W-W3,W-1,(H>>1)+(H>>4),Color);

  /* Directional buttons */
  DrawVLine(Img,W9,H>>4,H-(H>>4),Color);
  DrawVLine(Img,W3-W9,H>>4,H-(H>>4),Color);
  DrawHLine(Img,0,W3,H3,Color);
  DrawHLine(Img,0,W3,H-H3,Color);

  /* Button labels */
  PrintXY(Img,"L",2,2,Color,-1);
  PrintXY(Img,"R",W-W3+2,2,Color,-1);
  PrintXY(Img,"B",W-W3+2,(H>>4)+2,Color,-1);
  PrintXY(Img,"A+B",W-W3+2,(H>>1)-(H>>4)+2,Color,-1);
  PrintXY(Img,"A",W-W3+2,(H>>1)+(H>>4)+2,Color,-1);
  PrintXY(Img,"SELECT",2,H-(H>>4)+2,Color,-1);
  PrintXY(Img,"START",W-W3+2,H-(H>>4)+2,Color,-1);

  /* If requested, show on-screen dialpad */
  if(ShowDialpad)
  {
    DrawHLine(Img,W3,W-W3,H>>2,Color);
    DrawHLine(Img,W3,W-W3,H>>1,Color);
    DrawHLine(Img,W3,W-W3,H-(H>>2),Color);
    DrawVLine(Img,W3+W9,0,H-1,Color);
    DrawVLine(Img,W-W3-W9,0,H-1,Color);
    PrintXY(Img,"1",W3+2,2,Color,-1);
    PrintXY(Img,"2",W3+W9+2,2,Color,-1);
    PrintXY(Img,"3",W-W3-W9+2,2,Color,-1);
    PrintXY(Img,"4",W3+2,(H>>2)+2,Color,-1);
    PrintXY(Img,"5",W3+W9+2,(H>>2)+2,Color,-1);
    PrintXY(Img,"6",W-W3-W9+2,(H>>2)+2,Color,-1);
    PrintXY(Img,"7",W3+2,(H>>1)+2,Color,-1);
    PrintXY(Img,"8",W3+W9+2,(H>>1)+2,Color,-1);
    PrintXY(Img,"9",W-W3-W9+2,(H>>1)+2,Color,-1);
    PrintXY(Img,"*",W3+2,H-(H>>2)+2,Color,-1);
    PrintXY(Img,"0",W3+W9+2,H-(H>>2)+2,Color,-1);
    PrintXY(Img,"#",W-W3-W9+2,H-(H>>2)+2,Color,-1);
  }
}

/** GTKResizeVideo() *****************************************/
/** Change output rectangle dimensions.                     **/
/*************************************************************/
int GTKResizeVideo(int Width,int Height)
{
  int SX,SY,NX,NY,GX,GY;
  Image *Img;

  /* Will be redrawing whole screen next time */
  RedrawAll = 1;

  /* Memorize new output size preferences */
  OutWidth  = Width;
  OutHeight = Height;

  /* Compute new GTK widget size */
  GX = FullScreen? SCREEN_W:720;
  GY = FullScreen? SCREEN_H:448;

  /* Automatically compute width, if requested */
  if(Width>0)
  {
    SX = VideoImg&&(VideoW>0)? VideoW:Width;
    NX = (Width+SX-1)/SX;
  }
  else
  {
    SX = VideoImg&&(VideoW>0)&&(VideoW<GX)? VideoW:GX; 
    for(NX=1;SX*(NX+1)<=GX;++NX);
  }

  /* Automatically compute height, if requested */
  if(Height>0)
  {
    SY = VideoImg&&(VideoH>0)? VideoH:Height;
    NY = (Height+SY-1)/SY;
  }
  else
  {
    SY = VideoImg&&(VideoH>0)&&(VideoH<GY)? VideoH:GY; 
    for(NY=1;SY*(NY+1)<=GY;++NY);
  }

  /* Make sure automatically sized image is proportional */
  if(NX>NY) NX=NY; else NY=NX;
  if(Width<=0)  Width=SX*NX;
  if(Height<=0) Height=SY*NY;

  /* Crop new subimage as output image */
  Img = FullScreen&&(FBFD>=0)&&FBImg.Data? &FBImg:&GTKImg;
  CropImage(&OutImg,Img,(Img->W-Width)>>1,(Img->H-Height)>>1,Width,Height);

  /* Done */
  return(1);
}

/** SetFileHandler() *****************************************/
/** Attach handler that will be called to open files.       **/
/*************************************************************/
void SetFileHandler(int (*Handler)(const char *Filename))
{
  int J;

  /* Set new DBus open-file message handler */
  FileHandler = Handler;

  /* Spin GTK loop to let pending DBus messages come through */
  if(Handler)
    for(J=0;J<20;++J) { gtk_main_iteration_do(FALSE);usleep(20); }
}

/** PenJoystick() ********************************************/
/** Get simulated joystick buttons from touch screen UI.    **/
/** Result compatible with GetJoystick().                   **/
/*************************************************************/
unsigned int PenJoystick(void)
{
  unsigned int J;
  gint X,Y,W,H;

  /* Simulate joystick only when pen touches the screen */
  J=0;
  if(Widget&&(MseState&0xC0000000))
  {
    /* Get pointer coordinates */
    gtk_widget_get_pointer(Widget,&X,&Y);
    W = Widget->allocation.width;
    H = Widget->allocation.height;
    /* Don't accept touches outside of the window frame */
    if((X<0)||(Y<0)||(X>=W)||(Y>=H)) return(0);
    W/= 3;
    /* Top 1/16 of the widget: FIREL and FIRER */
    if(Y<(H>>4))
    { if(X<W) J|=BTN_FIREL; else if(X>=(W<<1)) J|=BTN_FIRER; }
    /* Bottom 1/16 of the widget: SELECT/EXIT and START */
    if(!J&&(Y>=(H-(H>>4))))
    { if(X<W) J|=BTN_SELECT|BTN_EXIT; else if(X>=(W<<1)) J|=BTN_START; }
    /* Right 1/3 of the screen is the fire buttons */
    if(!J&&(X>=(W<<1)))
    {
      /* Fire buttons overlap */
      if(Y<=(H>>1)+(H>>4)) J|=BTN_FIREB;
      if(Y>=(H>>1)-(H>>4)) J|=BTN_FIREA;
    }
    /* Left 1/3 of the screen is the directional pad */
    if(!J&&(X<W))
    {
      H/=3;
      W/=3;
      if(X<W) J|=BTN_LEFT; else if(X>=(W<<1)) J|=BTN_RIGHT;
      if(Y<H) J|=BTN_UP;   else if(Y>=(H<<1)) J|=BTN_DOWN;
    }
  }

  /* Done, return simulated "joystick" state */
  return(J);
}

/** PenDialpad() *********************************************/
/** Get simulated dialpad buttons from touch screen UI.     **/
/*************************************************************/
unsigned char PenDialpad(void)
{
  unsigned int J;
  gint X,Y,W,H;

  /* No dialpad buttons yet */
  J=DIAL_NONE;

  /* Simulate dialpad only when pen touches the screen */
  if(Widget&&(MseState&0xC0000000))
  {
    /* Get pointer coordinates */
    gtk_widget_get_pointer(Widget,&X,&Y);
    W = Widget->allocation.width/3;
    H = Widget->allocation.height;
    /* Dialpad is the middle 1/3 of the screen */
    if((X>=W)&&(X<(W<<1))) J=3*(X-W)/W+3*(Y/(H>>2))+1;
  }

  /* Done, return simulated "dialpad" state */
  return(J);
}

/** GTKGet*()/GTKSet*() **************************************/
/** Wrappers for getting and setting GConf config values.   **/
/*************************************************************/
const char *GTKGetString(const char *Key)
{
  char *P,*Path,*Value;

  /* Allocate buffer for config path */
  P    = strrchr(ARGV[0],'/');
  P    = P? P+1:ARGV[0];
  Path = malloc(strlen(P)+strlen(Key)+8);
  if(!Path) return(0);

  /* Compose config path */
  strcpy(Path,"/apps/");
  strcat(Path,P);
  strcat(Path,"/");
  strcat(Path,Key);

  /* Get and return value */
  Value=gconf_client_get_string(Conf,Path,0);
  free(Path);
  return(Value);
}

unsigned int GTKGetInteger(const char *Key,unsigned int Default)
{
  char *P,*Path;
  GConfValue *CfgValue;
  gint Value;

  /* Allocate buffer for config path */
  P    = strrchr(ARGV[0],'/');
  P    = P? P+1:ARGV[0];
  Path = malloc(strlen(P)+strlen(Key)+8);
  if(!Path) return(0);

  /* Compose config path */
  strcpy(Path,"/apps/");
  strcat(Path,P);
  strcat(Path,"/");
  strcat(Path,Key);

  /* Get and verify value */
  CfgValue = gconf_client_get_without_default(Conf,Path,0);
  Value    = CfgValue&&(CfgValue->type==GCONF_VALUE_INT)?
    gconf_value_get_int(CfgValue) : Default;

  /* Done */
  if(CfgValue) gconf_value_free(CfgValue);
  free(Path);
  return(Value);
}

int GTKSetString(const char *Key,const char *Value)
{
  char *P,*Path;
  int Result;

  /* Allocate buffer for config path */
  P    = strrchr(ARGV[0],'/');
  P    = P? P+1:ARGV[0];
  Path = malloc(strlen(P)+strlen(Key)+8);
  if(!Path) return(0);

  /* Compose config path */
  strcpy(Path,"/apps/");
  strcat(Path,P);
  strcat(Path,"/");
  strcat(Path,Key);

  /* Set value */
  Result=gconf_client_set_string(Conf,Path,Value,0);
  free(Path);
  return(Result);
}

int GTKSetInteger(const char *Key,unsigned int Value)
{
  char *P,*Path;
  int Result;

  /* Allocate buffer for config path */
  P    = strrchr(ARGV[0],'/');
  P    = P? P+1:ARGV[0];
  Path = malloc(strlen(P)+strlen(Key)+8);
  if(!Path) return(0);

  /* Compose config path */
  strcpy(Path,"/apps/");
  strcat(Path,P);
  strcat(Path,"/");
  strcat(Path,Key);

  /* Set value */
  Result=gconf_client_set_int(Conf,Path,Value,0);
  free(Path);
  return(Result);
}
