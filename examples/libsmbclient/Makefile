#
CC = gcc

SAMBA_INCL = ../../source/include
EXTLIB_INCL = -I/usr/include/gtk-1.2 \
	      -I/usr/include/glib-1.2 \
	      -I/usr/lib/glib/include


DEFS = -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
CFLAGS = -O0 -g -I$(SAMBA_INCL) $(EXTLIB_INCL) $(DEFS)

LDFLAGS = -L/usr/local/samba/lib
#LIBSMBCLIENT = /usr/local/samba/lib/libsmbclient.so
LIBSMBCLIENT = ../../source/bin/libsmbclient.a -ldl -lresolv

TESTS=	testsmbc \
	tree \
	testacl \
	testbrowse \
	testbrowse2 \
	teststat \
	teststat2 \
	testchmod \
	testutime \
	testread

all:	$(TESTS) smbsh

testsmbc: testsmbc.o 
	@echo Linking testsmbc
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBSMBCLIENT)

tree: tree.o
	@echo Linking tree
	$(CC) `gtk-config --cflags` $(CFLAGS) $(LDFLAGS) -o $@ $< `gtk-config --libs` $(LIBSMBCLIENT)

testacl: testacl.o
	@echo Linking testacl
	$(CC) `gtk-config --cflags` $(CFLAGS) $(LDFLAGS) -o $@ $< `gtk-config --libs` $(LIBSMBCLIENT) -lpopt

testbrowse: testbrowse.o
	@echo Linking testbrowse
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBSMBCLIENT) -lpopt

testbrowse2: testbrowse2.o
	@echo Linking testbrowse2
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBSMBCLIENT) -lpopt

teststat: teststat.o
	@echo Linking teststat
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBSMBCLIENT) -lpopt

teststat2: teststat2.o
	@echo Linking teststat2
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBSMBCLIENT) -lpopt

testchmod: testchmod.o
	@echo Linking testchmod
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBSMBCLIENT) -lpopt

testutime: testutime.o
	@echo Linking testutime
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBSMBCLIENT) -lpopt

testread: testread.o
	@echo Linking testread
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBSMBCLIENT) -lpopt

smbsh:
	make -C smbwrapper

clean:
	@rm -f *.o *~ $(TESTS)
	@make -C smbwrapper clean
