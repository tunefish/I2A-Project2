#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int k, k0, j;	//j is a general offset into the string

/*
 * Checkes whether i-th character of word is a consonant
 */
int is_consonant(char *word, int i)
{
    char c = tolower(word[i]);
    return c != 'a' && c != 'e' && c != 'i' && c != 'o' & c != 'u' && !(c == 'y' && i > 0 && is_consonant(word, i-1));
}

/*
 * Checks whether word contains a vowel
 */
int contains_vowel(char *word) {
    int i = 0;
    while (i < strlen(word)) {
        if (!is_consonant(word, i)) {
            return 1;
        }
        
        i++;
    }
    
    return 0;
}

/*
 * Checks whether word ends with a double consonant
 */
int ends_with_double_consonant(char *word) {
    int l = strlen(word);
    return word[l-1] == word[l-2] && is_consonant(word, l-1)
}

/*
 * Checks whether word ands with a consonant-vowel-consonant combination where the last consonant is not W, X or Y
 */
int ends_with_cvc(char *word) {
    int l = strlen(word) - 1;
    return is_consonant(word, l-2) && !is_consonant(word, l-1) && is_consonant(word, l) && word[l] != 'w' && word[l] != 'x' && word[l] != 'y';
}

/*
 * Calculates m value for a word
 */
int calculate_m(char *word) {
    int parts = 1;
    
    // type (consonant = 1, vowel = 0) of first letter
    char first_type = is_consonant(word, 0);
    
    // type of last checked letter
    char tmp_type = first_type;
    
    int i = 0;
    while (i < strlen(word)) {
        // if last checked type is different from current type, we enter a new part of the word
        if (tmp_type != is_consonant(word, i)) {
            tmp_type = !tmp_type;
            parts++;
        }
        i++;
    }
    
    //       remove optional initial consonant   remove optional vowel ending
    return (           n - first_type          -      (n - first_type)%2      ) / 2;
}

/*
 * Checks wether a word ends with a specific suffix
 */
int ends_with(char *word, char *suffix) {
    return strlen(word) >= strlen(suffix) && memcmp(word + strlen(word) - strlen(suffix), suffix, strlen(suffix));
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
    int m = calculate_m(word);
    
    /* STEP 1a */
    if (ends_with(word, "sses")) {                                                  // sses -> ss
        word = replace_suffix(word, 4, "ss");
    } else if (ends_with(word, "ies")) {                                            // ies  -> i
        word = replace_suffix(word, 3, "i");
    } else if (!ends_with(word, "ss") && ends_with(word, "s")) {                    // s    ->
        word = replace_suffix(word, 1, "");
    }
    
    int vowel = contains_vowel(word);
    int cont = 0;
    
    /* STEP 1b */
    if (m > 0 && ends_with(word, "eed")) {                                          // eed  -> ee
        word = replace_suffix(word, 3, "ee");
    } else if (vowel && ends_with(word, "ing")) {                                   // ing  -> ed
        word = replace_suffix(word, 3, "");
        cont = 1;
    } else if (vowel && ends_with(word, "ed")) {                                    // ed   ->
        word = replace_suffix(word, 2, "");
        cont = 1;
    }
    
    int l = strlen(word) - 1;
    if (cont) {
        if (ends_with(word, "at")) {                                                // at   -> ate
            word = replace_suffix(word, 2, "ate");
        } else if (ends_with(word, "bl")) {                                         // bl   -> ble
            word = replace_suffix(word, 2, "ble");
        } else if (ends_with(word, "iz")) {                                         // iz   -> ize
            word = replace_suffix(word, 2, "ize");
        } else if (ends_with_double_consonant(word) && word[l] != 'l' && word[l] != 's' && word[l] != 'z') {      // double consonant -> single consonant
            word[l-1] = '\0';
        } else if (m == 1 && ends_with_cvc(word)) {                                 // *CVC -> e
            word = replace_suffix(word, 0, "e");
        }
    }
    
    /* STEP 1c */
    if (contains_vowel(word) && ends_with(word, "y")) {
        word = replace_suffix(word, 1, "i");
    }
    
    /* STEP 2 */
}

//r(s) will be used later...
void r(char *s)
{
	if (m() > 0) setto(s);
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