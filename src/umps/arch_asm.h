/*
 * uMPS machine-specific constants, most notably bus & device memory
 * mapped register addresses.
 *
 * IMPORTANT: Keep this header assembler-safe!
 */

#ifndef UMPS_ARCH_ASM_H
#define UMPS_ARCH_ASM_H

/*
 * Generalities
 */
#define WORD_SIZE 4
#define WS        WORD_SIZE

#define DEV_BASE 0x10000000
#define RAM_BASE 0x20000000

/*
 * CPU Control registers
 */
#define CP0_Index      0
#define CP0_Random     1
#define CP0_EntryLo    2
#define CP0_BadVAddr   8
#define CP0_EntryHi   10
#define CP0_Status    12
#define CP0_Cause     13
#define CP0_EPC       14
#define CP0_PRID      15

/* Segment-related constants */
#define KSEGOS_BASE        0x00000000
#define KSEGOS_TOP         0x20000000
#define KSEGOS_BIOS_BASE   0x00000000
#define KSEGOS_BOOT_BASE   0x1FC00000

#define KUSEG2_BASE        0x80000000

/* Device register size */
#define DEV_REG_SIZE_W   4
#define DEV_REG_SIZE     (DEV_REG_SIZE_W * WS)

/*
 * Interrupt lines
 */

#define N_INTERRUPT_LINES   8

#define N_IL                N_INTERRUPT_LINES

/* Number of interrupt lines available to devices */
#define N_EXT_IL            5

/* Devices per interrupt line */
#define N_DEV_PER_IL        8

#define DEV_IL_START        (N_INTERRUPT_LINES - N_EXT_IL)

#define IL_TIMER            2
#define IL_DISK             3
#define IL_TAPE             4
#define IL_ETHERNET         5
#define IL_PRINTER          6
#define IL_TERMINAL         7

#define EXT_IL_INDEX(il)    ((il) - DEV_IL_START)

/*
 * Bus and device register definitions
 *
 * Device interrupt lines are identified by the range [3, 7],
 * i.e. their repsective physical interrupt lines. Keep this in mind
 * when using the macros below. This is slightly confusing, but so is
 * any alternative.
 */

/* Bus register space */
#define BUS_REG_RAM_BASE        0x10000000
#define BUS_REG_RAM_SIZE        0x10000004
#define BUS_REG_BIOS_BASE       0x10000008
#define BUS_REG_BIOS_SIZE       0x1000000C
#define BUS_REG_BOOT_BASE       0x10000010
#define BUS_REG_BOOT_SIZE       0x10000014
#define BUS_REG_TOD_HI          0x10000018
#define BUS_REG_TOD_LO          0x1000001C
#define BUS_REG_TIMER           0x10000020
#define BUS_REG_TIME_SCALE      0x10000024

/* Installed devices bitmap */
#define IDEV_BITMAP_BASE        0x10000028
#define IDEV_BITMAP_END         (IDEV_BITMAP_BASE + N_EXT_IL * WS)
#define IDEV_BITMAP_ADDR(line)  (IDEV_BITMAP_BASE + ((line) - DEV_IL_START) * WS)

/* Interrupting devices bitmap */
#define CDEV_BITMAP_BASE        0x1000003C
#define CDEV_BITMAP_END         (CDEV_BITMAP_BASE + N_EXT_IL * WS)
#define CDEV_BITMAP_ADDR(line)  (CDEV_BITMAP_BASE + ((line) - DEV_IL_START) * WS)

/* Device register area */
#define DEV_REG_START           0x10000050
#define DEV_REG_ADDR(line, dev) (DEV_REG_START + ((line) - DEV_IL_START) * N_DEV_PER_IL * DEV_REG_SIZE + (dev) * DEV_REG_SIZE)

/* End of memory mapped external device registers area */
#define DEV_REG_END             (DEV_REG_START + N_EXT_IL * N_DEV_PER_IL * DEV_REG_SIZE)

/*
 * Multiprocessor (MP) controller register space
 * Unfinished and/or unimplemented garbage - don't rely on this yet!
 */
#define MPC_NCPUS               0x100002D0
#define MPC_CPU_CONTROL         0x100002D4
#define MPC_CPU_BOOT_PC         0x100002D8
#define MPC_CPU_BOOT_ARG        0x100002DC

#define MPC_CPUID               0x100002E0
#define MPC_IPI_IN              0x100002E4
#define MPC_IPI_OUT             0x100002E8
#define MPC_SHADOW_0            0x100002EC
#define MPC_SHADOW_1            0x100002F0
#define MPC_BIOS_SP             0x100002F4
#define MPC_BIOS_EV             0x100002F8

#define MPC_BASE                MPC_NCPUS
#define MPC_END                 (MPC_BIOS_EV + WS)

#define DEV_END                 MPC_END

#endif /* !defined(UMPS_ARCH_ASM_H) */
