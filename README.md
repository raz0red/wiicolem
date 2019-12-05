[![Actions Status](https://github.com/raz0red/wiicolem/workflows/Build/badge.svg)](https://github.com/raz0red/wiicolem/actions)

# WiiColEm

Ported by raz0red

_WiiColem Video_

<a href='http://www.youtube.com/watch?feature=player_embedded&v=UdHW9kOBeiE' target='_blank'><img src='http://img.youtube.com/vi/UdHW9kOBeiE/0.jpg' width='425' height=344 /></a>

## Overview

WiiColEm is a port of the [ColEm ColecoVision emulator](https://fms.komkon.org/ColEm/) developed by 
Marat Fayzullin.

Features:

  * Support for driving, roller, and super action controllers
  * Tilt-based (Wiimote) driving support
  * Cartridge database w/ recommended controller settings and keypad 
    descriptions for most commercial cartridges
  * Cartridge keypad overlays
  * Per-cartridge button mappings  
  * High cartridge compatibility (see below) 

The following additions/modifications were made to the core emulation code:

  * Support for ColecoVision MegaCart(R)
  * "The Heist" now works correctly (memory initialization bug).
  * Added support for Opcode memory expansion.
  * Added support for "Lord of the Dungeon" (original 32k or trimmed 24k).
  * Mode 2 masking now works correctly (supports Daniel Bienvenu games).
  * Mode 0 and Mode 3 now work correctly ("Cabbage Patch Kids Picture
    Show" and "Smurf: Paint 'n Play Workshop" now work correctly).
  * Fixed graphic corruption that would occur when switching between games
    (VRAM and related state were not being reset correctly).
  * Fixed save/load state bug where the emulator would incorrectly report
    that the save was invalid (memory was not being cleared correctly).
  * Fixed save/load state bug causing the palette to not be restored 
    correctly (Aquattack, War Room)
  * Fixed issue where noise channel wasn't starting when it should (Matt
    Patrol). 
    
## What about BlueMSX-wii? 

So, a common question might be why release WiiColEm when the excellent
BlueMSX-wii emulator exists? The truth is that I started this project some
time ago prior to the release of BlueMSX-wii. After finally getting some free
time again I decided to go ahead and complete the port. The reason I decided
to release it is that it is an entirely different code base. As such, it may
play some things that BlueMSX-wii does not. WiiColEm also supports all of the
controllers (Driving, Roller, and Spinner). Finally, it has an on-screen 
keypad display with key descriptions and overlay support so all Coleco games
should be playable. 

## Known issues

  * Zip files are not currently supported 
  * The noise channel isn't accurate (core bug) 

## Installation

To install WiiColEm, simply extract the zip file directly to your SD card
or USB device (retain the hierarchical structure exactly).

Cartridge images must be placed in the roms directory (/wiicolem/roms).

## Cartridge Database 

WiiColem ships with a database that contains recommended settings for most
commercial cartridges. These settings cover controls mappings, keypad
overlays, keypad button descriptions, and advanced settings (whether
the cartridge requires the Opcode memory expansion, etc.).

To view/edit the settings applied for a particular cartridge perform the
following steps:

  * Load the cartridge (via the "Load cartridge" menu item)
  * Return to the WiiColem menu
  * Select "Cartridge settings (current cartridge)" menu item
  * Examine the "Control settings" and "Advanced" settings for the cartridge 

For more information on mapping controls and creating and/or customizing
cartridge settings, see the "Cartridge Settings" section (below). 

## Controls

The following section contains the "default" control mappings for WiiColEm.

It is important to note that if the cartridge that is being loaded exists in
the Cartridge Database it may contain non-default mappings. 

### WiiColEm menu:
  
    Wiimote:

      Left/Right  : Scroll (if sideways orientation)
      Up/Down     : Scroll (if upright orientation)
      A/2         : Select 
      B/1         : Back
      Home        : Exit to Homebrew Channel
      Power       : Power off

    Classic controller/Pro:

      Up/Down     : Scroll
      A           : Select 
      B           : Back
      Home        : Exit to Homebrew Channel
            
    Nunchuk controller:

      Up/Down     : Scroll
      C           : Select 
      Z           : Back
                  
    GameCube controller:

      Up/Down     : Scroll
      A           : Select 
      B           : Back
      Z           : Exit to Homebrew Channel
                        
### In-game (Keypad):
  
  The keypad allows you to press keys on the ColecoVision controller keypads.
  If a description has been provided for the currently selected key it will
  be displayed above the keypad.

  You can select whether you want emulation to pause while the keypad is
  displayed via the "Keypad pause" option in "Advanced" settings (pausing is
  enabled by default). This value can be overridden on a cartridge-by-
  cartridge basis via Cartridge Settings (see "Cartridge Settings" section,
  below).

  * When keypad pause is enabled, the keypad will be closed when a keypad
      button is pressed (or the keypad is explicitly closed). It is important
      to note that the keypad button will continue to be pressed as long as
      the controller button is held. This is necessary for games like War 
      Room where you need to hold the keypad buttons down to see the 
      different factories, etc.
      
  * When keypad pause is disabled, emulation will continue while the keypad
      is displayed. The keypad will continue to be displayed until it is
      explicitly closed. 

  It is also important to note that commonly used keys can be mapped to Wii
  controller buttons (see "Cartridge Settings" section, below).
  
    Wiimote:

      D-pad          : Choose Key
      2, 1, A, B     : Press Key
      +              : Close Keypad
      
    Wiimote + Nunchuk:

      D-pad, Analog  : Choose Key
      C, Z, A, B     : Press Key
      +              : Close Keypad
      
    Classic controller/Pro:

      D-pad/Analog   : Choose Key           
      A, B           : Press Key
      +              : Close Keypad
                              
    GameCube controller:

      D-pad/Analog   : Choose Key
      A, B           : Press Key
      Start          : Close Keypad
            
### In-game (Standard):
  
    Wiimote:

      D-pad          : Move
      2              : Left Fire Button
      1              : Right Fire Button
      +              : Toggle Keypad
      Home           : Display WiiColEm menu (see above)
      
    Wiimote + Nunchuk:

      D-pad, Analog  : Move
      C              : Left Fire Button
      Z              : Right Fire Button
      +              : Toggle Keypad
      Home           : Display WiiColEm menu (see above)
      
    Classic controller/Pro:

      D-pad/Analog   : Move
      A              : Left Fire Button
      B              : Right Fire Button
      +              : Toggle Keypad
      Home           : Display WiiColEm menu (see above)
                              
    GameCube controller:

      D-pad/Analog   : Move
      A              : Left Fire Button
      B              : Right Fire Button
      Start          : Toggle Keypad
      Z              : Display WiiColEm menu (see above)

### In-game (Super action):
  
  Very few games use the "spinner" that is a part of the super action 
  controller. You can enable/disable the "spinner" via Cartridge Settings
  (see "Cartridge Settings" section, below). 
  
  By disabling the spinner, you have more options available for the move
  controls (Nunchuk analog, both analogs on the Classic and GameCube 
  controllers).
      
    Wiimote + Nunchuk:

      D-pad, Analog  : Move (if spinner disabled)
      D-pad          : Move (if spinner enabled)
      Analog         : Spinner (if enabled)
      C, 2           : Yellow Button
      Z, 1           : Orange Button
      A              : Blue Button
      B              : Purple Button
      +              : Toggle Keypad
      Home           : Display WiiColEm menu (see above)
      
    Classic controller/Pro:

      D-pad/Analog   : Move (if spinner disabled)
      Left Analog    : Move (if spinner enabled)
      Right Analog   : Spinner (if enabled)
      A, R Trigger   : Yellow Button
      B, L Trigger   : Orange Button
      X, ZR          : Blue Button
      Y, ZL          : Purple Button
      +              : Toggle Keypad
      Home           : Display WiiColEm menu (see above)
                              
    GameCube controller:

      D-pad/Analog   : Move (if spinner disabled)
      Analog         : Move (if spinner enabled)
      C Stick        : Spinner (if enabled)
      A, R Trigger   : Yellow Button
      B, L Trigger   : Orange Button
      X              : Blue Button
      Y              : Purple Button
      Start          : Toggle Keypad
      Z              : Display WiiColEm menu (see above)

### In-game (Driving/Tilt):
  
  In this mode, you steer by tilting the Wiimote (similar to Excite
  Truck/Bots). You can adjust the tilt sensitivity via Cartridge Settings
  (see "Cartridge Settings" section, below).
      
    Wiimote:

      Tilt   : Steer
      D-pad  : Shift
      2      : Gas
      1      : Brake
      +      : Toggle Keypad
      Home   : Display WiiColEm menu (see above)
      
### In-game (Driving/Analog):
  
  In this mode, you steer by using the analog controls (Nunchuk, Classic,
  GameCube). You can adjust the analog sensitivity via Cartridge Settings
  (see "Cartridge Settings" section, below).
      
    Wiimote + Nunchuk:

      Analog        : Steer
      D-pad         : Shift
      C, 2          : Gas
      Z, 1          : Brake
      +             : Toggle Keypad
      Home          : Display WiiColEm menu (see above)
      
    Classic controller/Pro:

      Right Analog  : Steer
      D-pad         : Shift
      A             : Gas
      B             : Brake
      +             : Toggle Keypad
      Home          : Display WiiColEm menu (see above)
                              
    GameCube controller:

      C Stick       : Steer
      D-pad         : Shift
      A             : Gas
      B             : Brake
      Start         : Toggle Keypad
      Z             : Display WiiColEm menu (see above)
      
### In-game (Roller):

  In this mode, the trackball motion is simulated via analog controls
  (Nunchuk, Classic, GameCube). You can adjust the analog sensitivity via
  Cartridge Settings (see "Cartridge Settings" section, below).
  
    Wiimote + Nunchuk:

      Analog       : Move
      C, 2         : Left Fire (2p)
      Z, 1         : Right Fire (2p)
      A            : Left Fire
      B            : Right Fire
      +            : Toggle Keypad
      Home         : Display WiiColEm menu (see above)
      
    Classic controller/Pro:

      Left Analog  : Move
      A            : Left Fire (2p)
      B            : Right Fire (2p)
      X            : Left Fire
      Y            : Right Fire
      +            : Toggle Keypad
      Home         : Display WiiColEm menu (see above)
                              
    GameCube controller:

      Analog       : Move
      A            : Left Fire (2p)
      B            : Right Fire (2p)
      X            : Left Fire
      Y            : Right Fire
      Start        : Toggle Keypad
      Z            : Display WiiColEm menu (see above)
      
### In-game (Aquattack):
  
  Aquattack has a very unique control scheme. It uses the keypad buttons to 
  fire in eight different directions. Essentially, it uses the "keypad" as a 
  "d-pad". While the keypad buttons have been mapped to the eight different 
  buttons on the Classic controller (and pro) it is cumbersome to use. In 
  addition to mapping these buttons, a special control scheme has been added
  to WiiColEm that allows for the use of the right analog stick on both the 
  GameCube and Classic controller/Pro for firing in the eight different 
  directions.
        
## Cartridge Settings

WiiColEm contains the ability to manage per-cartridge settings. The settings
that can be edited include:

  * Control settings
  * Advanced settings (Whether it requires Opcode memory expansion,
    keypad pause, etc.) 
    
### Control Settings

The following control options are available:

  * The controller type (standard, super action, driving/tilt, driving/analog, roller)
  * Wiimote orientation
  * Sensitivity (Roller and driving controllers)
  * ColecoVision keypad and controller button mappings 

  When mapping buttons, you map a ColecoVision keypad or controller button to
  one of the "button groups" listed below. This allows you to map a button
  once across the different Wii controllers.

  You can use the "(View as)" menu item to toggle and view how the 
  ColecoVision buttons are mapped to a particular Wii controller (Wiimote,
  Nunchuk, Classic/Pro, and GameCube).

    Button 1:                Button 2:                Button 3:
    ---------                ---------                ---------
      Wiimote   : 2            Wiimote   : 1            Wiimote   : A
      Nunchuk   : C            Nunchuk   : Z            Nunchuk   : (n/a)
      Classic   : A            Classic   : B            Classic   : X
      GameCube  : A            GameCube  : B            GameCube  : X

    Button 4:                Button 5:                Button 6:
    ---------                ---------                ---------
      Wiimote   : B            Wiimote   : (n/a)        Wiimote   : (n/a)
      Nunchuk   : (n/a)        Nunchuk   : (n/a)        Nunchuk   : (n/a)
      Classic   : X            Classic   : R Trigger    Classic   : L Trigger
      GameCube  : X            GameCube  : R Trigger    GameCube  : L Trigger

    Button 7:                Button 8:
    ---------                ---------
      Wiimote   : (n/a)        Wiimote   : (n/a)
      Nunchuk   : (n/a)        Nunchuk   : (n/a)
      Classic   : ZR           Classic   : ZL
      GameCube  : (n/a)        GameCube  : (n/a)
           
## WiiColEm crashes, code dumps, etc.

If you are having issues with WiiColEm, please let me know about it via one
of the following locations:

* http://www.wiibrew.org/wiki/Talk:WiiColEm
* http://www.twitchasylum.com/forum/viewtopic.php?t=1245

## Credits

* NeoRame          : Icon
* Pixelboy         : Source overlays
* James Carter     : Source overlays
* Murph74          : Testing (0.2) 
* Astroman         : Testing (0.2)
* Yurkie           : Testing (0.1)

## Special thanks

* Tantric          : Menu example code and SDL enhancements
* Marat Fayzullin  : Answering all of my questions
* Daniel Bienvenu  : Help resolving the Mode 2 issues
* Opcode           : Providing a ROM for testing the Opcode memory expansion
                     and MegaCart(R) support

## Change log

### 03/06/11 (0.2)
  - Cartridge-specific overlays
  - Default keypad (controller) image for cartridges without specific overlays
    - Ability to set the default keypad (controller) size
  - Ability to specify whether to use cartridge-specific overlays (global
      and per-cartridge).
  - GX based scaler
    - Ability to adjust screen size to any size/dimensions via the
        "Screen Size" option under "Display". If this is entered after 
        loading a cartridge, the last frame will be displayed to assist in 
        sizing.
  - USB support
  - SDHC support
  - Classic Controller Pro support
  - Support for ColecoVision MegaCart(R)
  - Fixed Lord of the Dungeon save/load state issues
  - Fixed Opcode memory expansion save/load state issues
  - Fixed save/load state bug causing the palette to not be restored 
      correctly (Aquattack, War Room)
  - Ability to set Wiimote orientation (sideways/upright) for navigating menus
  - Firing in Aquattack is now supported via the right analog stick (GameCube,
      Classic/Pro controllers).
  - Fixed bugs caused by pressing multiple keypad buttons simultaneously

### 01/26/10 (0.1)
  - Initial release
