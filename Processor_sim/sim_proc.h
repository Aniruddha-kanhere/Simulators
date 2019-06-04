/*
 * sim_proc.h
 *
 *  Created on: Nov 21, 2018
 *      Author: Kalindi
 */

#ifndef SIM_PROC_H_
#define SIM_PROC_H_

#define FULL 1
#define NOT_FULL 0

#define Architectural_Registers 67
#define ROB_offset 100
#define NUM 6

char DEBUG=1;

extern unsigned long int cycle_count;

typedef struct debug{
	unsigned long int *age;
	char *op, *valid;
	int *src1, *src2;
	int *dest;
	int *FE_start, *FE_end;
	int *DE_start, *DE_end;
	int *RN_start, *RN_end;
	int *RR_start, *RR_end;
	int *DI_start, *DI_end;
	int *IS_start, *IS_end;
	int *EX_start, *EX_end;
	int *WB_start, *WB_end;
	int *RT_start, *RT_end;
}debug;

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

typedef struct DE_REG{
	unsigned long int *PC;
	int*OP, *DST, *SRC1, *SRC2;
	char *valid;
	unsigned long int *age;
}DE_REG;

typedef struct FE_REG{
	unsigned long int *PC;
	int*OP, *DST, *SRC1, *SRC2;
	char *valid;
	unsigned long int *age;
}FE_REG;

typedef struct Reorder_Buffer{
	char EMPTY;
	char FILLED;
	char *ready;
	char *miss_predict;
	int head;
	int tail;
	int *dst;
	unsigned long int *PC;
	double *value;
	unsigned long int *age;
}Reorder_Buffer;

typedef struct RN_REG{
	unsigned long int *PC;
	int *OP, *DST, *SRC1, *SRC2;
	char *valid;
	unsigned long int *age;
}RN_REG;

typedef struct RR_REG{
	unsigned long int *PC;
	int *OP, *DST, *SRC1, *SRC2;
	char *valid;
	int *ROB_entry;
	unsigned long int *age;
	char *S1_ready;
	char *S2_ready;
}RR_REG;

typedef struct DI_REG{
	unsigned long int *PC;
	int *OP, *DST, *SRC1, *SRC2;
	char *valid;
	int *ROB_entry;
	unsigned long int *age;
	char *S1_ready;
	char *S2_ready;
}DI_REG;

typedef struct Rename_map_table{
	char valid[Architectural_Registers];
	int ROB_entry[Architectural_Registers];
}Rename_map_table;

typedef struct Issue_queue{
	char *valid;
	char *S1_ready;
	char *S2_ready;
	int *OP, *DST, *SRC1, *SRC2;
	unsigned long int *age;
	unsigned long int *timer;
	int *ROB_entry;
}Issue_queue;

typedef struct Execute_list{
	char *valid;
	int *OP, *DST, *SRC1, *SRC2;
	unsigned long int *age;
	unsigned long int *timer;
	int *ROB_entry;
	int index;
	char *time_to_complete;
}Execute_list;

typedef struct WB_REG{
	char *valid;
	int *OP, *DST, *SRC1, *SRC2;
	unsigned long int *age;
	unsigned long int *timer;
	int index;
	int *ROB_entry;
	char *time_to_complete;
}WB_REG;

void Retire();
void Writeback();
void Execute(void);
void Issue(void);
void Dispatch(void);
void RegRead(void);
void Rename(void);
void Decode(void);
void Fetch(void);
int Advance_Cycle(void);

void update_tail();
void Issue_queue_update(void);
void wakeup_IQ_dependent(int);
void wakeup_DI_dependent(int);
void wakeup_RR_dependent(int);

#endif /* SIM_PROC_H_ */
