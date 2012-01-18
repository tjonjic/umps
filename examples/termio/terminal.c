#include "umps/arch.h"
#include "umps/types.h"

#define ST_READY           1
#define ST_BUSY            3
#define ST_TRANSMITTED     5
#define ST_RECEIVED        5

#define CMD_ACK            1
#define CMD_TRANSMIT       2
#define CMD_RECV           2

#define CHAR_OFFSET        8
#define TERM_STATUS_MASK   0xFF

volatile termreg_t *terminal = (termreg_t*) DEV_REG_ADDR(IL_TERMINAL, 0);

static unsigned int tx_status(volatile termreg_t *tp);
static unsigned int rx_status(volatile termreg_t *tp);

int term_putchar(char c)
{
    unsigned int stat;

    stat = tx_status(terminal);
    if (stat != ST_READY && stat != ST_TRANSMITTED)
        return -1;

    terminal->transm_command = ((c << CHAR_OFFSET) | CMD_TRANSMIT);

    while ((stat = tx_status(terminal)) == ST_BUSY)
        ;

    if (stat != ST_TRANSMITTED)
        return -1;

    terminal->transm_command = CMD_ACK;

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
    if (stat != ST_READY && stat != ST_RECEIVED)
        return -1;

    terminal->recv_command = CMD_RECV;

    while ((stat = rx_status(terminal)) == ST_BUSY)
        ;

    if (stat != ST_RECEIVED)
        return -1;

    stat = terminal->recv_status;

    terminal->recv_command = CMD_ACK;

    return stat >> CHAR_OFFSET;
}

static unsigned int tx_status(volatile termreg_t *tp)
{
    return ((tp->transm_status) & TERM_STATUS_MASK);
}

static unsigned int rx_status(volatile termreg_t *tp)
{
    return ((tp->recv_status) & TERM_STATUS_MASK);
}
