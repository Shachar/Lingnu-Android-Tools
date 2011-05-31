
all: split_bootimg new_strings de9patch xmlindent

split_bootimg: bootimg.h

de9patch: de9patch.c
	gcc -lpng de9patch.c -o de9patch

new_strings: new_strings.cpp
	g++ new_strings.cpp -o new_strings -lexpat

xmlindent: xmlindent.c
	gcc xmlindent.c -o xmlindent -lexpat

clean:
	$(RM) new_strings split_bootimg *.o core de9patch xmlindent

.PHONY: clean all
