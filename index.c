#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*add file to database*/
void add_file(char *)
{
	//TODO: remove all stop words from file
	//TODO: stem all remaining words
	//TODO: create new indexed words
	//TODO: update already indexed words
	return 0;
}

/*remove file from database*/
void remove_file(char *)
{
	//TODO: search for indexed word containing file in database
	//TODO: remove file from *documents[] in indexed word
	return 0;
}

/*search database for indexed word. Return documents containing word*/
char search_database(char *)
{
	//TODO: search database for indexed word
	//TODO: return *documents[] from indexed word
	return char;
}

/*rebuild list of indexed words and documents associated with them*/
void rebuild_index()
{
	//TODO:
}

/*load a database that already contains indexed words*/
database_p load_database()
{
	//TODO: 
	return database_p;
}