CC=gcc
CFLAGS=-g
BUILD_DIR=build/
BIN_DIR=bin/
BUILD_DIRS=dir
OBJS= 	$(BUILD_DIR)mem_manager.o	\
		${BUILD_DIR}test_app.o		\


all: $(BUILD_DIRS) test_app

test_app:$(BUILD_DIR)test_app.o ${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o $(BIN_DIR)test_app 

${BUILD_DIR}test_app.o:test_app.c
	${CC} ${CFLAGS} -c -I test_app.c -o ${BUILD_DIR}test_app.o

${BUILD_DIR}mem_manager.o:mem_manager/mem_manager.c
	${CC} ${CFLAGS} -c -I mem_manager/mem_manager.c -o ${BUILD_DIR}mem_manager.o

$(BUILD_DIRS):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)
clean:
	rm -rf build bin