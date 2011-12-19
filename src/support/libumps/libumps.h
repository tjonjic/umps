/*
 * External declarations for uMPS library module.
 */

#ifndef UMPS_LIBUMPS_H
#define UMPS_LIBUMPS_H

/*
 * "Forward declaration" hack!!
 * Many functions in this module accept a pointer to a cpu state
 * (STATE_PTR) structure. We cannot just forward declare that because
 * state_t was commonly defined by clients as an anonymous struct
 * typedef.
 */
#define STATE_PTR void*

/* Functions valid in user mode
 */
 

/* This function cause a system call trap
 */

extern unsigned int SYSCALL(unsigned int number, unsigned int arg1, unsigned int arg2, unsigned int arg3);


/* All these functions access CP0 registers.
 * Access to CP0 registers is always possible in kernel mode, or in user
 * mode with CPU 0 bit _set_ in STATUS register 
 */

extern unsigned int getINDEX(void);

extern unsigned int getRANDOM(void);

extern unsigned int getENTRYLO(void);

extern unsigned int getBADVADDR(void);

extern unsigned int getENTRYHI(void);

extern unsigned int getSTATUS(void);

extern unsigned int getCAUSE(void);

extern unsigned int getEPC(void);

extern unsigned int getPRID(void);

extern unsigned int getTIMER(void);


/* Only some of CP0 register are R/W: handling requires care.
 * All functions return the value in register after write
 */

extern unsigned int setINDEX(unsigned int index);

extern unsigned int setENTRYLO(unsigned int entry);

extern unsigned int setENTRYHI(unsigned int entry);

extern unsigned int setSTATUS(unsigned int entry);

extern unsigned int setCAUSE(unsigned int cause);

extern unsigned int setTIMER(unsigned int timer);


/* these functions produce a program trap if executed in user mode 
 * without CPU0 bit _set_
 */

extern void TLBWR(void);

extern void TLBWI(void);

extern void TLBP(void);

extern void TLBR(void);

extern void TLBCLR(void);

extern void WAIT(void);


/* This function requires BIOS intervention, and is valid _only_ in kernel
 * mode: otherwise it causes a program trap. It may be used to start
 * a new process
 */

/* This function load a processor state from memory: there is no valid
 * return value.  New process may use status, EntryHI, pc and CAUSE as actual
 * arguments if call is carefully built, since $4, $5, and $6 (a0, a1, a2)
 * registers are not loaded from memory, but are passed as they are to 
 * new process, while $7 (a3) is loaded with CAUSE value from processor
 * state in memory. 
 * Keep in mind that $2 (v0) register is used by routine itself and it is
 * not loaded from memory image nor have a meaningful starting value for the
 * new process this routine starts.
 *
 * This is NOT an atomic operation: the processor state is loaded register
 * by register from memory, and at the end a BIOS routine will be called to
 * load the critical CAUSE, STATUS, EntryHI and PC registers in one atomic
 * operation: so, this call is interruptible (in a clean way) or cause a
 * trap (for example, a memory access error if pointer is not correctly
 * set).  
 * If called from user state, it will trap ONLY at BIOS call, loading the
 * general registers with new/random values (if no other errors intervene);
 * this will corrupt the calling process state, but it does not harm system 
 * security a bit (I thought I said you to use it only in kernel mode...)
 */
 
extern unsigned int FORK(unsigned int entryhi, unsigned int status, unsigned int pc, STATE_PTR statep);
 
 
/* This function may be called from kernel or from user mode with CPU 0
 * STATUS bit _on_: otherwise, it will cause a trap
 */
 
/* This function stores processor state to memory. It intentionally leaves 
 * the PC field set to 0; putting a meaningful value there is programmer's 
 * task. 
 * Return value is PC value for the instruction immediately following
 * the call.
 * This too is NOT an atomic operation: the processor state is saved
 * register by register to memory. So, this call is interruptible (in a
 * clean way) or cause a trap (for example, an memory access error if
 * pointer is not correctly set).  
 * If called from user state, it will trap ONLY if CPU 0 bit of STATUS CP0
 * register is NOT set, and only when access to CP0 register (STATUS, ENTRYHI,
 * CAUSE) is requested (if no other errors intervene).
 * However, trying it does not harm system security a bit.
 */
 
extern unsigned int STST(STATE_PTR statep);


/* This function may be used to restart an interrupted/blocked process,
 * reloading it from a physical address passed as argument.  
 * It is available only in kernel mode, thru a BIOS routine 
 * (otherwise it causes a trap).
 * It updates processor status _completely_, in one atomic operation.
 * It has no meaningful return value: $2 (v0) register is used for
 * BIOS call, but it is reloaded too.
 * Remember that it is programmer's task to increment OLD area PC where 
 * needed  (e.g. syscall handling) 
 */
 
extern unsigned int LDST(STATE_PTR statep);
 

/* This function stops the system printing a warning message on terminal 0
 */

extern void PANIC(void);


/* This function halts the system printing a regular shutdown message on
 * terminal 0
 */

extern void HALT(void);

extern void INITCPU(unsigned int cpuid, STATE_PTR start_state, STATE_PTR state_areas);

extern int CAS(volatile unsigned int *atomic, unsigned int oldval, unsigned int newval);

#endif /* !defined(UMPS_LIBUMPS_H) */
