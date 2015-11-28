#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "util.h"
#include "stemmer.h"

void write_index_to_file(database_p db);
void parse_file_for_index(database_p db, char *file);

/*
 * Adds a file to the index
 */
void add_file(database_p db, char *file) {
	FILE *fb_file = fopen("filebase", "a");
    if (!fb_file) {
        printf("Error: filebase file not found\nFile not added to filebase!");
        return;
    }
    
    fprintf(fb_file, "%s\n", file);
    fclose(fb_file);
    
    // parse file contents and add words to index
    parse_file_for_index(db, file);
    write_index_to_file(db);
}

/*
 * Removes a file from index
 */
void remove_file(database_p db, char *file) {
	FILE *fb_file = fopen("filebase", "a");
    if (!fb_file) {
        printf("Error: filebase file not found\nFile not removed from filebase!\n");
        return;
    }
    
    char *f;
    int remove_line = 0;
    while ((f = read_line(fb_file))) {
        int cmp = strcmp(f, file);
        if (!cmp) {
            break;
        } else if (cmp > 1) {
            // documents sorted -> no need to search further
            printf("Error: %s is not in the index\nFile not removed from filebase!\n");
            return;
        }
        
        remove_line++;
    }
    
    rewind(fb_file);
    
    // create copy of filebase in .fb_tmp_cpy without the line containing the file to be removed
    FILE *fb_copy = fopen(".fb_tmp_cpy", "w");
    if (!fb_copy) {
        fprint("Error: couldn't create temporary copy of filebase.\n File not removed from filebase!\n");
        return;
    }
    
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
    
    // remove document from index
    indexed_word_p w = db->words;
    indexed_word_p p = NULL;
    while (w) {
        int i;
        for (i = 0; i < w->nr_docs; i++) {
            int cmp = strcmp(w->documents[i], file);
            if (!cmp) {
                memcpy(w->documents[i], w->documents[i+1], (w->nr_docs - i - 1) * sizeof(char *));
                w->nr_docs--;
                break;
            } else if (cmp > 0) {
                // documents sorted -> no need to search further
                break;
            }
        }
        
        if (w->nr_docs == 0) {
            // only occurance of this word is in removed document -> remove word from index
            if (!p) {
                db->words = w->next;
            } else {
                p->next = w->next;
            }
            
            indexed_word_p n = w->next;
            free(w->stem);
            free(w);
            w = n;
        } else {
            // get next indexed word
            p = w;
            w = w->next;
        }
    }
    
    // commit changes to file
    write_index_to_file(db);
}

/*
 * Searches index for indexed words and returns documents containing these words
 */
indexed_word_p search_database(database_p db, char *query) {
	//TODO: search database for indexed words
	//TODO: return *documents[] from indexed word
	return NULL;
}

/*
 * Regenerates the index based on the files in the filebase
 */
database_p rebuild_index() {
    FILE *fb_file = fopen("filebase", "r");
    if (!fb_file) {
        printf("Error: filebase file not found\nIndex was not rebuild\n");
        return;
    }
    
    database_p db = (database_p) malloc(sizeof(database_t));
    db->words = NULL;
    
    char *file;
    while ((file = read_line(fb_file))) {
        parse_file_for_index(db, file);
    }
    
    fclose(fb_file);
    
    write_index_to_file(db);
    return db;
}

/*
 * Parses a file and adds its words to the index
 */
void parse_file_for_index(database_p db, char *file) {
    
}

/*
 * Writes index to file
 */
void write_index_to_file(database_p db) {
    FILE *db_file = fopen("index", "w");
    if (!db_file) {
        printf("Error: couldn't open index file to write.\nUnable to write index to file\n");
        return;
    }
    
    // write one word in each line
    indexed_word_p w = db->words;
    while (w) {
        fprintf(db_file, "%s:%s", w->stem, w->documents[0]);
        
        // list all documents containing this word (or variations of it)
        int i;
        for(i = 1; i < w->nr_docs; i++) {
            fprintf(db_file, "|%s", w->documents[i]);
        }
        
        fprintf(db_file, "\n");
        
        w = w->next;
    }
    
    fclose(db_file);
}

/*
 * Parses and loads contents of the index file into a database struct
 */
database_p load_database() {
	FILE * db_file = fopen("index", "r");
    if (!db_file) {
        printf("Error: index file not found\nDatabase not loaded\n");
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
