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
