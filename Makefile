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
#TARGET		:=	$(notdir $(CURDIR))
TARGET		:=  boot
BUILD		:=	build
SOURCES		:= \
    src src/wii \
    src/wii/common \
    src/wii/common/netprint \
    src/wii/common/pngu \
    src/wii/common/FreeTypeGX \
    src/ColEm \
    src/Z80 \
    src/EMULib \
    src/ColEm/Wii \
    src/EMULib/Wii 
DATA		:= \
    src/wii/res/fonts \
    src/wii/res/gfx
#DATA		:=	
INCLUDES	:= \
    src src/wii \
    src/wii/common \
    src/wii/common/netprint \
    src/wii/common/pngu \
    src/wii/common/FreeTypeGX \
    src/ColEm \
    src/Z80 \
    src/EMULib \
    src/ColEm/Wii \
    src/EMULib/Wii \
    thirdparty/freetype/include \
    thirdparty/sdl/SDL/include \
    thirdparty/sdl/SDL_ttf/include \
    thirdparty/sdl/SDL_image/include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS	=   -g -O1 -Wall $(MACHDEP) $(INCLUDE) -DMSB_FIRST -DCOLEM -DWII -DBPP8 \
            -DBPS16 -DWII_BIN2O -DMEGACART \
            -Wno-format-truncation \
            -Wno-format-overflow
# -DNO_AUDIO_PLAYBACK -DWII_NETTRACE
CXXFLAGS	=	$(CFLAGS)

LDFLAGS	=	-g $(MACHDEP) -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS    :=  -lSDL_ttf -lSDL -lSDL_image -lpng -lfreetype -lfat -lwiiuse \
            -lbte -logc -lz -lm -lwiikeyboard

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= \
    thirdparty/freetype/lib \
    thirdparty/sdl/SDL/lib \
    thirdparty/sdl/SDL_ttf/lib \
    thirdparty/sdl/SDL_image/lib

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
    SndSDL.c \
    WiiColem.c \
    pngu.c \
    net_print.c \
    wii_app.c \
    wii_config.c \
    wii_file_io.c \
    wii_freetype.c \
    wii_gx.c \
    wii_hash.c \
    wii_hw_buttons.c \
    wii_input.c \
    wii_main.c \
    wii_resize_screen.c \
    wii_sdl.c \
    wii_snapshot.c \
    wii_util.c \
    wii_video.c \
    wii_coleco.c \
    wii_coleco_config.c \
    wii_coleco_db.c \
    wii_coleco_emulation.c \
    wii_coleco_keypad.c \
    wii_coleco_menu.c \
    wii_coleco_sdl.c \
    wii_coleco_snapshot.c     
    
CPPFILES    := \
    FreeTypeGX.cpp \
    Metaphrasis.cpp

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
					-I$(DEVKITPRO)/portlibs/ppc/include

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
