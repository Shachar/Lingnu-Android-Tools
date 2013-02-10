// newstrings.cpp

#include <cstring>
#include <iostream>
#include <fstream>
#include <map>

#include "expat.h"

#define BUFSIZE 4096

enum diff_operation {NOP, ADDED, DELETED, MODIFIED};

using namespace std;

typedef struct {
    XML_Parser *parser;
    map<string,string> strings;
    string current_name;
    bool formatted;
} parser_context;

std::string quote( const std::string & raw )
{
    std::string ret;

    for( unsigned int i=0; i<raw.length(); ++i ) {
        switch( raw[i] ) {
        case '<':
            ret+="&lt;";
            break;
        case '>':
            ret+="&gt;";
            break;
        case '&':
            ret+="&amp;";
            break;
        default:
            ret+=raw[i];
            break;
        }
    }

    return ret;
}

void tag_content(void *userData, const XML_Char *s, int len)
{
    char str[len+1];
    strncpy(str, s, len);
    str[len] = 0;
    parser_context* context = (parser_context*)userData;

    if (context->current_name.size() > 0)
	context->strings[context->current_name] += str;
}

void tag_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
    parser_context* context = (parser_context*)userData;
    
    for (int idx=0; atts[idx] != NULL; idx+=2)
	if (strcmp(atts[idx], "name") == 0) 
	    context->current_name = atts[idx+1];
	else if ((strcmp(atts[idx], "translatable") == 0) &&
		 (strcmp(atts[idx+1], "false") == 0))
	    return;  // ignore not translatable strings
	else if (strcmp(atts[idx], "formatted") == 0 )
            context->formatted = strcmp(atts[idx+1], "false")!=0;
        else 
	    {
		cerr<<"unknown attribute '"<<atts[idx]<<"' for element <string>"<<endl;
		exit(1);
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
    context.formatted = true;

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

void diff(map<string,string>& base, map<string,string>& other, map<string,diff_operation>& results)
{
    map<string, string>::iterator p;

    // search new strings:
    for(p = other.begin(); p != other.end(); p++)
	if ( base.find(p->first) == base.end() )
	    results[p->first] = ADDED;
    
    // search modified / deleted strings:
    for(p = base.begin(); p != base.end(); p++) {
	if ( other.find(p->first) == other.end() )
	    results[p->first] = DELETED;
	else if ( p->second != other[p->first] )
	    results[p->first] = MODIFIED;
    }

}

int merge2(char * old_strings_iw_file, char * new_strings_iw_file)
{
    // parse
    map<string,string> old_strings_iw;
    parse_strings_xml(old_strings_iw_file, old_strings_iw);

    map<string,string> new_strings_iw;
    parse_strings_xml(new_strings_iw_file, new_strings_iw);

    // output    
    cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>"<<endl;
    cout<<"<resources>"<<endl;

    map<string, string>::iterator p;
    for(p = old_strings_iw.begin(); p != old_strings_iw.end(); p++) {
	
	if ( new_strings_iw.find(p->first) != new_strings_iw.end() ) {
	    cout<<"<string name=\""<<p->first<<"\">"<<quote(new_strings_iw[p->first])<<
                    "</string>"<<endl;
	    new_strings_iw.erase(p->first);
	} else
	    cout<<"<string name=\""<<p->first<<"\">"<<quote(p->second)<<"</string>"<<endl;	    
    }    
    
    for(p = new_strings_iw.begin(); p != new_strings_iw.end(); p++)
	cout<<"<string name=\""<<p->first<<"\">"<<quote(p->second)<<"</string>"<<endl;

    cout<<"</resources>"<<endl;

    return 0;
}

int merge3(char * old_strings_en_file, char * new_strings_en_file, char * old_strings_iw_file)
{
    // parse
    map<string,string> old_strings_en;
    parse_strings_xml(old_strings_en_file, old_strings_en);

    map<string,string> new_strings_en;
    parse_strings_xml(new_strings_en_file, new_strings_en);

    map<string,string> old_strings_iw;
    parse_strings_xml(old_strings_iw_file, old_strings_iw);

    // diff
    map<string,diff_operation> diff_old_new;
    diff(old_strings_en, new_strings_en, diff_old_new);

    // output    
    cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>"<<endl;
    cout<<"<resources>"<<endl;

    map<string, string>::iterator p;
    for(p = old_strings_iw.begin(); p != old_strings_iw.end(); p++) {
	diff_operation diff_op = NOP;
	if ( diff_old_new.find(p->first) != diff_old_new.end() )
	    diff_op = diff_old_new[p->first];
	
	if (diff_op == DELETED)
	    continue;

	cout << "<string name=\""<<p->first<<"\"";

	if (diff_op == MODIFIED)
	    cout<<" MODIFIED";

	cout<<">";
	
	cout<<p->second<<"</string>"<<endl;
    }

    map<string, diff_operation>::iterator p_diff;
    for(p_diff = diff_old_new.begin(); p_diff != diff_old_new.end(); p_diff++)
	if (p_diff->second == ADDED)
	    cout << "<string name=\""<<p_diff->first<<"\" NEW"<<">"<<"</string>"<<endl;
    
    cout<<"</resources>"<<endl;

    return 0;
}

void usage(char * progname)
{
    cout<<"usage : "<<progname<<" old_strings_en new_strings_en old_string_iw"<<endl;
    cout<<"        "<<progname<<" old_string_iw new_strings_iw"<<endl;
    exit(0);
}

int main(int argc, char * argv[])
{
    if ((argc < 3) || (argc > 4))
	usage(argv[0]);

    if (argc == 3)
	return merge2(argv[1], argv[2]);
    
    return merge3(argv[1], argv[2], argv[3]);
}
