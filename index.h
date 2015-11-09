typedef struct database {
   int count:
   indexed_word_t words;
} database_t, *database_p;

typedef struct indexed_word {
    struct indexed_words *next;
    char *documents[];
} indexed_word_t, *indexed_word_p;

int add_file(char *);
int remove_file(char *);
int search_database(char *);
void rebuild_index();
database_p load_database();
