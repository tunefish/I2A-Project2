typedef struct indexed_word {
    struct indexed_word *next;
    int nr_docs;
    char *stem;
    char *documents[];
} indexed_word_t, *indexed_word_p;

typedef struct index {
   int count;
   indexed_word_p words;
} index_t, *index_p;

void add_file(index_p db, char *file);
void remove_file(index_p db, char *file);
index_p search_index(index_p, char *query);
index_p rebuild_index();
index_p load_index();
void close_index(index_p db);
