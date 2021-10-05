CC      := gcc
LD      := gcc
CFLAGS  := -O2 -g -Wall
LDFLAGS := 

# targets and sources
BIN := hammer
SRC := src/main.c src/bind.c src/cli.c src/cmd.c src/ctx.c src/map.c src/ns.c src/par.c src/rd.c src/rule.c src/target.c src/back/linux.c

# built from sources
OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.d)

# all
all: hammer.sh $(BIN)

$(BIN): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

hammer.sh: hammer.c hammer.src make.src
	rm -f $@
	cat hammer.src >> $@
	echo "##csrc##" >> $@
	cat hammer.c >> $@
	echo "##make##" >> $@
	cat make.src >> $@
	chmod 755 $@
	
hammer.c: src/inc.h $(SRC) Makefile
	cat src/inc.h $(SRC) | sed '/#pragma once/d' | sed '/#include "\(\.\.\/\)*inc.h"/d' > $@

#run: all
	@#cd test ; ./test.sh
	#cd uhg ; ./go.sh

# clean everything up
clean:
	rm -f $(OBJ) $(DEP) $(BIN)

# template rules
%.o: %.c Makefile
	$(CC) -c $< -o $@ $(CFLAGS) -MD -MP -I.

# dependencies
-include $(DEP)

# phony rules
.PHONY: all clean

# icnlude user rules too
-include make.user
