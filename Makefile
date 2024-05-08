OBJ_DIR=./build
EVB_SRCS = $(subst src/,,$(wildcard src/*.c))
EVB_OBJS = $(EVB_SRCS:%.c=$(OBJ_DIR)/%.o)

INCDIR = ./include
CFLAGS = -Wall -pedantic -I$(INCDIR)
LFLAGS = -L/usr/local/lib

CC = gcc $(FLAGS)

LIBS = -ljemalloc -pthread -lrt -ljson-c -luuid

EXE = ./bin/evb

all: build_dirs includes $(EVB_OBJS) $(EXE)

build_dirs:
	@mkdir -p build
	@mkdir -p bin
	@mkdir -p include/evb

includes: build_dirs
	@find ./src -name "*.h" -type f -exec cp {} ./include/evb \;

$(EXE): $(EVB_OBJS)
	$(CC) -o $@ $(EVB_OBJS) $(LFLAGS) $(LIBS) $(INCLUDE)

$(OBJ_DIR)/%.o: ./src/%.c includes
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDE) $(CXXFLAGS)

clean: 
	-$(RM) build/*.o $(EXE) include/evb/*

