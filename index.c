#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "index.h"
#include "stemmer.h"
#include "util.h"

#define MAX_SEARCH_RESULTS 10

void write_index_to_file(index_p index);
void parse_file_for_index(index_p index, char *file);
void load_stopwords();
void release_stopwords();
int is_stopword(char *word);
int find_str(char **strs, char *str, int min, int max);
int find_int(int *ints, int i, int min, int max);

int cmp_doc_found(const void *a, const void *b);

static int nr_stopwords = 0;
static char **stopwords = NULL;

typedef struct doc_found {
    int count;              // number of search terms found in a document
    unsigned long flag;     // if the n-th most significant bit is set, the n-th search term was found in this document (ignoring stopwords)
    int doc_id;             // index of the document in the filebase
} doc_found_t, *doc_found_p;

/*
 * Loads stopwords array from the stopwords file
 */
void load_stopwords() {
    FILE *sw_file = fopen("/stopwords", "r");
    if (!sw_file) {
        printf("stopwords file not found.\nCan't remove stopwords!\n");
        return;
    }

    int nr_stopwords;
    char c;
    while ((c = getc(sw_file)) != EOF) {
        if (c == '\n') {
            nr_stopwords++;
        }
    }

    stopwords = (char**) malloc(sizeof(char *) * nr_stopwords);

    rewind(sw_file);

    char *w;
    int i = 0;
    while ((w = read_line(sw_file))) {
        stopwords[i] = w;
        i++;
    }

    fclose(sw_file);
}

/*
 * Releases the memory allocated for the stopwords array
 */
void release_stopwords() {
    if (stopwords) {
        int i;
        for (i = 0; i < nr_stopwords; i++) {
            free(stopwords[i]);
        }
        free(stopwords);
    }
}

/*
 * Checks whether a word is a stopwords
 */
int is_stopword(char *word) {
    int i;
    for (i = 0; i < nr_stopwords; i++) {
        int cmp = strcmp(stopwords[i], word);

        if (!cmp) {
            return 1;
        } else if (cmp > 0) {
            return 0;
        }
    }

    return 0;
}

/*
 * Adds a file to the index
 */
index_p add_file(index_p index, char *file) {
    // insert file into file list (alphabetically ordered)
    int doc_id = 0;
    for (doc_id = 0; doc_id < index->nr_docs; doc_id++) {
        int cmp = strcmp(index->documents[doc_id], file);

        if (!cmp) {
            printf("%s is already in the filebase.\n", file);
            return;
        } else if (0 < cmp) {
            // right position in list found
            break;
        }
    }

    // insert document in list
    printf("index %d\n", doc_id); // DEBUG
    index = (index_p) realloc(index, sizeof(index_t) + sizeof(char *) * (index->nr_docs + 1));
    memcpy(&index->documents[doc_id+1], &index->documents[doc_id], sizeof(char *) * (index->nr_docs - doc_id));
    index->documents[doc_id] = (char *) malloc(strlen(file) + 1);
    memcpy(index->documents[doc_id], file, strlen(file) + 1);
    index->nr_docs++;

    // update indices
    indexed_word_p w = index->words;
    while (w) {
        int i;
        for (i = 0; i < w->nr_docs; i++) {
            if (w->documents[i] >= doc_id) {
                w->documents[i]++;
            }
        }

        w = w->next;
    }

    // parse file contents and add words to index
    parse_file_for_index(index, file);
    write_index_to_file(index);
    return index;
}

/*
 * Removes a file from index
 */
