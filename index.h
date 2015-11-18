typedef struct database {
   int count:
   indexed_word_t words;
} database_t, *database_p;

typedef struct indexed_word {
    struct indexed_words *next;
    char *documents[];
} indexed_word_t, *indexed_word_p;

void add_file(char *);
void remove_file(char *);
char search_database(char *);
void rebuild_index();
database_p load_database();

