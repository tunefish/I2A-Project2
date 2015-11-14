#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
* used source code from https://www.cs.cmu.edu/~callan/Teaching/porter.c
*/

#define TRUE 1
#define FALSE 0

static char * b;		//buffer for word to be stemmed
static int k, k0, j;	//j is a general offset into the string

//checks if b[i] is a consonant
int cons(int i)
{
	switch (b[i])
	{
	case 'a': case 'e': case'i': case 'o': case 'u': return FALSE;
	case 'y': return (i == k0) ? TRUE : !cons(i - 1);
	default: return TRUE;
	}
}

/*m() measures the number of consonant sequences between k0 and j, 
where c is a consonant sequence and v is a vowel sequence, and ... is arbitrary
such that

<c><v>			give 0
<c>vc<v>		gives 1
<c>vcvc<v>		gives 2
<c>vcvcvc<v>	gives 3
 ...
 */

int m()
{
	int n = 0;
	int i = k0;
	while (TRUE)
	{
		if (i > j) return n;
		if (!cons(i)) break; i++;
	}
	i++;
	while (TRUE)
	{
		while (TRUE)
		{
			if (i > j) return n;
			if (cons(i)) break;
			i++;
		}
		i++;
		n++;
		while (TRUE)
		{
			if (i > j) return n;
			if (!cons(i)) break;
			i++;
		}
		i++;
	}
}

//if vowelinstem() is TRUE, k0,...j, contains a vowel
int vowelinstem()
{
	int i;
	for (i = k0; i <= j; i++)
	{
		if (!cons(i)) return TRUE;
		else return FALSE;
	}
}

//if doublec(j) is true, then j,(j-1) contains a double consonant
int doublec(int j)
{
	if (j < k0 + 1) return FALSE;
	if (b[j] != b[j - 1]) return FALSE;
	return cons(j);
}

//if cvc(i) is TRUE, then i-2,i-1,i has the form consonant-vowel-consonant
//and the second c is not w,x, or y (used when trying to restore e to certain words)
int cvc(int i)
{
	if (i < k0 + 2 || !cons(i) || cons(i - 1) || !cons(i - 2)) return FALSE;
	{
		int ch = b[i];
		if (ch == 'w' || ch == 'x' || ch == 'y') return FALSE;
	}
	return TRUE;
}

//ends(s) is TRUE when when k0,...,k ends with the string s
int ends(char * s)
{
	int length = s[0];
	if (s[length] != b[k]) return FALSE;
	if (length > k - k0 + 1) return FALSE;
	if (memcmp(b + k - length + 1, s + 1, length) != 0) return FALSE;
	j = k - length;
	return TRUE;
}

//setto(s) sets (j+1),...k to the characters in the string s, readjusting k
void setto(char *s)
{
	int length = s[0];
	memmove(b + j + 1, s + 1, length);
	k = j + length;
}

//r(s) will be used later...
void r(char *s)
{
	if (m() > 0) setto(s);
}

/*
step1ab() gets rid of plurals and -ed or -ing endingds
eg.
caresses  ->  caress
ponies    ->  poni
ties      ->  ti
caress    ->  caress
cats      ->  cat

feed      ->  feed
agreed    ->  agree
disabled  ->  disable

matting   ->  mat
mating    ->  mate
meeting   ->  meet
milling   ->  mill
messing   ->  mess

meetings  ->  meet

*/
void step1ab()
{
	if (b[k] == 's')
	{
		if (ends("\04" "sses")) k -= 2; 
		else if (ends("\03" "ies")) setto("\01" "i"); 
		else if (b[k - 1] != 's') k--;
	}
	if (ends("\03" "eed")) { if (m() > 0) k--; } 
	else if ((ends("\02" "ed") || ends("\03" "ing")) && vowelinstem())
	{
		k = j;
		if (ends("\02" "at")) setto("\03" "ate"); 
		else if (ends("\02" "bl")) setto("\03" "ble"); 
		else if (ends("\02" "iz")) setto("\03" "ize"); 
		else if (doublec(k))
		{
			k--;
			{
				int ch = b[k];
				if (ch == 'l' || ch == 's' || ch == 'z') k++;
			}
		}
		else if (m() == 1 && cvc(k)) setto("\01" "e");
	}
}

//step1c() turns terminal y to i when there is another vowel in stem
void step1c() 
{
	if (ends("\01" "y") && vowelinstem()) 
		b[k] = "i";
}

