#

PROJECT = test
SOURCES = test.cpp
OBJECTS = test.o
LIBS    = -lX11 -lm -lGL
CC = g++

$(PROJECT): $(OBJECTS)
	$(CC) -O2 -g $(OBJECTS) $(LIBS) -o $(PROJECT)
	rm -f *.o;
	@echo Compilation Complete
	@echo

$(OBJECTS): $(SOURCES)
	@echo Deleting up $(OBJECTS) $(PROJECT)
	@echo Compiling Sources
	$(CC) -O2 -Wall -ansi -pedantic -g -c $(SOURCES)
