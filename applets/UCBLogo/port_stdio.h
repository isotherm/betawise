#include <stdio.h>
char *neo_fgets(char *, int, FILE *);
int neo_fputc(int, FILE *);
int neo_fputs(const char *, FILE *);
int neo_putc(int, FILE *);

/* The OS3K profile uses the ASCII classifications needed by the parser. */
#include <ctype.h>
#undef isdigit
#undef isupper
#undef islower
#define isdigit(ch) ((unsigned)((ch) - '0') <= 9)
#define isupper(ch) ((unsigned)((ch) - 'A') <= 25)
#define islower(ch) ((unsigned)((ch) - 'a') <= 25)
#undef tolower
static inline int neo_ascii_tolower(int ch)
{
    return isupper(ch) ? ch + ('a' - 'A') : ch;
}
#define tolower(ch) neo_ascii_tolower(ch)
