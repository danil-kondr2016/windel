CC=gcc
WINDRES=windres
CFLAGS=-g -O2 -D_UNICODE -DUNICODE
LDFLAGS=-municode -mwindows
LDLIBS=-lgdi32 \
	-luser32 \
	-lkernel32 \
	-lcomctl32

WinDel.exe: main.o resource.o errmsg.o windel.o 
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

resource.o: resource.rc
	$(WINDRES) -c65001 -J rc -O coff $< $@

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm *.o *.exe
