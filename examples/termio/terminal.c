#include "umps/arch.h"
#include "umps/types.h"

#define ST_READY           1
#define ST_BUSY            3
#define ST_TRANSMITTED     5
#define ST_RECEIVED        5

#define CMD_TRANSMIT       2
#define CMD_RECV           2

#define CHAROFFSET      8
#define STATUSMASK      0xFF

terminal_t *terminal = (terminal_t*) DEV_REG_ADDR(IL_TERMINAL, 0);

static unsigned int tx_status(terminal_t *tp);
static unsigned int rx_status(terminal_t *tp);

int term_putchar(char c)
{
    unsigned int stat;

    stat = tx_status(terminal);
    if (stat != ST_READY && stat != ST_TRANSMITTED)
        return -1;

    terminal->transm_command = ((c << CHAROFFSET) | CMD_TRANSMIT);
    stat = tx_status(terminal);
    while (stat == ST_BUSY)
        stat = tx_status(terminal);
    if (stat != ST_TRANSMITTED)
        return -1;

    return 0;
}

int term_puts(char *str)
{
    for (; *str; ++str)
        if (term_putchar(*str))
            return -1;
    return 0;
}

int term_getchar(void)
{
    unsigned int stat;

    stat = rx_status(terminal);
    if (stat != ST_READY && stat != ST_TRANSMITTED)
        return -1;

    terminal->recv_command = CMD_RECV;
    stat = rx_status(terminal);
    while (stat == ST_BUSY)
        stat = rx_status(terminal);
    if (stat != ST_RECEIVED)
        return -1;

    return terminal->recv_status >> CHAROFFSET;
}

static unsigned int tx_status(terminal_t *tp)
{
    return ((tp->transm_status) & STATUSMASK);
}

static unsigned int rx_status(terminal_t *tp)
{
    return ((tp->recv_status) & STATUSMASK);
}

