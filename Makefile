CC := gcc
PKG_CONFIG := pkg-config

PACKAGES := glib-2.0 gtk+-3.0 json-glib-1.0

CFLAGS = -Wall -g
INCLUDES = `$(PKG_CONFIG) --cflags $(PACKAGES)`
LIBS += `$(PKG_CONFIG) --libs $(PACKAGES)`

PREFIX := /usr

ng_SRC := $(wildcard *.c)
ng_OBJ := $(ng_SRC:.c=.o)
ng_HEADERS := $(wildcard *.h)

all: nonogram-game

nonogram-game: $(ng_OBJ)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c $(ng_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

install: nonogram-game
	install nonogram-game $(PREFIX)/bin

clean:
	rm -f nonogram-game $(ng_OBJ)

.PHONY: all clean install
