OBJ=common.o vars.o iplist.o sockinfo.o actions.o exit.o file_shred.o hooks.o dl_hooks.o exec_hooks.o time_hooks.o file_hooks.o filesys_hooks.o socket_hooks.o config.o net.o socks.o 
CFLAGS=-g -fPIC -g -O2 
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"enhancer\" -DPACKAGE_TARNAME=\"enhancer\" -DPACKAGE_VERSION=\"1.6\" -DPACKAGE_STRING=\"enhancer\ 1.6\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -Drestrict=__restrict__ -DHAVE_LIBC=1 -DHAVE_UNSHARE=1 -DGETTIMEOFDAY_TZVOID=1
CC=gcc
VERSION=1.6


all: enhancer.so

enhancer.so: $(OBJ)
	$(CC) $(FLAGS) -shared -o enhancer.so $(OBJ) -ldl

common.o: common.c common.h
	$(CC) $(FLAGS) -c common.c

vars.o: vars.c vars.h
	$(CC) $(FLAGS) -c vars.c

iplist.o: iplist.c iplist.h
	$(CC) $(FLAGS) -c iplist.c

sockinfo.o: sockinfo.c sockinfo.h
	$(CC) $(FLAGS) -c sockinfo.c

actions.o: actions.c actions.h common.h
	$(CC) $(FLAGS) -c actions.c

exit.o: exit.c exit.h common.h
	$(CC) $(FLAGS) -c exit.c

file_shred.o: file_shred.c file_shred.h common.h
	$(CC) $(FLAGS) -c file_shred.c

config.o: config.c config.h common.h
	$(CC) $(FLAGS) -c config.c

hooks.o: hooks.c hooks.h common.h
	$(CC) $(FLAGS) -c hooks.c

#use CFLAGS no FLAGS for file_hooks, due to OFFSET_BITS_64 causing issues
file_hooks.o: file_hooks.c file_hooks.h common.h
	$(CC) $(CFLAGS) -c file_hooks.c

filesys_hooks.o: filesys_hooks.c filesys_hooks.h common.h
	$(CC) $(FLAGS) -c filesys_hooks.c

dl_hooks.o: dl_hooks.c dl_hooks.h common.h
	$(CC) $(FLAGS) -c dl_hooks.c

exec_hooks.o: exec_hooks.c exec_hooks.h common.h
	$(CC) $(FLAGS) -c exec_hooks.c

time_hooks.o: time_hooks.c time_hooks.h common.h
	$(CC) $(FLAGS) -c time_hooks.c

socket_hooks.o: socket_hooks.c socket_hooks.h common.h
	$(CC) $(FLAGS) -c socket_hooks.c

x11_hooks.o: x11_hooks.c x11_hooks.h common.h
	$(CC) $(FLAGS) -c x11_hooks.c

net.o: net.c net.h common.h
	$(CC) $(FLAGS) -c net.c

socks.o: socks.c socks.h common.h
	$(CC) $(FLAGS) -c socks.c



clean:
	rm -f *.o enhancer.so config.log

install:
	cp enhancer.so /usr/local/lib

test:
	echo "no tests"
