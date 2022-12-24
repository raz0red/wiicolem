#--param large-function-growth=800 \
#--param inline-unit-growth=200 \

MAKEFLAGS += --no-builtin-rules
MAKE = /usr/bin/make
CFILES := \
    ./src/Colem/Coleco.c \
    ./src/Colem/AdamNet.c \
    ./src/EMULib/AY8910.c \
    ./src/EMULib/Hunt.c \
    ./src/EMULib/C24XX.c \
    ./src/EMULib/CRC32.c \
	./src/EMULib/FDIDisk.c \
    ./src/EMULib/SN76489.c \
    ./src/EMULib/TMS9918.c \
    ./src/EMULib/DRV9918.c \
	./src/EMULib/Sound.c \
	./src/EMULib/Wii/SndEmscripten.c \
    ./src/Z80/Z80.c

#WiiColem.cpp
#SndSDL.c

CPPFILES := ./src/Colem/Wii/Emscripten.cpp

FILES := $(patsubst %.c,%.o,$(CFILES)) $(patsubst %.cpp,%.o,$(CPPFILES))

FLAGS := \
	-O3 \
    $(CFLAGS) \
    -I./src/ColEm \
	-I./src/ColEm/Wii \
    -I./src/Z80 \
    -I./src/EMULib \
	-I./src/EMULib/Wii \
    -DWRC -DWII -DCOLEM -DBPS16 -DBPP32 -DMEGACART -DLSB_FIRST -DSLOW_MELODIC_AUDIO -DWAVE_INTERPOLATION \
  	-Winline \
  	-fomit-frame-pointer \
	-fno-strict-overflow \
	-fsigned-char \
  	-Wno-strict-aliasing \
  	-Wno-narrowing \
    -flto

#-g -O1 -Wall $(MACHDEP) $(INCLUDE) -DMSB_FIRST -DCOLEM -DWII -DBPP8 \
#            -DBPS16 -DWII_BIN2O -DMEGACART -DZLIB \
#             -DENABLE_VSYNC -DENABLE_SMB

LINK_FLAGS := \
    -O3 \
	-lz \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="'colem'" \
    -s TOTAL_MEMORY=67108864 \
    -s ALLOW_MEMORY_GROWTH=1 \
	-s ASSERTIONS=0 \
	-s EXIT_RUNTIME=0 \
	-s EXPORTED_RUNTIME_METHODS="['FS', 'cwrap']" \
    -s EXPORTED_FUNCTIONS="['_EmStart', '_EmStep']" \
	-s INVOKE_RUN=0 \
    -flto

all: colem

colem:
	@echo colem
	$(MAKE) colem.js

colem.js: $(FILES)
	emcc -o $@ $(FILES) $(LINK_FLAGS)

%.o : %.cpp
	emcc -c $< -o $@ \
	$(FLAGS)

%.o : %.c
	emcc -c $< -o $@ \
	$(FLAGS)

clean:
	@echo "Cleaning"
	@echo $(FILES)
	rm -fr *.o */*.o */*/*.o */*/*/*.o */*/*/*/*.o
