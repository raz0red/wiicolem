include ../../EMULib/Rules.Maemo

DEFINES+= -DCOLEM -DZLIB -DDEBUG -DCONDEBUG
CFLAGS += -I$(LIBZ80)

# All object files (deleting them on cleanup)
OBJECTS+= $(Z80) $(SN76489) $(TMS9918) \
	  ../Coleco.o ../Menu.o Maemo.o

all:    colem

colem:	Makefile $(OBJECTS)
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)
