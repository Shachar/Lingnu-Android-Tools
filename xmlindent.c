// xmlindent - organize xml indentation level and attributes
// Copyright (C) 2011 Lingnu Open Source Consulting Ltd.
// Written by Asaf Ohaion
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include <stdio.h>
#include "expat.h"

int depth = 0;

void usage(char * progname)
{
    printf("usage : %s xml_file\n", progname);
    exit(0);
}

void fail(char * message)
{
    fprintf(stderr, "%s\n", message);
    exit(10);
}

void indent()
{
    int i;
    for (i=0; i<depth*4; i++)
	printf(" ");
}

void tag_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
    depth++;
    indent();
    printf("<%s", name);

    int i;
    for(i = 0; atts[i]; i += 2)	{
	if (i > 0) {
	    indent();
	    printf("\t");
	}
	printf(" %s=\"%s\"", atts[i], atts[i + 1]);
	if (atts[i+2])
	    printf("\n");
    }
    printf(">\n");
}

void tag_end(void *userData, const XML_Char *name)
{
    indent();
    printf("</%s>\n", name);
    depth--;
}

int main(int argc, char * argv[])
{
    if (argc < 2)
	usage(argv[0]);

    char *buffer;
    int fsize;

    XML_Parser parser = XML_ParserCreate(NULL);
    if (! parser) 
	fail("Couldn't allocate memory for parser");

    FILE * fp = fopen(argv[1], "r");

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    buffer = (char *)malloc(fsize+1);

    fread(buffer, 1, fsize, fp);

    buffer[fsize] = '\0';

    XML_SetElementHandler(parser, tag_start, tag_end);

    XML_Parse(parser, buffer, fsize, 0);

    return 0;
}
