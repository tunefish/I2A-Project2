#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "stemmer.h"
#include "util.h"

int starts_with(char *str, char *pre);
char *read_line(FILE *ptr);
void sanitize_name(char* str);

int main(int argc, void *argv) {
    index_p index = load_index();

    int exit = 0;
    char *command;
    while (!exit) {
        printf(" > ");
        char *command = read_line(stdin);
        
        if (!strcmp(command, "exit")) {
            // exit command
            exit = 1;
            printf("Exit requested..");
            
		} else if (strcmp(command, "rebuild index ")) {
            // rebuild index command
            close_index(index);
            index = rebuild_index();
		} else if (starts_with(command, "search for ")) {
            // search for <search_query> command
            char *query = (char *) malloc(strlen(command) - 10);
            memcpy(query, command+11, strlen(command) - 10);
            
            index_p result = search_index(index, query);
            
            if (result) {
                // print result
                int count = 0;
                indexed_word_p w = result->words;
                while (w) {
                    printf("Documents containing %s:\n", w->stem);
                    
                    int i;
                    for (i = 0; i < w->nr_docs; i++, count++) {
                        printf(" [%2d] %s\n", count, w->documents[i]);
                    }
                    
                    w = w->next;
                }
                
                close_index(result);
            } else {
                printf("No results found in filebase!\n");
            }
            
            free(query);
            
        } else if (!strcmp(command, "add file ")) {
            // add file <file> command
            char *file = (char*) malloc(strlen(command) - 8);
            memcpy(file, command+9, strlen(command) - 8);
            
			add_file(index, file);
            free(file);
            
        } else if (starts_with(command, "remove file ")) {
            // remove file <file> command
            char *file = (char*) malloc(strlen(command) - 11);
            memcpy(file, command+12, strlen(command) - 11);
            
            remove_file(index, file);
            free(file);
        }
    }

    // release memory
    close_index(index);

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