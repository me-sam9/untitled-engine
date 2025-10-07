# ue - untitled engine
# See LICENSE file for copyright and license details.

# customize below to fit your system

BIN = ue
FULLNAME = untitled engine
WIDTH = 800
HEIGHT = 600
CC = tcc
INCS = -Iinclude
LIBS = -lglfw -lGLEW -lsoil2 -lm -lGL

CFLAGS = -pedantic -Wall -std=c99 -MD $(INCS) \
	 -DBIN=\"$(BIN)\" \
	 -DFULLNAME=\""$(FULLNAME)"\" \
	 -DWIDTH=$(WIDTH) \
	 -DHEIGHT=$(HEIGHT)
LDFLAGS = $(LIBS)

SRC = ue.c $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

-include ${DEP}

run: all
	@./$(BIN)

mango: all
	@mangohud ./$(BIN)

clean:
	rm -f $(BIN) $(OBJ) $(DEP)

.PHONY: all run clean
