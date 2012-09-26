CC = gcc

SRC_DIR = SRC/base
INC_DIR = $(SRC_DIR)/include
OBJ_DIR = OBJ
LIB_DIR = SRC/libs

LIB = -lX11 -lm
LIB_PATH = -L/usr/lib/X11
X11_INCLUDE = /usr/include/X11

_SOURCES = main.c parse_input.c util.c
SOURCES = $(patsubst %,$(INC_DIR)/%,$(_SOURCES)
HEADERS = $(wildcard $(INC_DIR)/*.h) $(wildcard $(LIB_DIR)/easygl/*.h)

OBJS_EASYGL = $(patsubst $(LIB_DIR)/easygl/%.c, $(OBJ_DIR)/%.o,$(wildcard $(LIB_DIR)/easygl/*c))
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*c))

DEBUG_FLAGS = -g
WARN_FLAGS = -Wall
OPT_FLAGS = 
INC_FLAGS = -I$(INC_DIR) -I$(LIB_DIR)/easygl -I$(X11_INCLUDE)

CFLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(INC_FLAGS)

EXEC = kroute

all: $(EXEC)


$(EXEC): $(OBJS) $(OBJS_EASYGL)
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PATH) $(LIB)

$(OBJS): $(OBJ_DIR)/%.o:$(SRC_DIR)/%.c $(HEADERS) $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS_EASYGL): $(OBJ_DIR)/%.o:$(LIB_DIR)/easygl/%.c $(HEADERS) $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

.PHONY: clean

clean:
	rm -fv $(OBJ_DIR)/*.o
	rm -fv $(EXEC)
