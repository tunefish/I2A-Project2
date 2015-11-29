#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stemmer.h"

#define _REPLACE_SUFFIX(__w, __s, __r) \
BEGIN_REPLACE_SUFFIX(__w, __s, __r) \
_END_REPLACE_SUFFIX

#define _REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
_BEGIN_REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
_END_REPLACE_SUFFIX

#define _BEGIN_REPLACE_SUFFIX(__w, __s, __r) \
if (ends_with(__w, __s)) { \
    __w = replace_suffix(__w, strlen(__s), __r);

#define _BEGIN_REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
if (__c && ends_with(__w, __s)) {\
    __w = replace_suffix(__w, strlen(__s), __r);

#define _OR_REPLACE_SUFFIX(__w, __s, __r) \
} else _BEGIN_REPLACE_SUFFIX(__w, __s, __r)

#define _OR_REPLACE_SUFFIX_COND(__w, __s, __r, __c) \
} else if (__c && ends_with(__w, __s)) { \
    __w = replace_suffix(__w, strlen(__s), __r);

#define _END_REPLACE_SUFFIX }

int calculate_m(char *word);
int is_consonant(char *word, int i);
int contains_vowel(char *word);
int ends_with(char *word, char *suffix);
int ends_with_double_consonant(char *word);
int ends_with_cvc(char *word);
char *replace_suffix(char *word, int suffix_length, char *replacement);

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
 * Checks wether a word ends with a specific suffix
 */
int ends_with(char *word, char *suffix) {
    return strlen(word) >= strlen(suffix) && memcmp(word + strlen(word) - strlen(suffix), suffix, strlen(suffix));
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
    // copy word to new memory location
    char *result = (char *) malloc(strlen(word) + 1);
    memcpy(result, word, strlen(word) + 1);

    int m = calculate_m(result);

    /* STEP 1a */
    _BEGIN_REPLACE_SUFFIX(result, "sses", "ss")
    _OR_REPLACE_SUFFIX(result, "ies", "i")
    _OR_REPLACE_SUFFIX_COND(result, "s", "", !ends_with(result, "ss"))
    _END_REPLACE_SUFFIX

    /* STEP 1b */
    int vowel = contains_vowel(result);
    int cont = 0;
    _BEGIN_REPLACE_SUFFIX_COND(result, "eed", "ee", m > 0)
    _OR_REPLACE_SUFFIX_COND(result, "ing", "", vowel)
        cont = 1;
    _OR_REPLACE_SUFFIX_COND(result, "ed", "", vowel)
        cont = 1;
    _END_REPLACE_SUFFIX

    int l = strlen(result) - 1;
    if (cont) {
        _BEGIN_REPLACE_SUFFIX(result, "at", "ate")
        _OR_REPLACE_SUFFIX(result, "bl", "ble")
        _OR_REPLACE_SUFFIX(result, "iz", "ize")
        _OR_REPLACE_SUFFIX_COND(result, "", "", ends_with_double_consonant(result) && result[l] != 'l' && result[l] != 's' && result[l] != 'z')
            result[l] = '\0';
        _OR_REPLACE_SUFFIX_COND(result, "", "e", m == 1 && ends_with_cvc(result))
        _END_REPLACE_SUFFIX
    }

    /* STEP 1c */
    _REPLACE_SUFFIX_COND(result, "y", "i", contains_vowel(result))

    /* STEP 2 */
    if (m > 0) {
        _BEGIN_REPLACE_SUFFIX(result, "ational", "ate")
        _OR_REPLACE_SUFFIX(result, "tional", "tion")
        _OR_REPLACE_SUFFIX(result, "enci", "ence")
        _OR_REPLACE_SUFFIX(result, "anci", "ance")
        _OR_REPLACE_SUFFIX(result, "izer", "ize")
        _OR_REPLACE_SUFFIX(result, "abli", "able")
        _OR_REPLACE_SUFFIX(result, "alli", "al")
        _OR_REPLACE_SUFFIX(result, "entli", "ent")
        _OR_REPLACE_SUFFIX(result, "eli", "e")
        _OR_REPLACE_SUFFIX(result, "ousli", "ous")
        _OR_REPLACE_SUFFIX(result, "ization", "ize")
        _OR_REPLACE_SUFFIX(result, "ation", "ate")
        _OR_REPLACE_SUFFIX(result, "ator", "ate")
        _OR_REPLACE_SUFFIX(result, "alism", "al")
        _OR_REPLACE_SUFFIX(result, "ivenes", "ive")
        _OR_REPLACE_SUFFIX(result, "fulness", "ful")
        _OR_REPLACE_SUFFIX(result, "ousness", "ous")
        _OR_REPLACE_SUFFIX(result, "aliti", "al")
        _OR_REPLACE_SUFFIX(result, "iviti", "ive")
        _OR_REPLACE_SUFFIX(result, "biliti", "ble")
        _END_REPLACE_SUFFIX
    }

    /* STEP 3 */
    if (m > 0) {
        _BEGIN_REPLACE_SUFFIX(result, "icate", "ic")
        _OR_REPLACE_SUFFIX(result, "ative", "")
        _OR_REPLACE_SUFFIX(result, "alize", "al")
        _OR_REPLACE_SUFFIX(result, "aciti", "ic")
        _OR_REPLACE_SUFFIX(result, "ical", "ic")
        _OR_REPLACE_SUFFIX(result, "ful", "")
        _OR_REPLACE_SUFFIX(result, "ness", "")
        _END_REPLACE_SUFFIX
    }

    /* STEP 4 */
    l = strlen(result) - 1;
    if (m > 1) {
        _BEGIN_REPLACE_SUFFIX(result, "al", "")
        _OR_REPLACE_SUFFIX(result, "ance", "")
        _OR_REPLACE_SUFFIX(result, "ence", "")
        _OR_REPLACE_SUFFIX(result, "er", "")
        _OR_REPLACE_SUFFIX(result, "ic", "")
        _OR_REPLACE_SUFFIX(result, "able", "")
        _OR_REPLACE_SUFFIX(result, "ible", "")
        _OR_REPLACE_SUFFIX(result, "ant", "")
        _OR_REPLACE_SUFFIX(result, "ement", "")
        _OR_REPLACE_SUFFIX(result, "ment", "")
        _OR_REPLACE_SUFFIX(result, "ent", "")
        _OR_REPLACE_SUFFIX_COND(result, "ion", "", result[l-3] == 's' || result[l-3] == 't')
        _OR_REPLACE_SUFFIX(result, "ou", "")
        _OR_REPLACE_SUFFIX(result, "ism", "")
        _OR_REPLACE_SUFFIX(result, "ate", "")
        _OR_REPLACE_SUFFIX(result, "iti", "")
        _OR_REPLACE_SUFFIX(result, "ous", "")
        _OR_REPLACE_SUFFIX(result, "ive", "")
        _OR_REPLACE_SUFFIX(result, "ize", "")
        _END_REPLACE_SUFFIX
    }

    /* STEP 5a */
    _REPLACE_SUFFIX_COND(result, "e", "", m > 1 || (m == 1 && ends_with_cvc(result)))

    /* STEP 5b */
    _REPLACE_SUFFIX_COND(result, "l", "", m > 1 && ends_with_double_consonant(result))

    return result;
}
