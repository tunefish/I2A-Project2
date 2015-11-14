#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.c"

int starts_with(char *str, char *pre);
char *read_line(FILE *ptr);
void sanitize_name(char* str);

int main(int argc, void *argv) {

    int exit = 0;
    char *command;
    while (!exit) {
        printf(" > ");
        char *command = read_line(stdin);
        
        if (!strcmp(command, "exit")) {
            // exit program
            exit = 1;
            printf("Exit requested..");
            
		}
		else if (strcmp(command, "rebuild index ")) {
			// TODO: rebuild index
		}
		else if (starts_with(command, "search for ")) {
            if (strlen(command) > 11) {
                // TODO: split search queue into words
                // TODO: stem search queue
                // TODO: search database index
				// TODO: return documents containing all words of the search queue, followed by n-1 and n-2 words. 
            }
            
        } else if (!strcmp(command, "add file ")) {
			// TODO: open file
			// TODO: remove stop words
			// TODO: stem file
			// TODO: add new stemmed words to index 
			// TODO: add file to appropriate index
            
        } else if (starts_with(command, "remove file ")) {
            // TODO: remove file from index
            
        }
    }

    // TODO: release resources allocated by program

    return 0;
}

/*
 * Checks if pre is a prefix of str
 */
int starts_with(char *str, char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

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

/*
 * Removes all whitespace caracters from a string
 */
void sanitize_name(char *str) {
    int i = 0;
    int j = 0;
    while (i < strlen (str)) {
        if (isalpha(str[i])) {
            str[j] = tolower(str[i]);
            i++;
            j++;
        } else {
            i++;
        }
    }
    
    str[j] = '\0';
}