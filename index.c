#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "util.h"

/*add file to database*/
void add_file(database_p db, char * file) {
	FILE *fb_file = fopen("filebase", "a");
    if (!fb_file) {
        printf("Error: filebase file not found\n");
        return;
    }
    
    fprintf(fb_file, "%s\n", file);
    fclose(fb_file);
    
    // TODO: parse contents of file and add it to database
}

/*remove file from database*/
void remove_file(database_p db, char * file)
{
	FILE *fb_file = fopen("filebase", "a");
    if (!fb_file) {
        printf("Error: filebase file not found\nFile not removed from filebase!\n");
        return;
    }
    
    char *f;
    int remove_line = 0;
    while ((f = read_line(fb_file))) {
        if (strlen(f) == strlen(file) && !memcmp(f, file, strlen(f))) {
            break;
        }
        
        remove_line++;
    }
    
    rewind(fb_file);
    
    // create copy of filebase in .fb_tmp_cpy without the line containing the file to be removed
    FILE *fb_copy = fopen(".fb_tmp_cpy", "w");
    char c;
    int line = 0;
    while ((c = getc(fb_file)) != EOF) {
        // write all characters except the ones in the line containing the file to be removed
        if (line != remove_line) {
            putc(c, fb_copy);
        }
        
        if (c == '\n') {
            line++;
        }
    }
    
    fclose(fb_file);
    fclose(fb_copy);
    
    // remove original filebase and replace it with new copy
    remove("filebase");
    rename(".fb_tmp_cpy", "filebase");
    
    
}

/*search database for indexed word. Return documents containing word*/
indexed_word_p search_database(database_p db, char * query) {
	//TODO: search database for indexed words
	//TODO: return *documents[] from indexed word
	return NULL;
}

/*rebuild list of indexed words and documents associated with them*/
void rebuild_index(database_p db) {
	//TODO:
    return;
}

/*
 * Parses and loads contents of the index file into a database struct
 */
database_p load_database() {
	FILE * db_file = fopen("index", "r");
    if (!db_file) {
        printf("Error: index file not found\n");
        return NULL;
    }
    
    database_p db = (database_p) malloc(sizeof(database_t));
    db->words = NULL;
    
    char *line;
    char *stem;
    char *doc;
    while ((line = read_line(db_file))) {
        // get the stem
        stem = strtok(line, ":");
       
        // ignore empty lines
        if (!stem) {
            continue;
        }
        
        // create struct for stem
        int struct_size = sizeof(indexed_word_t);
        indexed_word_p w = (indexed_word_p) malloc(struct_size);
        w->stem = (char *) malloc(strlen(stem) + 1);
        memcpy(w->stem, stem, strlen(stem) + 1);
        w->nr_docs = 0;
        
        // insert into db
        w->next = db->words;
        db->words = w;
        
        // get list of documents containing this stem
        stem = strtok(NULL, ":");
        
        // read each document
        doc = strtok(stem, "|");
        while(doc != NULL) {
            struct_size+= sizeof(char*);
            w = (indexed_word_p) realloc(w, struct_size);
            w->documents[w->nr_docs] = (char *) malloc(strlen(doc) + 1);
            memcpy(w->documents[w->nr_docs], doc, strlen(doc) + 1);
            w->nr_docs++;
            
            doc = strtok(NULL, "|");
        }
    }
    
    fclose(db_file);
    
	return db;
}

/*
 * Frees the memory occupied by a database struct
 */
void close_database(database_p db) {
    indexed_word_p w;
    while ((w = db->words)) {
        db->words = w->next;
        
        free(w->stem);
        
        int i;
        for (i = 0; i < w->nr_docs; i++) {
            free(w->documents[i]);
        }
        
        free(w);
    }
    free(db);
}
