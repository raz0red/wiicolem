include ../../EMULib/Rules.Unix

# Depending on your CPU endianess, use either -DLSB_FIRST or -DMSB_FIRST.
# Depending on your X11 display mode, use -DBPP8, -DBPP16, or -DBPP32.
DEFINES+= -DCOLEM -DLSB_FIRST -DZLIB -DDEBUG -DCONDEBUG -DBPP16
CFLAGS += -I$(LIBZ80)
OBJECTS+= $(Z80) $(SN76489) $(TMS9918) \
	  ../Coleco.o ../ColEm.o ../Menu.o Unix.o

all:    colem

colem:	Makefile $(OBJECTS)
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)
