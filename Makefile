
# Folders 
OBJDIR = obj
BINDIR = bin
SRCDIR = src
INCDIR = include

# Compiler
CC = gcc
CFLAGS = -pthread -g -Wall -Werror -I $(INCDIR)

# Files
SOURCES := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(INCDIR)/*.h
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o) 
DEP = $(OBJECTS:%.o=%.d)

all: bin/tracker 

bin/tracker: obj/tracker.o obj/network.o obj/debug.o
	@echo "\n-----------------> Linking ... "
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@

# Dependencies to the headers are
# covered by this include
-include $(DEP) 		

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	@echo "\n-----------------> Compiling $@ ... "
	$(CC) $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -f bin/tracker $(OBJECTS)

