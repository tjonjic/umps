/*
 * uMPS BIOS related constants
 */

#ifndef BIOS_H
#define BIOS_H

/* FIXME: Are all of these safe to use (abuse?) as break codes? */
#define BIOS_SRV_FORK        0
#define BIOS_SRV_LDST        1
#define BIOS_SRV_PANIC       2
#define BIOS_SRV_HALT        3
#define BIOS_SRV_START_CPU   4

#endif /* !defined(BIOS_H) */
