CFLAGS=-ggdb -O0 -Wall -Wextra -Werror
EXECUTABLE=lisp

OBJECTS=main.o \
		lexer.o \
		lisp.o \
		map.o \
		parser.o \
		object.o \
		lstring.o

all: lisp

lisp: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE)

clean:
	-@rm -Rf *~ *.o

nuke: clean
	-@rm -Rf $(EXECUTABLE)
