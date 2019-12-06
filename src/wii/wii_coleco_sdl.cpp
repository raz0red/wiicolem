//---------------------------------------------------------------------------//
//   __      __.__.___________        .__                                    //
//  /  \    /  \__|__\_   ___ \  ____ |  |   ____   _____                    //
//  \   \/\/   /  |  /    \  \/ /  _ \|  | _/ __ \ /     \                   //
//   \        /|  |  \     \___(  <_> )  |_\  ___/|  Y Y  \                  //
//    \__/\  / |__|__|\______  /\____/|____/\___  >__|_|  /                  //
//         \/                \/                 \/      \/                   //
//     WiiColem by raz0red                                                   //
//     Port of the ColEm emulator by Marat Fayzullin                         //
//                                                                           //
//     [github.com/raz0red/wiicolem]                                         //
//                                                                           //
//---------------------------------------------------------------------------//
//                                                                           //
//  Copyright (C) 2019 raz0red                                               //
//                                                                           //
//  The license for ColEm as indicated by Marat Fayzullin, the author of     //
//  ColEm is detailed below:                                                 //
//                                                                           //
//  ColEm sources are available under three conditions:                      //
//                                                                           //
//  1) You are not using them for a commercial project.                      //
//  2) You provide a proper reference to Marat Fayzullin as the author of    //
//     the original source code.                                             //
//  3) You provide a link to http://fms.komkon.org/ColEm/                    //
//                                                                           //
//---------------------------------------------------------------------------//

#include "wii_sdl.h"

#include "wii_coleco.h"

/**
 * Initializes the SDL library
 */
int wii_sdl_handle_init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        return 0;
    }

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        return 0;
    }

    back_surface = 
        SDL_SetVideoMode(
            WII_WIDTH,
            WII_HEIGHT, 
            8, 
            SDL_DOUBLEBUF|SDL_HWSURFACE
        );

    if (!back_surface) {
        return 0;
    }

    blit_surface = 
        SDL_CreateRGBSurface(
        SDL_SWSURFACE, 
        COLECO_WIDTH, 
        COLECO_HEIGHT,
        back_surface->format->BitsPerPixel,
        back_surface->format->Rmask,
        back_surface->format->Gmask,
        back_surface->format->Bmask, 0);

    return 1;
}
