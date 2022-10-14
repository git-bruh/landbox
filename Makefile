.POSIX:

.PHONY: compile_commands.json

INCLUDES = -Iinclude
XCFLAGS = $(CFLAGS) $(CPPFLAGS) $(INCLUDES) \
	-std=c11 \
	-Wall -Wextra -Wshadow -Wnull-dereference -Wformat=2 -Wcast-qual \
	-Wconversion -Wpointer-arith -Wunused-macros -Wredundant-decls \
	-Wwrite-strings \
	-Werror=int-conversion -Werror=implicit-function-declaration \
	-Werror=incompatible-pointer-types
XLDFLAGS = $(LDFLAGS)

BIN = landbox
LIB = liblandbox.a

BIN_OBJ = \
	src/landbox.o

LIB_OBJ = \
	lib/landbox.o \
	lib/util.o

all: $(BIN) $(LIB)

format:
	clang-format -i ./*/*.[hc]

tidy:
	clang-tidy ./*/*.[hc] -- $(XCFLAGS)

DEP = $(BIN_OBJ:.o=.d) $(LIB_OBJ:.o=.d)
-include $(DEP)

.c.o:
	$(CC) $(XCFLAGS) -MMD -c -o $@ $<

$(LIB): $(LIB_OBJ)
	$(AR) -rc $@ $(LIB_OBJ)

$(BIN): $(BIN_OBJ) $(LIB)
	$(CC) $(XCFLAGS) -o $@ $(BIN_OBJ) $(LIB)

compile_commands.json:
	CC=cc compiledb -n make

install: all
	mkdir -p $(DESTDIR)/$(PREFIX)/bin $(DESTDIR)/$(PREFIX)/lib \
		$(DESTDIR)/$(PREFIX)/include
	cp -f $(BIN) $(DESTDIR)/$(PREFIX)/bin
	cp -f $(LIB) $(DESTDIR)/$(PREFIX)/lib
	cp -f include/landbox.h $(DESTDIR)/$(PREFIX)/include

clean:
	$(RM) -f $(BIN) $(LIB) $(BIN_OBJ) $(LIB_OBJ) $(DEP) compile_commands.json
