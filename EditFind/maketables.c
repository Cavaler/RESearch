#include <pcre\config.h>
#include <pcre\pcre_internal.h>
#include <Plugin.hpp>

#undef islower
#undef isupper
#undef isalpha
#undef isalnum

#define tolower(i) FARToLower(pTable, i)
#define toupper(i) FARToUpper(pTable, i)
#define islower(i) FARIsLower(pTable, i)
#define isupper(i) FARIsUpper(pTable, i)
#define isalpha(i) FARIsAlpha(pTable, i)
#define isalnum(i) (FARIsAlpha(pTable, i) || isdigit(i))

int FARToLower(struct CharTableSet *pTable, int c) {
	return pTable->LowerTable[c];
}

int FARToUpper(struct CharTableSet *pTable, int c) {
	return pTable->UpperTable[c];
}

int FARIsAlpha(struct CharTableSet *pTable, int c) {
	return pTable->LowerTable[c] != pTable->UpperTable[c];
}

int FARIsLower(struct CharTableSet *pTable, int c) {
	return (!FARIsAlpha(pTable, c)) ? 0 :
		(pTable->LowerTable[c] == c) ? 1 : 0;
}

int FARIsUpper(struct CharTableSet *pTable, int c) {
	return (!FARIsAlpha(pTable, c)) ? 0 :
		(pTable->UpperTable[c] == c) ? 1 : 0;
}


const unsigned char *far_maketables(struct CharTableSet *pTable) {
unsigned char *yield, *p;
int i,j;

yield = (unsigned char*)(pcre_malloc)(tables_length);

if (yield == NULL) return NULL;
p = yield;

/* First comes the lower casing table */

for (i = 0; i < 256; i++) *p++ = tolower(i);

/* Next the case-flipping table */

for (i = 0; i < 256; i++) *p++ = islower(i)? toupper(i) : tolower(i);

/* Then the character class tables. Don't try to be clever and save effort
on exclusive ones - in some locales things may be different. Note that the
table for "space" includes everything "isspace" gives, including VT in the
default locale. This makes it work for the POSIX class [:space:]. */

memset(p, 0, cbit_length);
for (i = 0; i < 256; i++)
{
  j = pTable->DecodeTable[i];
  if (isdigit(j))
    {
    p[cbit_digit  + i/8] |= 1 << (i&7);
    p[cbit_word   + i/8] |= 1 << (i&7);
    }
  if (isupper(i))
    {
    p[cbit_upper  + i/8] |= 1 << (i&7);
    p[cbit_word   + i/8] |= 1 << (i&7);
    }
  if (islower(i))
    {
    p[cbit_lower  + i/8] |= 1 << (i&7);
    p[cbit_word   + i/8] |= 1 << (i&7);
    }
  if (i == '_')   p[cbit_word   + i/8] |= 1 << (i&7);
  if (isspace(j)) p[cbit_space  + i/8] |= 1 << (i&7);
  if (isxdigit(j))p[cbit_xdigit + i/8] |= 1 << (i&7);
  if (isgraph(j)) p[cbit_graph  + i/8] |= 1 << (i&7);
  if (isprint(j)) p[cbit_print  + i/8] |= 1 << (i&7);
  if (ispunct(j)) p[cbit_punct  + i/8] |= 1 << (i&7);
  if (iscntrl(j)) p[cbit_cntrl  + i/8] |= 1 << (i&7);
  }
p += cbit_length;

/* Finally, the character type table. In this, we exclude VT from the white
space chars, because Perl doesn't recognize it as such for \s and for comments
within regexes. */

for (i = 0; i < 256; i++)
{
  int x = 0;
  j = pTable->DecodeTable[i];
  if (i != 0x0b && isspace(j)) x += ctype_space;
  if (isalpha(i)) x += ctype_letter;
  if (isdigit(j)) x += ctype_digit;
  if (isxdigit(j)) x += ctype_xdigit;
  if (isalpha(i) || isdigit(j) || i == '_') x += ctype_word;

  /* Note: strchr includes the terminating zero in the characters it considers.
  In this instance, that is ok because we want binary zero to be flagged as a
  meta-character, which in this sense is any character that terminates a run
  of data characters. */

  if (strchr("*+?{^.$|()[", i) != 0) x += ctype_meta; *p++ = x; }

return yield;
}

/* End of maketables.c */
