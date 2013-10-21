OBJS = dvdtoimg.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -O0 -W -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)
LIBS = -lm -lcdio

dvdtoimg : $(OBJS)
	$(CC) $(LFLAGS) $(LIBS) $(OBJS) -o dvdtoimg

dvdtoimg.o : dvdtoimg.cpp
	$(CC) $(CFLAGS) dvdtoimg.cpp

clean:
	\rm *.o dvdtoimg

tar: $(OBJS)
	tar jcfv dvdtoimg.tbz readme.txt Makefile dvdtoimg
