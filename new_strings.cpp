// newstrings.c

#include <cstring>
#include <iostream>
#include <fstream>
#include <map>

#include "expat.h"

#define BUFSIZE 4096


using namespace std;

typedef struct {
    XML_Parser *parser;
    map<string,string> strings;
    string current_name;
} parser_context;

void tag_content(void *userData, const XML_Char *s, int len)
{
    char str[len+1];
    strncpy(str, s, len);
    str[len] = 0;
    parser_context* context = (parser_context*)userData;

    if (context->current_name.size() > 0)
	context->strings[context->current_name] = str;
}

void tag_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
    parser_context* context = (parser_context*)userData;
    
    for (int idx=0; atts[idx] != NULL; idx+=2)
	if (strcmp(atts[idx], "name") == 0) {
	    context->current_name = atts[idx+1];
	}
    
    XML_SetCharacterDataHandler(*context->parser, tag_content);    
}

void tag_end(void *userData, const XML_Char *name)
{
    parser_context* context = (parser_context*)userData;

    XML_SetCharacterDataHandler(*context->parser, NULL);
}


void parse_strings_xml(char* filename, map<string, string>& string_map)
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
    context.strings = string_map;

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
	
	if (file.eof()) {
	    string_map = context.strings;
	    break;
	}
    }	    
}


int main(int argc, char * argv[])
{
    if (argc != 4) {
	cout<<"usage : "<<argv[0]<<" old_strings_en new_strings_en old_string_iw"<<endl;
	return 0;
    }

    cout<<"parsing "<<argv[1]<<endl;
    map<string,string> old_strings_en;
    parse_strings_xml(argv[1], old_strings_en);

    cout<<"parsing "<<argv[2]<<endl;
    map<string,string> new_strings_en;
    parse_strings_xml(argv[2], old_strings_en);

    cout<<"parsing "<<argv[3]<<endl;
    map<string,string> old_strings_iw;
    parse_strings_xml(argv[3], old_strings_en);

/*    map<string, string>::iterator p;
    
    cout<<"Printing Strings.."<<endl;
    for(p = old_strings_en.begin(); p != old_strings_en.end(); p++) {
	cout << p->first << " = " << p->second << endl;
	}	    */

    return 0;
}
