#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _REPLACE_SUFFIX(__w, __s, __r) \
BEGIN_REPLACE_SUFFIX(__w, __s, __r); \
_END_REPLACE_SUFFIX

#define _REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
_BEGIN_REPLACE_SUFFIX_COND(__w, __s, __r, __c); \
_END_REPLACE_SUFFIX

#define _BEGIN_REPLACE_SUFFIX(__w, __s, __r) \
if (ends_with(__w, __s)) { \
    __w = replace_suffix(__w, strlen(__s), __r);

#define _BEGIN_REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
if (__c && ends_with(__w, __s)) {\
    __w = replace_suffix(__w, strlen(__s), __r);

#define _ALT_REPLACE_SUFFIX(__w, __s, __r) \
} else _BEGIN_REPLACE_SUFFIX(__w, __s, __r);

#define _ALT_REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
} else if (__c && ends_with(__w, __s)) { \
    __w = replace_suffix(__w, strlen(__s), __r);

#define _END_REPLACE_SUFFIX }

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
    return word[l-1] == word[l-2] && is_consonant(word, l-1);
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
    return (        parts - first_type         -    (parts - first_type)%2    ) / 2;
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
    _BEGIN_REPLACE_SUFFIX(word, "sses", "ss");
    _ALT_REPLACE_SUFFIX(word, "ies", "i");
    _ALT_REPLACE_SUFFIX_COND(word, "s", "", !ends_with(word, "ss"));
    _END_REPLACE_SUFFIX
    
    int vowel = contains_vowel(word);
    int cont = 0;
    
    /* STEP 1b */
    _BEGIN_REPLACE_SUFFIX_COND(word, "eed", "ee", m > 0);
    _ALT_REPLACE_SUFFIX_COND(word, "ing", "", vowel);
        cont = 1;
    _ALT_REPLACE_SUFFIX_COND(word, "ed", "", vowel);
        cont = 1;
    _END_REPLACE_SUFFIX
    
    int l = strlen(word) - 1;
    if (cont) {
        _BEGIN_REPLACE_SUFFIX(word, "at", "ate");
        _ALT_REPLACE_SUFFIX(word, "bl", "ble");
        _ALT_REPLACE_SUFFIX(word, "iz", "ize");
        _ALT_REPLACE_SUFFIX_COND(word, "", "", ends_with_double_consonant(word) && word[l] != 'l' && word[l] != 's' && word[l] != 'z');
            word[l] = '\0';
        _ALT_REPLACE_SUFFIX_COND(word, "", "e", m == 1 && ends_with_cvc(word));
        _END_REPLACE_SUFFIX
    }
    
    /* STEP 1c */
    _REPLACE_SUFFIX_COND(word, "y", "i", contains_vowel(word));
    
    /* STEP 2 */
    if (m > 0) {
        _BEGIN_REPLACE_SUFFIX(word, "ational", "ate");
        _ALT_REPLACE_SUFFIX(word, "tional", "tion");
        _ALT_REPLACE_SUFFIX(word, "enci", "ence");
        _END_REPLACE_SUFFIX
    }
}

int main() {
    return 0;
}
