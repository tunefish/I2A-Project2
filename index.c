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

int cmp_doc_found_desc(const void *a, const void *b);

static int nr_stopwords = 0;
static char **stopwords = NULL;

typedef struct doc_found {
    int doc_id;             // index of the document in the filebase
    int count;              // number of search terms found in a document
    unsigned long flag;     // if the n-th most significant bit is set, the n-th search term was found in this document (ignoring stopwords)
} doc_found_t, *doc_found_p;

/*
 * Loads stopwords array from the stopwords file
 */
void load_stopwords() {
    // open file or print error message
    FILE *sw_file = fopen("stopwords", "r");
    if (!sw_file) {
        printf("stopwords file not found.\nCan't remove stopwords!\n");
        return;
    }

    // count number of stopwords
    nr_stopwords = 0;
    char c;
    while ((c = getc(sw_file)) != EOF) {
        if (c == '\n') {
            nr_stopwords++;
        }
    }

    stopwords = (char**) malloc(sizeof(char *) * nr_stopwords);
    rewind(sw_file);

    // load stopwords into array
    int i;
    for (i = 0; i < nr_stopwords; i++) {
        stopwords[i] = read_line(sw_file);
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
    if (!stopwords) {
        load_stopwords();
    }
    
    return find_str(stopwords, word, 0, nr_stopwords - 1) != -1;
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
    index = (index_p) realloc(index, sizeof(index_t) + sizeof(char *) * (index->nr_docs + 1));
    memcpy(&index->documents[doc_id+1], &index->documents[doc_id], sizeof(char *) * (index->nr_docs - doc_id));
    index->documents[doc_id] = (char *) malloc(strlen(file) + 1);
    memcpy(index->documents[doc_id], file, strlen(file) + 1);
    index->nr_docs++;

    // update indices: increase indices which are greater or equal to doc_id of added document
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
    // open file or print error message
    if (!index->nr_docs) {
        printf("Filebase empty!\n");
        return;
    }
    
    // obtain document id a.k.a. index in filebase
    int doc_id = find_str(index->documents, file, 0, index->nr_docs - 1);
    
    if (doc_id < 0) {
        printf("Error: %s is not in the filebase!\n", file);
        return;
    }

    // remove document from list in index
    free(index->documents[doc_id]);
    memcpy(&index->documents[doc_id], &index->documents[doc_id+1], sizeof(char *) * (index->nr_docs - 1));
    index->nr_docs--;

    indexed_word_p w = index->words;    // current word
    indexed_word_p p = NULL;            // previous word
    
    // remove document from the list of each indexed word
    while (w) {
        // find index of removed document in list (or of first document with higher id)
        int i;
        int remove = 0;
        for (i = 0; i < w->nr_docs; i++) {
            if (w->documents[i] == doc_id) {
                w->nr_docs--;
                // document found in list, indicate removal
                remove = 1;
                break;
            } else if (w->documents[i] > doc_id) {
                break;
            }
        }
        
        // reduce document id of all documents with id > removed document id
        // and shift array items (in order to remove entry of the document we want to remove) if neccessary
        for (; i < w->nr_docs; i++) {
            w->documents[i] = w->documents[i+remove] - 1;
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

    // array containing pointers to the stem of the search terms
    char *words[strlen(query)];
    
    // if bit at the n-th most significant bit is set it means the n-th word of the query is found in a document
    doc_found_t found[index->nr_docs];
    memset(found, 0, sizeof(doc_found_t) * index->nr_docs);

    int nr_words = 0;
    char *word = strtok(query, " ");

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
                // this stem is one of the search terms
                
                // unsigned long with <nr_words>-th most significant bit set to 1
                unsigned long flag = 1 << (sizeof(unsigned long) * 8 - 1 - nr_words);
                
                // increase counter of documents in list and update flags
                int i;
                for (i = 0; i < w->nr_docs; i++) {
                    found[w->documents[i]].doc_id = w->documents[i];
                    found[w->documents[i]].count++;
                    found[w->documents[i]].flag |= flag;
                }

                break;
            } else if (cmp > 0) {
                // words in index are alphabetically orderd => no need to search further
                break;
            }

            w = w->next;
        }

        // save stem for later and continue with next search term
        words[nr_words] = word_stem;
        word = strtok(NULL, " ");
        nr_words++;
    }
    
    // sort documents by number of search terms found in each document and the order of the search terms
    qsort(&found, index->nr_docs, sizeof(doc_found_t), cmp_doc_found_desc);
    
    // create result index
    index_p result = (index_p) malloc(sizeof(index_t) + sizeof(char *) * MAX_SEARCH_RESULTS);
    result->nr_docs = 0;
    result->words = NULL;
    
    unsigned long last_flag = 0;    // flag of last processed document
    indexed_word_p w = NULL;        // current group of documents (of the same (sub-)set of search terms)
    indexed_word_p p = NULL;        // previous group of documents
    
    // create a index_p struct with the results, each 'word' in this index represents a group of documents which contains the same (sub-)set of search terms
    int i;
    for (i = 0; i < MAX_SEARCH_RESULTS && i < index->nr_docs && found[i].flag; i++) {
        if (found[i].flag != last_flag) {
            // the flag is not equal to previous one => create new 'group' of documents
            indexed_word_p w_new = (indexed_word_p) malloc(sizeof(indexed_word_t));
            w_new->next = NULL;
            w_new->nr_docs = 0;
            w_new->stem = (char *) malloc(1);
            *w_new->stem = '\0';
            
            // create a string of all search terms found in this document
            int k;
            for (k = 0; k < nr_words; k++) {
                // check whether k-th most significant bit is set
                if (found[i].flag & (1 << (sizeof(unsigned long) * 8 - 1 - k))) {
                    w_new->stem = (char *) realloc(w_new->stem, strlen(w_new->stem) + strlen(words[k]) + 3);
                    strcat(w_new->stem, words[k]);
                    strcat(w_new->stem, ", ");
                }
            }
            
            // remove final ', '
            *(w_new->stem + strlen(w_new->stem) - 1) = '\0';
            *(w_new->stem + strlen(w_new->stem) - 1) = '\0';
            
            // update pointer to this group
            if (!last_flag) {
                // first result document: set as first element of linked list
                result->words = w_new;
            } else {
                w->next = w_new;
            }
            
            p = w;
            w = w_new;
            last_flag = found[i].flag;
        }
        
        // add document to group
        w = (indexed_word_p) realloc(w, sizeof(indexed_word_t) + sizeof(int) * (w->nr_docs + 1));
        w->documents[w->nr_docs] = i;
        w->nr_docs++;
        
        // copy name of the document into result index
        char *d = index->documents[found[i].doc_id];
        result->documents[i] = (char *) malloc(strlen(d) + 1);
        memcpy(result->documents[i], d, strlen(d) + 1);
        result->nr_docs++;
        
        // update pointer to this group (needed after realloc)
        if (!p) {
            result->words = w;
        } else {
            p->next = w;
        }
    }
    
    // free memory of the search terms
    for (i = 0; i < nr_words; i++) {
        free(words[i]);
    }
    
    return result;
}

