CC = g++

SRC_DIR = SRC/base
INC_DIR = $(SRC_DIR)/include
OBJ_DIR = OBJ
LIB_DIR = SRC/libs

LIB = -lX11 -lm -lumfpack -lamd -lsuitesparseconfig -lrt -lblas
LIB_PATH = -L/usr/lib/X11 -L$(LIB_DIR)/UMFPACK/Lib -L$(LIB_DIR)/AMD/Lib -L$(LIB_DIR)/SuiteSparse_config
X11_INCLUDE = /usr/include/X11
UMFPACK_INCLUDE = -I$(LIB_DIR)/UMFPACK/Include -I$(LIB_DIR)/SuiteSparse_config -I$(LIB_DIR)/AMD/Include

HEADERS_EASYGL = $(wildcard $(LIB_DIR)/easygl/*.h)
HEADERS_UMFPACK = $(wildcard $(LIB_DIR)/UMFPACK/Include/*.h)
HEADERS = $(wildcard $(INC_DIR)/*.h) $(HEADERS_EASYGL) $(HEADERS_UMFPACK)

OBJS_EASYGL = $(patsubst $(LIB_DIR)/easygl/%.c, $(OBJ_DIR)/%.o,$(wildcard $(LIB_DIR)/easygl/*c))
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*c))
UMFPACK_LIB = $(LIB_DIR)/UMFPACK/Lib/libumfpack.a

DEBUG_FLAGS = -g
WARN_FLAGS = -Wall
OPT_FLAGS = -fPIC -O0
INC_FLAGS = -I$(INC_DIR) -I$(LIB_DIR)/easygl -I$(X11_INCLUDE) $(UMFPACK_INCLUDE)

CFLAGS =  $(DEBUG_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(INC_FLAGS)

EXEC = kplace

all: $(EXEC) tags

$(EXEC): $(OBJS) $(OBJS_EASYGL) $(UMFPACK_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PATH) $(LIB)

$(OBJS): $(OBJ_DIR)/%.o:$(SRC_DIR)/%.c $(HEADERS) $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS_EASYGL): $(OBJ_DIR)/%.o:$(LIB_DIR)/easygl/%.c $(HEADERS) $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(UMFPACK_LIB): 
	(cd $(SRC_DIR)/Lib; make)

$(OBJ_DIR):
	mkdir -p $@

tags: 
	ctags -R

.PHONY: clean

clean:
	rm -fv $(OBJ_DIR)/*.o
	rm -fv $(EXEC)
