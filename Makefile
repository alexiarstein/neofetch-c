CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -Iinclude
SRCDIR = src
OBJDIR = build
INCDIR = include
TARGET = neofetch

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(OBJDIR)
	$(CC) $(OBJECTS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: debug
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)