/*
 * Compares two doc_found structs based on the number of found search terms (1st priority) and the first found search term (2nd priority)
 * Used for sorting documents of search results descending of their importance
 */
int cmp_doc_found_desc(const void *a, const void *b) {
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

    // rescan every document
    int i;
    for (i = 0; i < index->nr_docs; i++) {
        parse_file_for_index(index, index->documents[i]);
    }

    // save
    write_index_to_file(index);
}

/*
 * Parses a file and adds its words to the index
 */
void parse_file_for_index(index_p index, char *file) {
    // open file or print error message
    FILE *f = fopen(file, "r");
    if (!f) {
        printf("Cannot open %s!\nIndex not updated.\n", file);
        return;
    }

    // document id = index of document in list of all documents in filebase (alphabetically ordered)
    int doc_id = find_str(index->documents, file, 0, index->nr_docs-1);
    
    if (doc_id < 0) {
        printf("Error: %s is not in the filebase!\n", file);
        return;
    }

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
            
            if (!strlen(word_stem)) {
                word = strtok(NULL, " ");
                continue;
            }

            // insert document into index / add new stem to index
            indexed_word_p w = index->words;    // current word
            indexed_word_p p = NULL;            // previous word
            int flag = 0;
            while (w && !flag) {
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
                        flag = 0;
                        break;
                    } else if (w->documents[i] > doc_id) {
                        break;
                    }
                }
                
                // only add document to list if it's not already in the list
                if (flag) {
                    w = (indexed_word_p) realloc(w, sizeof(indexed_word_t) + sizeof(int) * (w->nr_docs + 1));
                    
                    // update pointer to this group (needed after realloc)
                    if (!p) {
                        index->words = w;
                    } else {
                        p->next = w;
                    }

                    // insert document in list
                    memcpy(&w->documents[i+1], &w->documents[i], sizeof(int) * (w->nr_docs - i));
                    w->documents[i] = doc_id;
                    w->nr_docs++;
                }
                
                free(word_stem);
            } else {
                // stem is not indexed, add it to index
                w = (indexed_word_p) malloc(sizeof(indexed_word_t) + sizeof(int));
                w->stem = word_stem;
                w->nr_docs = 1;
                w->documents[0] = doc_id;

                // insert this word in linked list
                if (!p) {
                    w->next = index->words;
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

    // each line contains the name (relative path) to one document in the filebase
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
    // format: <stem>:<n>:doc_id_1|doc_id_2|..|doc_id_n
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

    indexed_word_p p = NULL;
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
        if (!p) {
            index->words = w;
            w->next = NULL;
        } else {
            w->next = p->next;
            p->next = w;
        }
        p = w;

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
        if (cmp > 0 && min != middle) {
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
