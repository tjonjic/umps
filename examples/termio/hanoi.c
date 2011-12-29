#include "umps/libumps.h"
#include "terminal.h"

#define LINE_BUF_SIZE 64

static char buf[LINE_BUF_SIZE];

void hanoi(unsigned int n, char src, char dst, char aux)
{
    if (n > 0) {
        hanoi(n - 1, src, aux, dst);
        term_puts("Move disk from peg ");
        term_putchar(src);
        term_puts(" to peg ");
        term_putchar(dst);
        term_puts(";\n");
        hanoi(n - 1, aux, dst, src);
    }
}

static void readline(char *buf, unsigned int count)
{
    int c;

    while (--count && (c = term_getchar()) != '\n')
        *buf++ = c;

    *buf = '\0';
}

#define IS_DIGIT(x) ('0' <= x && x <= '9')

static unsigned int broken_strtou(const char *str)
{
    unsigned int retval = 0;
    const char *p = str;

    for (p = str; *p && IS_DIGIT(*p); p++)
        retval = retval * 10 + *p - '0';

    return retval;
}

int main(int argc, char *argv[])
{
    unsigned int n;

    term_puts("***************************\n");
    term_puts("      Towers of Hanoi      \n");
    term_puts("***************************\n");

    term_puts("\nNumber of disks: ");
    readline(buf, LINE_BUF_SIZE);
    n = broken_strtou(buf);
    hanoi(n, 'L', 'R', 'M');

    HALT();
    return 0;
}
