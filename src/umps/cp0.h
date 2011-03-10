#ifndef UMPS_CP0_H
#define UMPS_CP0_H

/*
 * CP0 registers
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
#define CP0_Timer     16
#define CP0_LLAddr    17

/*
 * CP0 Status fields
 */
#define STATUS_IEc       0x00000001
#define STATUS_IEc_BIT   0
#define STATUS_KUc       0x00000002
#define STATUS_KUc_BIT   1

#define STATUS_IEp       0x00000004
#define STATUS_IEp_BIT   2
#define STATUS_KUp       0x00000008
#define STATUS_KUp_BIT   3

#define STATUS_IEo       0x00000010
#define STATUS_IEo_BIT   4
#define STATUS_KUo       0x00000020
#define STATUS_KUo_BIT   5

#define STATUS_IM_MASK       0x0000ff00
#define STATUS_IM(line)      (1U << (8 + (line)))
#define STATUS_IM_BIT(line)  (8 + (line))

#define STATUS_BEV       0x00400000
#define STATUS_BEV_BIT   22

#define STATUS_VMc       0x01000000
#define STATUS_VMc_BIT   24
#define STATUS_VMp       0x02000000
#define STATUS_VMp_BIT   25
#define STATUS_VMo       0x04000000
#define STATUS_VMo_BIT   26

#define STATUS_CU0       0x10000000
#define STATUS_CU0_BIT   28
#define STATUS_CU1       0x20000000
#define STATUS_CU1_BIT   29
#define STATUS_CU2       0x40000000
#define STATUS_CU2_BIT   30
#define STATUS_CU3       0x80000000
#define STATUS_CU3_BIT   31

/*
 * CP0 Cause fields
 */
#define CAUSE_EXCCODE_MASK     0x0000007c
#define CAUSE_EXCCODE_BIT      2
#define CAUSE_GET_EXCCODE(x)   (((x) & CAUSE_EXCCODE_MASK) >> CAUSE_EXCCODE_BIT)

#define CAUSE_IP_MASK          0x0000ff00
#define CAUSE_IP(line)         (1U << (8 + (line)))
#define CAUSE_IP_BIT(line)     (8 + (line))

#define CAUSE_CE_MASK          0x30000000
#define CAUSE_CE_BIT           28
#define CAUSE_GET_CE(x)        (((x) & CAUSE_CE_MASK) >> CAUSE_CE_BIT)

#define CAUSE_BD               0x80000000
#define CAUSE_BD_BIT           31

#endif /* !defined(UMPS_CP0_H) */
