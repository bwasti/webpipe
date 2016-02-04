CC=gcc
OBJDIR=objs
SRCDIR=.

SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

CFLAGS+=$(ENV_CFLAGS)
LDFLAGS+=$(ENV_LDFLAGS)
CFLAGS+=-I/usr/include
ifndef STATIC_LIBWEBSOCKETS
	LDFLAGS+=-lwebsockets
endif
LDFLAGS+=-lpthread

TARGET=webpipe

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJS): | $(OBJDIR)
$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/$(TARGET)
