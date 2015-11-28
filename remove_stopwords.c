#include <stdio.h>
#include <string.h>

/*
*turns given file of stopwords into an array, which is then
*used to remove given stopwords from another file
*/
char* file_to_array(FILE *fp)
{
	
	FILE *fr;			//file pointer
	char buf[20];		//buffer for chars optained per line
	char * pch;			//char pointer to be used in parsin
		
	fr = fopen("stopwords", "r");			//opens file in read mode
	if (!fr)								//if a file is not given, return 1
		return 1;

	int ch;
	int num_lines;
	while (EOF != (ch = getchar()))
	{
		if (ch == '\n')
			++num_lines;
	}
	char *stopwords[num_lines];	//since each word is a line, the stopwords array should be as big as lines in the file

	int i=0; //used as array index
		
	while (fgets(buf, 20, fr) != NULL)	//gets each line (word) from the file
	{
		printf("%s", buf);		//prints the line from file
		stopwords[i] = buf; 
	}

	fclose(fr);

	return stopwords;
}
/*
* removes stopwords found in stopwords file (which are turned into an array) from file fp
*/

void remove_stopwords(FILE *fp, FILE *stopwords)
{
	char *stopwords = file_to_array("stopwords");
	int i;
	for (i == 0;i < stopwords.size();i++)
	{
		//TODO: remove word from file
	}
}