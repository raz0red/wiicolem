!include ..\..\EMULib\Rules.MSDOS

DEFINES	= $(DEFINES) -DCOLEM -DZLIB -DDEBUG -DCONDEBUG -DBPP16
CFLAGS	= $(CFLAGS) -I$(LIBZ80)
OBJECTS	= $(OBJECTS) $(Z80) $(SN76489) $(TMS9918) \
	  ..\Coleco.obj ..\ColEm.obj ..\Menu.obj MSDOS.obj

all:	ColEm.exe
	call sc/b ColEm.exe
#	call upx ColEm.exe

clean:
        del $(OBJECTS)
	del *.err
	del *.tmp
	del *.bak

ColEm.exe: Makefile $(OBJECTS)
	wcl386 -fe=$@ -l=dos4g -"library zlib" $(OBJECTS)
