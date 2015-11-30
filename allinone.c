#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_SEARCH_RESULTS 10

typedef struct indexed_word {
    struct indexed_word *next;          // next indexed word
    char *stem;                         // stem of this word
    int nr_docs;                        // number of documents in filebase containing this word or variations of it
    int documents[];                    // list of these documents' index in the filebase
} indexed_word_t, *indexed_word_p;

typedef struct index {
   indexed_word_p words;                // linked list of indexed words
   int nr_docs;                         // number of documents in the filebase
   char *documents[];                   // list of the names of the documents in the filebase
} index_t, *index_p;

index_p add_file(index_p db, char *file);
void remove_file(index_p db, char *file);
index_p search_index(index_p, char *query);
void rebuild_index(index_p index);
index_p load_index();
void close_index(index_p db);

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

#define _REPLACE_SUFFIX(__w, __s, __r) \
BEGIN_REPLACE_SUFFIX(__w, __s, __r) \
_END_REPLACE_OR

#define _REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
_BEGIN_REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
_END_REPLACE_OR

#define _BEGIN_REPLACE_SUFFIX(__w, __s, __r) \
if (ends_with(__w, __s)) { \
    __w = replace_suffix(__w, strlen(__s), __r);

#define _BEGIN_REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
if (ends_with(__w, __s)) { \
    if (__c) { \
        __w = replace_suffix(__w, strlen(__s), __r); \
    _END_REPLACE_COND

#define _OR_REPLACE_SUFFIX(__w, __s, __r) \
} else _BEGIN_REPLACE_SUFFIX(__w, __s, __r)

#define _OR_REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
_OR_REPLACE_SUFFIX_COND_NE(__w, __s, __r, __c) \
    _END_REPLACE_COND

#define _OR_REPLACE_SUFFIX_COND_NE(__w, __s, __r, __c) \
} else if (ends_with(__w, __s)) { \
    if (__c) { \
        __w = replace_suffix(__w, strlen(__s), __r);

#define _END_REPLACE_COND }
#define _END_REPLACE_OR }

int calculate_m(char *word, int suffix_len);
int is_consonant(char *word, int i);
int contains_vowel(char *word, int suffix_len);
int ends_with(char *word, char *suffix);
int ends_with_double_consonant(char *word);
int ends_with_cvc(char *word, int suffix_len);
char *replace_suffix(char *word, int suffix_length, char *replacement);

char *stem(char *word);

char *read_line(FILE *ptr);
void nonalpha_to_space(char *str);
int starts_with(char *str, char *pre);


int main(int argc, void *argv) {
    load_stopwords();
    index_p index = load_index();

    int exit = 0;
    while (!exit) {
        printf(" > ");
        char *command = read_line(stdin);

        if (!strcmp(command, "exit")) {
            // exit command
            exit = 1;
            printf("Exit requested..\n");

		} else if (!strcmp(command, "rebuild index")) {
            // rebuild index command
            rebuild_index(index);
		} else if (starts_with(command, "search for ")) {
            // search for <search_query> command
            char *query = (char *) malloc(strlen(command) - 10);
            memcpy(query, command+11, strlen(command) - 10);

            index_p result = search_index(index, query);

            printf("Results (showing no more than 10, there might be more):");
            if (result) {
                // print result
                int count = 0;
                indexed_word_p w = result->words;
                if (!w) {
                    printf("No documents found for search term %s\n", query);
                }
                
                while (w) {
                    printf("Documents containing %s:\n", w->stem);

                    int i;
                    for (i = 0; i < w->nr_docs; i++, count++) {
                        printf(" [%2d] %s\n", count, result->documents[w->documents[i]]);
                    }

                    w = w->next;
                }

                close_index(result);
            } else {
                printf("No documents found for search term %s\n", query);
            }

            free(query);

        } else if (starts_with(command, "add file ")) {
            // add file <file> command
            char *file = (char*) malloc(strlen(command) - 8);
            memcpy(file, command+9, strlen(command) - 8);

			index = add_file(index, file);
            free(file);

        } else if (starts_with(command, "remove file ")) {
            // remove file <file> command
            char *file = (char*) malloc(strlen(command) - 11);
            memcpy(file, command+12, strlen(command) - 11);

            remove_file(index, file);
            free(file);
        }
        
        free(command);
    }
    


    // release memory
    release_stopwords();
    close_index(index);

    return 0;
}

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

/*
 * Calculates m value for a word
 */
int calculate_m(char *word, int suffix_len) {
    if (strlen(word) <= suffix_len) {
        return 0;
    }
    
    int parts = 1;

    // type (consonant = 1, vowel = 0) of first letter
    char first_type = is_consonant(word, 0);

    // type of last checked letter
    char tmp_type = first_type;

    int i = 0;
    while (i < strlen(word) - suffix_len) {
        // if last checked type is different from current type, we enter a new part of the word
        if (tmp_type != is_consonant(word, i)) {
            tmp_type = !tmp_type;
            parts++;
        }
        i++;
    }

    //       remove optional initial consonant   remove optional vowel ending
    return (        parts - first_type         -    (parts - first_type)%2    ) / 2;
}

/*
 * Checkes whether i-th character of word is a consonant
 */
int is_consonant(char *word, int i) {
    char c = tolower(word[i]);
    return c != 'a' && c != 'e' && c != 'i' && c != 'o' & c != 'u' && !(c == 'y' && i > 0 && is_consonant(word, i-1));
}

/*
 * Checks whether word contains a vowel
 */
int contains_vowel(char *word, int suffix_len) {
    if (strlen(word) <= suffix_len) {
        return 0;
    }
    
    int i = 0;
    while (i < strlen(word) - suffix_len) {
        if (!is_consonant(word, i)) {
            return 1;
        }

        i++;
    }

    return 0;
}

/*
 * Checks wether a word ends with a specific suffix
 */
int ends_with(char *word, char *suffix) {
    return strlen(word) >= strlen(suffix) && !memcmp(word + strlen(word) - strlen(suffix), suffix, strlen(suffix));
}

/*
 * Checks whether word ends with a double consonant
 */
int ends_with_double_consonant(char *word) {
    int l = strlen(word);
    return word[l-1] == word[l-2] && is_consonant(word, l-1);
}

/*
 * Checks whether word ands with a consonant-vowel-consonant combination where the last consonant is not W, X or Y
 */
int ends_with_cvc(char *word, int suffix_len) {
    if (strlen(word) <= suffix_len) {
        return 0;
    }
    
    int l = strlen(word) - 1 - suffix_len;
    return is_consonant(word, l-2) && !is_consonant(word, l-1) && is_consonant(word, l) && word[l] != 'w' && word[l] != 'x' && word[l] != 'y';
}

/*
 * Replaces a suffix of a word
 */
char *replace_suffix(char *word, int suffix_length, char *replacement) {
    if (suffix_length < strlen(replacement)) {
        word = (char *) realloc(word, strlen(word) - suffix_length + strlen(replacement) + 1);
    }

    // copy whole replacement string, including \0 string terminator
    memcpy(word + strlen(word) - suffix_length, replacement, strlen(replacement) + 1);
    return word;
}

/*
 * Runs Porter Stemming Algorithm on a word
 */
char *stem(char *word) {
    // copy word to new memory location
    char *result = (char *) malloc(strlen(word) + 1);
    memcpy(result, word, strlen(word) + 1);

    /* STEP 1a */
    _BEGIN_REPLACE_SUFFIX(result, "sses", "ss")
    _OR_REPLACE_SUFFIX(result, "ies", "i")
    _OR_REPLACE_SUFFIX(result, "ss", "ss")
    _OR_REPLACE_SUFFIX(result, "s", "")
    _END_REPLACE_OR

    /* STEP 1b */
    int cont = 0;
    int vowel = contains_vowel(result, 3);
    _BEGIN_REPLACE_SUFFIX_COND(result, "eed", "ee", calculate_m(result, 3) > 0)
    _OR_REPLACE_SUFFIX_COND_NE(result, "ing", "", vowel)
        cont = 1;
    _END_REPLACE_COND
    _OR_REPLACE_SUFFIX_COND_NE(result, "ed", "", vowel || !is_consonant(result, strlen(result) - 3))
        cont = 1;
    _END_REPLACE_COND
    _END_REPLACE_OR

    int l = strlen(result) - 1;
    if (cont) {
        _BEGIN_REPLACE_SUFFIX(result, "at", "ate")
        _OR_REPLACE_SUFFIX(result, "bl", "ble")
        _OR_REPLACE_SUFFIX(result, "iz", "ize")
        _OR_REPLACE_SUFFIX_COND_NE(result, "", "", ends_with_double_consonant(result) && result[l] != 'l' && result[l] != 's' && result[l] != 'z')
            // removes the last consonant of the consonant pair at the end of the word
            result[l] = '\0';
        _END_REPLACE_COND
        _OR_REPLACE_SUFFIX_COND(result, "", "e", calculate_m(result, 0) == 1 && ends_with_cvc(result, 0))
        _END_REPLACE_OR
    }

    /* STEP 1c */
    _REPLACE_SUFFIX_COND(result, "y", "i", contains_vowel(result, 1))

    /* STEP 2 */
    int m7 = calculate_m(result, 7);
    int m6 = m7 > 0 ? m7 : calculate_m(result, 6);
    int m5 = m6 > 0 ? m6 : calculate_m(result, 5);
    int m4 = m5 > 0 ? m5 : calculate_m(result, 4);
    int m3 = m4 > 0 ? m4 : calculate_m(result, 3);
    _BEGIN_REPLACE_SUFFIX_COND(result, "ational", "ate", m7 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "tional", "tion", m6 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "enci", "ence", m4 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "anci", "ance", m4 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "izer", "ize", m4 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "abli", "able", m4 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "alli", "al", m4 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "entli", "ent", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "eli", "e", m3 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ousli", "ous", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ization", "ize", m7 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ation", "ate", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ator", "ate", m4 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "alism", "al", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ivenes", "ive", m6 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "fulness", "ful", m7 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ousness", "ous", m7 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "aliti", "al", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "iviti", "ive", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "biliti", "ble", m6 > 0)
    _END_REPLACE_OR

    /* STEP 3 */
    m5 = calculate_m(result, 5);
    m4 = m5 > 0 ? m5 : calculate_m(result, 4);
    m3 = m4 > 0 ? m4 : calculate_m(result, 3);
    _BEGIN_REPLACE_SUFFIX_COND(result, "icate", "ic", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ative", "", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "alize", "al", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "aciti", "ic", m5 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ical", "ic", m4 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ful", "", m3 > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ness", "", m4 > 0)
    _END_REPLACE_OR

    /* STEP 4 */
    l = strlen(result) - 1;
    m5 = calculate_m(result, 5);
    m4 = m5 > 1 ? m5 : calculate_m(result, 4);
    m3 = m4 > 1 ? m4 : calculate_m(result, 3);
    int m2 = m3 > 1 ? m3 : calculate_m(result, 2);
    _BEGIN_REPLACE_SUFFIX_COND(result, "al", "", m2 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ance", "", m4 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ence", "", m4 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "er", "", m2 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ic", "", m2 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "able", "", m4 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ible", "", m4 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ant", "", m3 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ement", "", m5 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ment", "", m4 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ent", "", m3 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ion", "", result[l-3] == 's' || result[l-3] == 't')
    _OR_REPLACE_SUFFIX_COND(result, "ou", "", m2 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ism", "", m3 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ate", "", m3 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "iti", "", m3 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ous", "", m3 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ive", "", m3 > 1)
    _OR_REPLACE_SUFFIX_COND(result, "ize", "", m3 > 1)
    _END_REPLACE_OR

    /* STEP 5a */
    int m1 = calculate_m(result, 1);
    _REPLACE_SUFFIX_COND(result, "e", "", m1 > 1 || (m1 == 1 && !ends_with_cvc(result, 1)))

    /* STEP 5b */
    _REPLACE_SUFFIX_COND(result, "l", "", calculate_m(result, 0) > 1 && ends_with_double_consonant(result))

    return result;
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
    
    // remove newline character at the end of the string
    if (*(line - 1) == '\n') {
        *(line - 1) = '\0';
        
        // special case Windows (DOH!) -> remove \r as well
        if (*(line - 2) == '\r') {
            *(line - 2) = '\0';
        }
    }
    
    return linep;
}

/*
 * Turns non alpha characters into spaces
 */
void nonalpha_to_space(char *str) {
    char *c;
    for(c = str; *c; c++) {
        if(!isalpha(*c)) {
            *c = ' ';
        } else {
            *c = tolower(*c);
        }
    }
}

/*
 * Checks if pre is a prefix of str
 */
int starts_with(char *str, char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}
