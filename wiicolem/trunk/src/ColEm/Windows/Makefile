!include ..\..\EMULib\Rules.Windows

DEFINES	= $(DEFINES) -DCOLEM -DZLIB -DDEBUG -DCONDEBUG -DBPP16
CFLAGS	= $(CFLAGS) -I$(LIBZ80)
OBJECTS	= $(OBJECTS) $(Z80) $(SN76489) $(TMS9918) \
	  ..\Coleco.obj ..\Menu.obj Windows.obj

all:	WColEm.exe
	call upx WColEm.exe

clean:
        del $(OBJECTS)
	del *.err
	del *.tmp
	del *.bak
	del *.res

WColEm.exe: Makefile ColEm.rc ColEm.rh $(OBJECTS)
	wcl386 -fe=$@ -l=win95 -"library zlib,ddraw,winmm,ws2_32" $(OBJECTS)
	wrc/bt=nt -I$(EMULIB)\Windows ColEm.rc $@
