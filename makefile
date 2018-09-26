.PHONY:clean check_obj_dir

CC := clang++
OBJ_DIR := obj
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS) )
LIBS := -lpthread
#DEBUG := -D DEBUG
CFLAGS := -I. -std=c++11


test.app:check_obj_dir $(OBJS)
	$(CC) $(OBJS) -o $@ -g $(LIBS)


check_obj_dir:
# use "test -d " to test whether it exsits and it is a directory
	@if test ! -d $(OBJ_DIR);\
		then\
		mkdir $(OBJ_DIR);\
	fi


$(OBJ_DIR)/%.o:%.cpp
	$(CC) -c  $(CFLAGS) $< -o $@ -g $(DEBUG) 


test:test.app
	$(EXEC) ./test.app


clean:
	rm -r $(OBJ_DIR)/*.o *.app
