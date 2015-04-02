CC=gcc
CFLAGS=-g -O0 -Wall -std=c99
#add headers to DEPS, otherwise make wont detect changes and recompile
#when headers change
DEPS =

# Shell
SH_SRC = bash_ba.c makeargv.c error.c path_alloc.c
SH_OBJ = bash_ba.o makeargv.o error.o path_alloc.o
SH_EXEC = bash_ba

#.PHONY declaration just says that 'all' isn't a file name.
.PHONY: all
all: $(SH_EXEC)

#compile all .o into executable
#%.o == all .o files
#$@ = left side of :
#$< = right side of :
%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

$(SH_EXEC): $(SH_OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

#https://www.gnu.org/software/make/manual/html_node/Cleanup.html#Cleanup
.PHONY: clean
clean:
	find . -maxdepth 1 -type f -perm +111 ! -name "*.sh" -exec rm {} +
	find . -maxdepth 1 -type f -name "*.o" -print -exec rm {} +

