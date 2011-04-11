// newstrings.c

#include <cstring>
#include <iostream>
#include <fstream>

#include "expat.h"

#define BUFSIZE 4096


using namespace std;

typedef struct string_entry_s {
    char * name;
    char * value;
    struct string_entry_s * next;
} string_entry;

typedef struct {
    XML_Parser *parser;
    string_entry *string_list;
} parser_context;

void tag_content(void *userData, const XML_Char *s, int len)
{
    char str[len+1];
    strncpy(str, s, len);
    str[len] = 0;
    cout<<"content :"<<str<<endl;
}

void tag_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
    cout<<"start "<<name<<endl;
    
    for (int idx=0; atts[idx] != NULL; idx+=2)
	cout<<"attrs "<<atts[idx]<<" = "<<atts[idx+1]<<endl;

    parser_context* context = (parser_context*)userData;

    XML_SetCharacterDataHandler(*context->parser, tag_content);    
}

void tag_end(void *userData, const XML_Char *name)
{
    cout<<"end "<<name<<endl;
    parser_context* context = (parser_context*)userData;

    XML_SetCharacterDataHandler(*context->parser, NULL);
}


void parse_strings_xml(char* filename)
{
    ifstream file(filename, ios::in); 
    
    if ( file.fail() ) {
	cerr<<"Cannot open "<<filename<<" : "; perror(0);
	exit(1);
    }
    
    XML_Parser p = XML_ParserCreate(NULL);
    if (! p) {
	cerr<<"Couldn't allocate memory for parser"<<endl;
	exit(-1);
    }

    parser_context context;
    context.parser = &p;

    XML_SetElementHandler(p, tag_start, tag_end);
    XML_SetUserData(p, &context);

    for (;;) {
	char buf[BUFSIZE];
	file.read(buf, BUFSIZE);

	if (file.fail() == -1) {
	    cerr<<"Couldn't read from file : "; perror(0);
	    exit(1);
	}
	
	XML_Parse(p, buf, file.gcount(), file.eof());	
	
	if (file.eof())
	    break;
    }	
    
    // p.    
}


int main(int argc, char * argv[])
{
    if (argc != 4) {
	cout<<"usage : "<<argv[0]<<" old_strings_en new_strings_en old_string_iw"<<endl;
	return 0;
    }

    parse_strings_xml(argv[1]);
	
    return 0;
}
