OBJ=common.o vars.o iplist.o sockinfo.o actions.o exit.o hooks.o exec_hooks.o time_hooks.o file_hooks.o socket_hooks.o config.o net.o socks.o 
FLAGS=-g -fPIC -g -O2
CC=gcc

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

config.o: config.c config.h common.h
	$(CC) $(FLAGS) -c config.c

hooks.o: hooks.c hooks.h common.h
	$(CC) $(FLAGS) -c hooks.c

exec_hooks.o: exec_hooks.c exec_hooks.h common.h
	$(CC) $(FLAGS) -c exec_hooks.c

time_hooks.o: time_hooks.c time_hooks.h common.h
	$(CC) $(FLAGS) -c time_hooks.c

file_hooks.o: file_hooks.c file_hooks.h common.h
	$(CC) $(FLAGS) -c file_hooks.c

socket_hooks.o: socket_hooks.c socket_hooks.h common.h
	$(CC) $(FLAGS) -c socket_hooks.c

x11_hooks.o: x11_hooks.c x11_hooks.h common.h
	$(CC) $(FLAGS) -c x11_hooks.c

net.o: net.c net.h common.h
	$(CC) $(FLAGS) -c net.c

socks.o: socks.c socks.h common.h
	$(CC) $(FLAGS) -c socks.c



clean:
	rm -f *.o

install:
	cp enhancer.so /usr/local/lib

test:
	echo "no tests"
