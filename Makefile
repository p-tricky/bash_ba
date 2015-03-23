CC=clang
CFLAGS=-g -O0 -Wall -std=c99
#add headers to DEPS, otherwise make wont detect changes and recompile
#when headers change
DEPS =

# DupeExecDemo
DED_SRC = DupExecDemo.c makeargv.c error.c
DED_OBJ = DupExecDemo.o makeargv.o error.o
DED_EXEC = DupExecDemo 

# PipeExecDemo
PED_SRC = PipeExecDemo.c error.c
PED_OBJ = PipeExecDemo.o error.o
PED_EXEC = PipeExecDemo

# Up
UP_SRC = Up.c error.c
UP_OBJ = Up.o error.o
UP_EXEC = Up

# Shell
SH_SRC = bash_ba.c makeargv.c error.c
SH_OBJ = bash_ba.o makeargv.o error.o
SH_EXEC = bash_ba

#.PHONY declaration just says that 'all' isn't a file name.
.PHONY: all
all: $(DED_EXEC) $(PED_EXEC) $(UP_EXEC) $(SH_EXEC)

#compile all .o into executable
#%.o == all .o files
#$@ = left side of :
#$< = right side of :
%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

$(DED_EXEC): $(DED_OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

$(PED_EXEC): $(PED_OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

$(UP_EXEC): $(UP_OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

$(SH_EXEC): $(SH_OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

#https://www.gnu.org/software/make/manual/html_node/Cleanup.html#Cleanup
.PHONY: clean
clean:
	find . -maxdepth 1 -type f -perm +111 ! -name "*.sh" -exec rm {} +
	find . -maxdepth 1 -type f -name "*.o" -print -exec rm {} +

