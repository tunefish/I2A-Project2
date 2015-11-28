typedef struct indexed_word {
    struct indexed_word *next;
    int nr_docs;
    char *stem;
    char *documents[];
} indexed_word_t, *indexed_word_p;

typedef struct database {
   int count;
   indexed_word_p words;
} database_t, *database_p;

void add_file(char * file);
void remove_file(char * file);
indexed_word_p search_database(char * query);
void rebuild_index();
database_p load_database();
void close_database(database_p db);
