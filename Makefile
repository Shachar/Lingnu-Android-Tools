
all: split_bootimg

split_bootimg: bootimg.h

new_strings: new_strings.c
	gcc new_strings.c -o new_strings -lexpat

clean:
	$(RM) split_bootimg *.o core

.PHONY: clean all
