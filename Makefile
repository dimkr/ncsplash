# compiler stuff
CC ?= cc
CFLAGS ?= -Wall
LIBS = -lncurses

# installation stuff
PREFIX ?= /usr
BIN_DIR ?= $(PREFIX)/bin
MAN_DIR ?= $(PREFIX)/share/man

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

ncsplash: ncsplash.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -v -f ncsplash *.o

install: ncsplash
	install -v -m755 -D ncsplash $(BIN_DIR)/ncsplash
	install -v -m644 -D ncsplash.1 $(MAN_DIR)/man1/ncsplash.1

uninstall:
	rm -v -f $(BIN_DIR)/ncsplash $(MAN_DIR)/man1/ncsplash.1