//step2() maps double sufficies to single ones
void step2() {
	switch (b[k - 1])
	{
	case 'a': if (ends("\07" "ational")) { r("\03" "ate"); break; }
			  if (ends("\06" "tional")) { r("\04" "tion"); break; }
			  break;
	case 'c': if (ends("\04" "enci")) { r("\04" "ence"); break; }
			  if (ends("\04" "anci")) { r("\04" "ance"); break; }
			  break;
	case 'e': if (ends("\04" "izer")) { r("\03" "ize"); break; }
			  break;
	case 'l': if (ends("\03" "bli")) { r("\03" "ble"); break; }
			  if (ends("\04" "alli")) { r("\02" "al"); break; }
			  if (ends("\05" "entli")) { r("\03" "ent"); break; }
			  if (ends("\03" "eli")) { r("\01" "e"); break; }
			  if (ends("\05" "ousli")) { r("\03" "ous"); break; }
			  break;
	case 'o': if (ends("\07" "ization")) { r("\03" "ize"); break; }
			  if (ends("\05" "ation")) { r("\03" "ate"); break; }
			  if (ends("\04" "ator")) { r("\03" "ate"); break; }
			  break;
	case 's': if (ends("\05" "alism")) { r("\02" "al"); break; }
			  if (ends("\07" "iveness")) { r("\03" "ive"); break; }
			  if (ends("\07" "fulness")) { r("\03" "ful"); break; }
			  if (ends("\07" "ousness")) { r("\03" "ous"); break; }
			  break;
	case 't': if (ends("\05" "aliti")) { r("\02" "al"); break; }
			  if (ends("\05" "iviti")) { r("\03" "ive"); break; }
			  if (ends("\06" "biliti")) { r("\03" "ble"); break; }
			  break;
	case 'g': if (ends("\04" "logi")) { r("\03" "log"); break; }
	}
}

//step3() deals with -ic-, -full, -ness, etc. Similar to step 2
void step3() {
	switch (b[k])
	{
	case 'e': if (ends("\05" "icate")) { r("\02" "ic"); break; }
			  if (ends("\05" "ative")) { r("\00" ""); break; }
			  if (ends("\05" "alize")) { r("\02" "al"); break; }
			  break;
	case 'i': if (ends("\05" "iciti")) { r("\02" "ic"); break; }
			  break;
	case 'l': if (ends("\04" "ical")) { r("\02" "ic"); break; }
			  if (ends("\03" "ful")) { r("\00" ""); break; }
			  break;
	case 's': if (ends("\04" "ness")) { r("\00" ""); break; }
			  break;
	}
}

//step4() takes off -ant, -ence, etc in context of <c>vcvc<v>
void step4() {
	switch (b[k - 1])
	{
	case 'a': if (ends("\02" "al")) break; return;
	case 'c': if (ends("\04" "ance")) break;
		if (ends("\04" "ence")) break; return;
	case 'e': if (ends("\02" "er")) break; return;
	case 'i': if (ends("\02" "ic")) break; return;
	case 'l': if (ends("\04" "able")) break;
		if (ends("\04" "ible")) break; return;
	case 'n': if (ends("\03" "ant")) break;
		if (ends("\05" "ement")) break;
		if (ends("\04" "ment")) break;
		if (ends("\03" "ent")) break; return;
	case 'o': if (ends("\03" "ion") && (b[j] == 's' || b[j] == 't')) break;
		if (ends("\02" "ou")) break; return;
		/* takes care of -ous */
	case 's': if (ends("\03" "ism")) break; return;
	case 't': if (ends("\03" "ate")) break;
		if (ends("\03" "iti")) break; return;
	case 'u': if (ends("\03" "ous")) break; return;
	case 'v': if (ends("\03" "ive")) break; return;
	case 'z': if (ends("\03" "ize")) break; return;
	default: return;
	}
	if (m() > 1) k = j;
}

//step5() removes a final -e if m()>1 and changes -ll to -l if m()>1
void step5()
{
	j = k;
	if (b[k] == 'e')
	{
		int a = m();
		if (a > 1 || a == 1 && !cvc(k - 1)) k--;
	}
	if (b[k] == 'l' && doublec(k) && m() > 1) k--;
}

//stem(p,i,j) uses p as a char pointer and the string is to be stemmed from 
//p[i] to p[j]
int stem(char* p, int i, int j)
{
	b = p;
	k = j;
	k0 = i;
	if (k <= k0 + 1) return k; //strings of length 1 and 2 will not be stemmed
	step1ab();
	step1c();
	step2();
	step3();
	step4();
	step5();
	return k; //stemmed word
}