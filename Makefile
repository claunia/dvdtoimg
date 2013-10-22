OBJS = dvdtoimg.o md5.o sha1.o crc32.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -O0 -W -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)
LIBS = -lm -lcdio

dvdtoimg : $(OBJS)
	$(CC) $(LFLAGS) $(LIBS) $(OBJS) -o dvdtoimg

dvdtoimg.o : dvdtoimg.cpp
	$(CC) $(CFLAGS) dvdtoimg.cpp

md5.o : md5.c md5.h
	$(CC) $(CFLAGS) md5.c

sha1.o : sha1.c sha1.h
	$(CC) $(CFLAGS) sha1.c

crc32.o : crc32.c crc32.h
	$(CC) $(CFLAGS) crc32.c

clean:
	\rm *.o dvdtoimg

tar: $(OBJS)
	tar jcfv dvdtoimg.tbz readme.txt Makefile dvdtoimg
