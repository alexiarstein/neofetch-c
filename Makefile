# neofetch-c - A fast system information tool written in C
# Copyright (C) 2026 Alexia Michelle <https://github.com/alexiarstein/neofetch-c>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -Iinclude
HARDENING_CFLAGS = -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Werror=format-security -fPIE
LDFLAGS = -Wl,-z,relro -Wl,-z,now -pie
SRCDIR = src
OBJDIR = build
INCDIR = include
TARGET = neofetch

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(OBJDIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(HARDENING_CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: debug
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)