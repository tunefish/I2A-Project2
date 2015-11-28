#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "util.h"
#include "stemmer.h"

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
				// TODO: return documents containing all words of the search queue, followed by n-1 and n-2 words (up to 10). 
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