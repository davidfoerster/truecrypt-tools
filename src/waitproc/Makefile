APPNAME = waitproc

CC = gcc
CPPFLAGS += -pipe -DNDEBUG
CFLAGS += -std=gnu99 -O1 -g0 -Wall -Wextra -Wconversion
LDFLAGS += -Wl,--as-needed -s

$(APPNAME): *.c *.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o "$@" $(filter %.c, $^)

clean:
	rm -f -- $(APPNAME)

.PHONY: clean
