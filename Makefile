
CC=g++
CFLAGS=-c -Wall
UNAME := $(shell uname)
LIBS=-lGL -lglut -lGLU
EXECUTABLE=prak

# Apple spezifische Pfade
ifeq ($(UNAME), Darwin)
  INCP=-I/usr/X11R6/include/ -I/opt/local/include
  LIBP=-L/opt/local/lib -L/usr/X11R6/lib
  LIBS=-lglut -lglu -lgl
endif

all: wavefront.o help.o Template_PrakCG.o
	$(CC) $(INCP) $(LIBP) wavefront.o help.o Template_PrakCG.o $(LIBS) -o $(EXECUTABLE)

wavefront: wavefront.cpp
	$(CC) $(CFLAGS) wavefront.cpp -o wavefront.o

help: help.cpp
	$(CC) $(CFLAGS) help.cpp -o help.o
	
Template_PrakCG: Template_PrakCG.cpp
	$(CC) $(CFLAGS) Template_PrakCG.cpp -o Template_PrakCG.o

clean:
	rm *.o $(EXECUTABLE)
