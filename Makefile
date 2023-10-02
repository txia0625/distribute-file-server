LIB_PATH := $(shell pwd)
server: server.c udp.c libmfs.so
	gcc -o server server.c udp.c
libmfs.so: mfs.c udp.c server
	gcc -o libmfs.so -fpic -shared mfs.c udp.c udp.h ufs.h


