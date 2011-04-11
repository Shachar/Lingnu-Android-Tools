// newstrings.c

#include <stdio.h>
#include <fcntl.h>
#include "expat.h"

#define BUFSIZE 4096

typedef struct {
    char * name;
    char * value;
} string_entry;


void tag_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
    printf("start %s\n", name);
    
    int idx;    
    for (idx=0; atts[idx] != NULL; idx+=2)
	printf("attr %s = %s\n", atts[idx], atts[idx+1]);
    
}

void tag_end(void *userData, const XML_Char *name)
{
    printf("end %s\n", name);
}

void parse_strings_xml(char* filename)
{
    int fd;
    
    if (( fd = open(filename, O_RDONLY)) == -1) {
	fprintf(stderr, "Cannot open %s : ", filename); perror(0);
	exit(1);
    }
    
    XML_Parser p = XML_ParserCreate(NULL);
    if (! p) {
	fprintf(stderr, "Couldn't allocate memory for parser\n");
	exit(-1);
    }   

    XML_SetElementHandler(p, tag_start, tag_end);

    for (;;) {
	char buf[BUFSIZE];
	ssize_t count = read(fd, buf, BUFSIZE);

	if (count == -1) {
	    fprintf(stderr, "Couldn't read from file : ");perror(0);
	    exit(1);
	}
	
	XML_Parse(p, buf, count, (count == 0));	
	
	if (count == 0) 
	    break;
    }	
    
    // p.    
}


int main(int argc, char * argv[])
{
    if (argc != 4) {
	printf("usage : %s old_strings_en new_strings_en old_string_iw\n", argv[0]);
	return 0;
    }

    parse_strings_xml(argv[1]);
	
    return 0;
}
