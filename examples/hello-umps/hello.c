#include "umps/libumps.h"
#include "umps/arch.h"
#include "umps/types.h"

#define ST_READY           1
#define ST_BUSY            3
#define ST_TRANSMITTED     5

#define CMD_ACK            1
#define CMD_TRANSMIT       2

#define CHAR_OFFSET        8
#define TERM_STATUS_MASK   0xFF

typedef unsigned int u32;

static void term_puts(const char *str);
static int term_putchar(char c);
static u32 tx_status(termreg_t *tp);

static termreg_t *term0_reg = (termreg_t *) DEV_REG_ADDR(IL_TERMINAL, 0);

void main(void)
{
    term_puts("hello, world\n");

    /* Go to sleep and power off the machine if anything wakes us up */
    WAIT();
    *((u32 *) MCTL_POWER) = 0x0FF;
    while (1) ;
}

static void term_puts(const char *str)
{
    while (*str)
        if (term_putchar(*str++))
            return;
}

static int term_putchar(char c)
{
    u32 stat;

    stat = tx_status(term0_reg);
    if (stat != ST_READY && stat != ST_TRANSMITTED)
        return -1;

    term0_reg->transm_command = ((c << CHAR_OFFSET) | CMD_TRANSMIT);

    while ((stat = tx_status(term0_reg)) == ST_BUSY)
        ;

    term0_reg->transm_command = CMD_ACK;

    if (stat != ST_TRANSMITTED)
        return -1;
    else
        return 0;
}

static u32 tx_status(termreg_t *tp)
{
    return ((tp->transm_status) & TERM_STATUS_MASK);
}
