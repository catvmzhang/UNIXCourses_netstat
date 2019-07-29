CC = gcc
PROG = hw1
CFLAG = -g

all: $(PROG)

debug: CFLAG += -DDEBUG
debug: $(PROG)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAG)

hw1: hw1.c def.o def.h
	$(CC) hw1.c -o $@ def.o $(CFLAG)
	sudo chown root hw1
	sudo chmod 4775 hw1
