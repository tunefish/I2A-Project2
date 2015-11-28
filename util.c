#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

/*
 * Reads a line
 */
char *read_line(FILE *ptr) {
    char *line = malloc(128), *linep = line;
    size_t lenmax = 128, len = lenmax;
    int c;

    if (line == NULL) {
        return NULL;
    }

    for (;;) {
        c = fgetc(ptr);
        
        // end of string / file => return string
        if(c == EOF) {
            break;
        }

        // string buffer full => double the size
        if (--len == 0) {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);

            if (linen == NULL) {
                free(linep);
                return NULL;
            }
            
            line = linen + (line - linep);
            linep = linen;
        }

        // end of line => return string
        if((*line++ = c) == '\n') {
            break;
        }
    }

    *line = '\0';
    
    // nothing read => return NULL
    if (strlen(linep) == 0) {
        return NULL;
    }
    
    // remove newline characted at the end of the string
    if (*(line - 1) == '\n') {
        *(line - 1) = '\0';
    }
    
    return linep;
}
