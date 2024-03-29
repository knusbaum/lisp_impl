CFLAGS=-ggdb -O3 -Wall -Wextra -Werror
EXECUTABLE=lisp
LDFLAGS=-lpthread

OBJECTS=main.o \
		lexer.o \
		context.o \
		map.o \
		parser.o \
		object.o \
		lstring.o \
		threaded_vm.o \
		compiler.o \
		gc.o

all: lisp

lisp: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

clean:
	-@rm -Rf *~ *.o

nuke: clean
	-@rm -Rf $(EXECUTABLE)
