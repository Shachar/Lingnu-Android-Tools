
all: split_bootimg new_strings

split_bootimg: bootimg.h

new_strings: new_strings.cpp
	g++ new_strings.cpp -o new_strings -lexpat

clean:
	$(RM) new_strings split_bootimg *.o core

.PHONY: clean all
