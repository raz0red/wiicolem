#---------------------------------------------------------------------------#
#   __      __.__.___________        .__                                    #
#  /  \    /  \__|__\_   ___ \  ____ |  |   ____   _____                    #
#  \   \/\/   /  |  /    \  \/ /  _ \|  | _/ __ \ /     \                   #
#   \        /|  |  \     \___(  <_> )  |_\  ___/|  Y Y  \                  #
#    \__/\  / |__|__|\______  /\____/|____/\___  >__|_|  /                  #
#         \/                \/                 \/      \/                   #
#     WiiColem by raz0red                                                   #
#     Port of the ColEm emulator by Marat Fayzullin                         #
#                                                                           #
#     [github.com/raz0red/wiicolem]                                         #
#                                                                           #
#---------------------------------------------------------------------------#
#                                                                           #
#  Copyright (C) 2019 raz0red                                               #
#                                                                           #
#  The license for ColEm as indicated by Marat Fayzullin, the author of     #
#  ColEm is detailed below:                                                 #
#                                                                           #
#  ColEm sources are available under three conditions:                      #
#                                                                           #
#  1) You are not using them for a commercial project.                      #
#  2) You provide a proper reference to Marat Fayzullin as the author of    #
#     the original source code.                                             #
#  3) You provide a link to http://fms.komkon.org/ColEm/                    #
#                                                                           #
#---------------------------------------------------------------------------#

#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC)
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=  boot
BUILD		:=	build
SOURCES		:= \
    src/wii \
    src/ColEm \
    src/Z80 \
    src/EMULib \
    src/ColEm/Wii \
    src/EMULib/Wii 
DATA		:= \
    res/fonts \
    res/gfx
INCLUDES	:= \
    wii-emucommon/include \
    wii-emucommon/netprint/include \
    wii-emucommon/pngu/include \
    wii-emucommon/FreeTypeGX/include \
    wii-emucommon/i18n/include \
    wii-emucommon/sdl/SDL/include \
    wii-emucommon/sdl/SDL_ttf/include \
    wii-emucommon/sdl/SDL_image/include \
    src/wii \
    src/ColEm \
    src/Z80 \
    src/EMULib \
    src/ColEm/Wii \
    src/EMULib/Wii

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS	=   -g -O1 -Wall $(MACHDEP) $(INCLUDE) -DMSB_FIRST -DCOLEM -DWII -DBPP8 \
            -DBPS16 -DWII_BIN2O -DMEGACART -DZLIB \
            -Wno-format-truncation \
            -Wno-format-overflow -DENABLE_VSYNC -DENABLE_SMB
# -DNO_AUDIO_PLAYBACK -DWII_NETTRACE
CXXFLAGS	=	$(CFLAGS)

LDFLAGS	=	-g $(MACHDEP) -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS    :=  -lSDL -lemucommon -ltinysmb -lSDL_ttf -lSDL_image -lpng -lfreetype \
            -lfat -lwiiuse -lbte -logc -lz -lbz2 -lm -lwiikeyboard

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= \
    wii-emucommon/ \
    wii-emucommon/sdl/SDL/lib \
    wii-emucommon/sdl/SDL_ttf/lib \
    wii-emucommon/sdl/SDL_image/lib

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES      := \
    Coleco.c \
    AdamNet.c \
    AY8910.c \
    Hunt.c \
    C24XX.c \
    CRC32.c \
    SN76489.c \
    TMS9918.c \
    DRV9918.c \
    Z80.c \
    Sound.c \
    SndSDL.c

CPPFILES    := \
    WiiColem.cpp \
    wii_coleco.cpp \
    wii_coleco_config.cpp \
    wii_coleco_db.cpp \
    wii_coleco_emulation.cpp \
    wii_coleco_keypad.cpp \
    wii_coleco_menu.cpp \
    wii_coleco_sdl.cpp \
    wii_coleco_snapshot.cpp

sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif
					
export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
					

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC) \
					-I$(DEVKITPRO)/portlibs/ppc/include \
					-I$(DEVKITPRO)/portlibs/ppc/include/freetype2

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(CURDIR)/$(dir)) \
					-L$(LIBOGC_LIB) \
					-L$(DEVKITPRO)/portlibs/ppc/lib

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol

#---------------------------------------------------------------------------------
run:
	psoload $(TARGET).dol

#---------------------------------------------------------------------------------
reload:
	psoload -r $(TARGET).dol

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
# This rule links in binary data with the .jpg extension
#---------------------------------------------------------------------------------
%.jpg.o	:	%.jpg
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
# This rule links in binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

%.mod.o	:	%.mod
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

%.ttf.o	:	%.ttf
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

%.png.o	:	%.png
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

