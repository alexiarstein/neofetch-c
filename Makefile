# neofetch-c - A fast system information tool written in C
# Copyright (C) 2026 Alexia Michelle <https://github.com/alexiarstein/neofetch-c>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

CC = gcc

override CFLAGS += -Wall -Wextra -std=c11 -fstack-protector-strong -fPIE
override CPPFLAGS += -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -Iinclude
override LDFLAGS += -Wl,-z,relro -Wl,-z,now -pie
override CFLAGS += -Wformat -Werror=format-security
SRCDIR = src
OBJDIR = build
INCDIR = include
TARGET = neofetch

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(OBJDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	install -D -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	install -d -m 755 /usr/share/neofetch/ascii
	install -m 644 ascii/*.ascii /usr/share/neofetch/ascii/

uninstall:
	rm -f /usr/local/bin/$(TARGET)
	rm -rf /usr/share/neofetch

.PHONY: debug
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)