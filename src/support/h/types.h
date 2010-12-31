/**************************************************************************** 
 *
 * This header file contains utility types definitions.
 * 
 ****************************************************************************/

typedef struct
{
	unsigned int status;
	unsigned int command;
	unsigned int data0;
	unsigned int data1;
} device_t;


typedef struct
{
	unsigned int recv_status;
	unsigned int recv_command;
	unsigned int transm_status;
	unsigned int transm_command;
} terminal_t;


#define INTNUM	5
#define DEVNUM	8

typedef struct 
{
	unsigned int rambase;
	unsigned int ramsize;
	unsigned int execbase;
	unsigned int execsize;
	unsigned int bootbase;
	unsigned int bootsize;
	unsigned int todhi;
	unsigned int todlo;
	unsigned int intervaltimer;
	unsigned int timescale;
	unsigned int inst_dev[INTNUM];
	unsigned int interrupt_dev[INTNUM];
	device_t	devreg[INTNUM][DEVNUM];
} devregarea_t;

	
#define STATEREGNUM	31

typedef struct
{
	unsigned int entryhi;
	unsigned int cause;
	unsigned int status;
	unsigned int pc;
	int s_reg[STATEREGNUM];
} state_t;
	
#define	at	s_reg[0]
#define	v0	s_reg[1]
#define v1	s_reg[2]
#define a0	s_reg[3]
#define a1	s_reg[4]
#define a2	s_reg[5]
#define a3	s_reg[6]
#define t0	s_reg[7]
#define t1	s_reg[8]
#define t2	s_reg[9]
#define t3	s_reg[10]
#define t4	s_reg[11]
#define t5	s_reg[12]
#define t6	s_reg[13]
#define t7	s_reg[14]
#define s0	s_reg[15]
#define s1	s_reg[16]
#define s2	s_reg[17]
#define s3	s_reg[18]
#define s4	s_reg[19]
#define s5	s_reg[20]
#define s6	s_reg[21]
#define s7	s_reg[22]
#define t8	s_reg[23]
#define t9	s_reg[24]
#define gp	s_reg[25]
#define sp	s_reg[26]
#define fp	s_reg[27]
#define ra	s_reg[28]
#define HI	s_reg[29]
#define LO	s_reg[30]


#define ETH_ALEN 6
#define ETH_PAYLOAD 1500
                                                                                
typedef struct 
{
	unsigned char dest[ETH_ALEN];
	unsigned char src[ETH_ALEN];
	unsigned char proto[2];
	unsigned char data[ETH_PAYLOAD];
  	unsigned char dummy[2];
} packet_t;

