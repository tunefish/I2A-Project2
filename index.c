#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "util.h"

/*add file to database*/
void add_file(char * file)
{
	//TODO: remove all stop words from file
	//TODO: stem all remaining words
	//TODO: create new indexed words
	//TODO: update already indexed words
	return;
}

/*remove file from database*/
void remove_file(char * file)
{
	//TODO: search for indexed word containing file in database
	//TODO: remove file from *documents[] in indexed word
	return;
}

/*search database for indexed word. Return documents containing word*/
indexed_word_p search_database(char * query)
{
	//TODO: search database for indexed word
	//TODO: return *documents[] from indexed word
	return NULL;
}

/*rebuild list of indexed words and documents associated with them*/
void rebuild_index()
{
	//TODO:
    return;
}

/*load a database that already contains indexed words*/
database_p load_database() {
	FILE * db_file = fopen("index", "r");
    if (!db_file) {
        printf("Error: index file not found\n");
        exit(1);
    }
    
    database_p db = (database_p) malloc(sizeof(database_t));
    db->words = NULL;
    
    char * line;
    char * stem;
    char * doc;
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