void remove_file(index_p index, char *file) {
    int doc_id = find_str(index->documents, file, 0, index->nr_docs - 1);

    // remove document from list in index
    free(index->documents[doc_id]);
    memcpy(&index->documents[doc_id], &index->documents[doc_id+1], index->nr_docs - 1);
    index->nr_docs--;

    // remove document from each list of each indexed word
    indexed_word_p w = index->words;
    indexed_word_p p = NULL;
    while (w) {
        int i;
        for (i = 0; i < w->nr_docs; i++) {
            if (w->documents[i] == doc_id) {
                memcpy(&w->documents[i], &w->documents[i+1], (w->nr_docs - i - 1) * sizeof(int *));
                w->nr_docs--;
            } else if (w->documents[i] > doc_id) {
                // update index
                w->documents[i]--;
            }
        }

        if (w->nr_docs == 0) {
            // only occurance of this word is in removed document -> remove word from index
            if (!p) {
                index->words = w->next;
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
    write_index_to_file(index);
}

/*
 * Searches index for indexed words and returns documents containing these words
 */
index_p search_index(index_p index, char *query) {
    nonalpha_to_space(query);

    // a set bit at the n-th most significant bit means the n-th word of the query is found in a document
    doc_found_t found[index->nr_docs];
    memset(found, 0, sizeof(doc_found_t) * index->nr_docs);

    int word_nr = 0;
    char *word = strtok(query, " ");

    // array containing pointers to the stem of the search terms
    char *words[strlen(query) - strlen(word)];
    while (word) {
        // ignore stop words
        if (is_stopword(word)) {
            word = strtok(NULL, " ");
            continue;
        }

        char *word_stem = stem(word);

        indexed_word_p w = index->words;
        while (w) {
            int cmp = strcmp(w->stem, word_stem);

            if (!cmp) {
                // long with <word_nr>-th most significant bit set to 1
                unsigned long flag = (ULONG_MAX - (ULONG_MAX >> 1)) >> word_nr;
                
                // increase counter of documents in list and add flag
                int i;
                for (i = 0; i < w->nr_docs; i++) {
                    found[w->documents[i]].count++;
                    found[w->documents[i]].flag |= flag;
                    found[w->documents[i]].doc_id = w->documents[i];
                }

                break;
            } else if (cmp > 0) {
                break;
            }

            w = w->next;
        }

        words[word_nr] = word_stem;
        word = strtok(NULL, " ");
        word_nr++;
    }
    
    index_p result = (index_p) malloc(sizeof(index_p) + sizeof(char *) * MAX_SEARCH_RESULTS);
    result->words = NULL;
    
    unsigned long last_flag = 0;
    indexed_word_p w;
    
    // create a index_p struct with the results, each 'word' in this index represents a group of documents which contains the same (sub-)set of search terms
    int i;
    for (i = 0; i < MAX_SEARCH_RESULTS; i++) {
        if (found[i].flag != last_flag) {
            last_flag = found[i].flag;
            indexed_word_p w_new = (indexed_word_p) malloc(sizeof(indexed_word_t));
            w_new->next = NULL;
            w_new->nr_docs = 0;
            w_new->stem = (char *) malloc(1);
            *w_new->stem = '\0';
            
            // create a string of all search terms found in this document
            int k;
            for (k = sizeof(unsigned long) * 8 - 1; k >= 0; k--) {
                // check whether k-th least significant bit is set
                if (last_flag && 1 << k) {
                    w_new->stem = (char *) realloc(w_new->stem, strlen(w_new->stem) + strlen(words[k]) + 3);
                    strcat(w_new->stem, ", ");
                    strcat(w_new->stem, words[k]);
                }
            }
            
            if (!last_flag) {
                // first result document: set as first element of linked list
                result->words = w_new;
            } else {
                w->next = w_new;
            }
            
            w = w_new;
        }
        
        // add document to list
        result->documents[i] = (char *) malloc(strlen(words[i]) + 1);
        free(words[i]);
        
        w = (indexed_word_p) realloc(w, sizeof(indexed_word_p) + sizeof(int *) * (w->nr_docs + 1));
        w->documents[w->nr_docs] = i;
        w->nr_docs++;
    }
    
    return result;
}

/*
 * Compares two doc_found structs based on the number of found search terms (1st priority) and the first found search term (2nd priority)
 */
int cmp_doc_found(const void *a, const void *b) {
   doc_found_p aa = (doc_found_p) a;
   doc_found_p bb = (doc_found_p) b;
   
   if (aa->count == bb->count && aa->flag == bb->flag) {
       return 0;
   } else  if (aa->count > bb->count || (aa->count < bb->count && aa->flag > bb->flag)) {
       return -1;
   } else {
       return 1;
   }
}

/*
 * Regenerates the index based on the files in the filebase
 */
void rebuild_index(index_p index) {
    // clear index but keep filebase
    indexed_word_p w;
    while ((w = index->words)) {
        index->words = w->next;

        free(w->stem);
        free(w);
    }

    int i;
    for (i = 0; i < index->nr_docs; i++) {
        parse_file_for_index(index, index->documents[i]);
    }

    write_index_to_file(index);
}

/*
 * Parses a file and adds its words to the index
 */
void parse_file_for_index(index_p index, char *file) {
    FILE *f = fopen(file, "r");
    if (!f) {
        printf("Cannot open %s!\nIndex not updated.", file);
        return;
    }

    // document id = index of document in list of all documents in filebase (alphabetically ordered)
    int doc_id = find_str(index->documents, file, 0, index->nr_docs-1);

    int nr_unique_stems = 0;
    char ** unique_stems = (char **) malloc(sizeof(char *));

    char *l;
    while ((l = read_line(f))) {
        // turn non alpha characters into spaces
        nonalpha_to_space(l);

        char *word = strtok(l, " ");
        while (word) {
            // ignore stopwords
            if (is_stopword(word)) {
                word = strtok(NULL, " ");
                continue;
            }

            char *word_stem = stem(word);
            printf("%s -> %s\n", word, word_stem);

            // insert document into index / add new stem to index
            indexed_word_p w = index->words;
            indexed_word_p p = NULL;
            int flag = 0;
            while (w && !flag) {
                printf("testing %s\n", w->stem);
                int cmp = strcmp(w->stem, word_stem);
                if (!cmp) {
                    // stem is already indexed
                    flag = 1;
                    break;
                } else if (0 < cmp) {
                    // stem not indexed yet
                    flag = 2;
                    break;
                }

                p = w;
                w = w->next;
            }

            if (flag == 1) {
                // stem indexed, add document to list
                int i;
                for (i = 0; i < w->nr_docs; i++) {
                    if (w->documents[i] == doc_id) {
                        // document is already indexed for this stem
                        break;
                    } else if (w->documents[i] > doc_id) {
                        // add document to the list for this stem
                        w = (indexed_word_p) realloc(w, sizeof(indexed_word_t) + sizeof(int *) * (w->nr_docs + 1));

                        memcpy(&w->documents[i+1], &w->documents[i], sizeof(int *) * (w->nr_docs - i));
                        w->documents[i] = doc_id;
                        w->nr_docs++;
                    }
                }
            } else {
                // stem is not indexed, add it to index
                w = (indexed_word_p) malloc(sizeof(indexed_word_t) + sizeof(int *));
                w->next = NULL;
                w->nr_docs = 1;
                w->documents[0] = doc_id;

                if (!p) {
                    index->words = w;
                } else {
                    w->next = p->next;
                    p->next = w;
                }
            }

            // get next word
            word = strtok(NULL, " ");
        }

        free(l);
    }

    fclose(f);
}

/*
 * Writes index to file
 */
void write_index_to_file(index_p index) {
    // STEP 1: write filebase to file
    FILE *fb_file = fopen("filebase", "w");
    if (!fb_file) {
        printf("Error: couldn't open filebase file to write.\nUnable to write index to file\n");
        return;
    }

    int i;
    for (i = 0; i < index->nr_docs; i++) {
        fprintf(fb_file, "%s\n", index->documents[i]);
    }

    fclose(fb_file);

    // STEP 2: write index to file

    FILE *index_file = fopen("index", "w");
    if (!index_file) {
        printf("Error: couldn't open index file to write.\nUnable to write index to file\n");
        return;
    }

    // write one word in each line
    indexed_word_p w = index->words;
    while (w) {
        fprintf(index_file, "%s:%d:%d", w->stem, w->nr_docs, w->documents[0]);

        // list all documents containing this word (or variations of it)
        int i;
        for(i = 1; i < w->nr_docs; i++) {
            fprintf(index_file, "|%d", w->documents[i]);
        }

        fprintf(index_file, "\n");

        w = w->next;
    }

    fclose(index_file);
}

/*
 * Parses and loads contents of the index file into a index struct
 */
index_p load_index() {
    // STEP 1: populate list of all documents

    FILE *fb_file = fopen("filebase", "r");
    if (!fb_file) {
        printf("Error: filebase file not found.\nIndex not loaded!\n");
        return NULL;
    }

    // count total number of documents
    char c;
    int nr_docs = 0;
    while ((c = fgetc(fb_file)) != EOF) {
        if (c == '\n') {
            nr_docs++;
        }
    }

    rewind(fb_file);

    // create index struct
    index_p index = (index_p) malloc(sizeof(index_t) + sizeof(char *) * nr_docs);
    index->words = NULL;
    index->nr_docs = nr_docs;

    // load all documents in a list
    int i;
    for (i = 0; i < nr_docs; i++) {
        index->documents[i] = read_line(fb_file);
    }

    fclose(fb_file);

    // STEP 2: populate list of all words

    FILE * index_file = fopen("index", "r");
    if (!index_file) {
        printf("Error: index file not found.\nIndex not loaded!\n");
        return NULL;
    }

    char *line, *stem, *docs, *doc, *tmp;
    while ((line = read_line(index_file))) {
        // get the stem
        stem = strtok(line, ":");

        // ignore empty lines
        if (!stem) {
            continue;
        }

        // get number of documents for this word
        int nr_docs = strtol(strtok(NULL, ":"), &tmp, 10);

        // create struct for stem
        indexed_word_p w = (indexed_word_p) malloc(sizeof(indexed_word_t) + sizeof(int) * nr_docs);
        w->stem = (char *) malloc(strlen(stem) + 1);
        memcpy(w->stem, stem, strlen(stem) + 1);
        w->nr_docs = nr_docs;

        // insert into index
        w->next = index->words;
        index->words = w;

        // get list of documents containing this stem
        docs = strtok(NULL, ":");

        // read each document
        doc = strtok(docs, "|");

        int i = 0;
        while(doc != NULL) {
            w->documents[i] = strtol(doc, &tmp, 10);

            doc = strtok(NULL, "|");
            i++;
        }

        free(line);
    }

    fclose(index_file);

	return index;
}

/*
 * Frees the memory occupied by a index struct
 */
void close_index(index_p index) {
    indexed_word_p w;

    int i;
    for (i = 0; i < index->nr_docs; i++) {
        free(index->documents[i]);
    }

    while ((w = index->words)) {
        index->words = w->next;

        free(w->stem);
        free(w);
    }
    free(index);
}

/*
 * Binary search for document by its name
 */
int find_str(char **strs, char *str, int min, int max) {
    int middle = (min + max) / 2;
    int cmp = strcmp(strs[middle], str);

    if (!cmp) {
        // string found
        return middle;
    } else if (min != max) {
        if (cmp < 0 && min != middle) {
            // continue search in left half
            return find_str(strs, str, min, middle - 1);
        } else if (max != middle) {
            // continue search in right half
            return find_str(strs, str, middle + 1, max);
        }
    }
    
    // finished searching
    return -1;
}

/*
 * Binary search for document by its name
 */
int find_int(int *ints, int i, int min, int max) {
    int middle = (min + max) / 2;

    if (ints[middle] == i) {
        // int found
        return middle;
    } else if (min != max) {
        if (ints[middle] > i) {
            // continue search in left half
            return find_int(ints, i, min, middle - 1);
        } else {
            // continue search in right half
            return find_int(ints, i, middle + 1, max);
        }
    } else {
        // finished searching
        return -1;
    }
}
