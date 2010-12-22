
all: split_bootimg

split_bootimg: bootimg.h

clean:
	$(RM) split_bootimg *.o core

.PHONY: clean all
