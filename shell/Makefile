CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
	-Wall -Wextra -Werror -Wno-unused-parameter -fno-asm
	
INCDIR = include
SRCDIR = src

SRC = $(SRCDIR)/main.c \
      $(SRCDIR)/prompt.c \
      $(SRCDIR)/input.c \
      $(SRCDIR)/parser.c \
	  $(SRCDIR)/builtins.c

all: shell.out

shell.out: $(SRC)
	$(CC) $(CFLAGS) -I$(INCDIR) -o shell.out $(SRC)

clean:
	rm -f shell.out

.PHONY: all clean
