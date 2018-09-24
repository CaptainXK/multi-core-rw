.PHONY:clean check_obj_dir

OBJ_DIR := obj
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS) )
LIBS := -lpthread


test.app:check_obj_dir $(OBJS)
	gcc $(OBJS) -o $@ -g $(LIBS)


check_obj_dir:
# use "test -d " to test whether it exsits and it is a directory
	@if test ! -d $(OBJ_DIR);\
		then\
		mkdir $(OBJ_DIR);\
	fi


$(OBJ_DIR)/%.o:%.c
	gcc -c $< -o $@ -g


test:test.app
	$(EXEC) ./test.app


clean:
	rm -r $(OBJ_DIR)/*.o *.app