/*
*  main.cpp
*
*  Created on: Nov 21, 2018
*      Author: Aniruddha
*/

#include "sim_proc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <iomanip>

int DECODE_STAGE=0, current_state=12;

DE_REG DE;
FE_REG FE;
debug Debug;
RN_REG RN;
RR_REG RR;
DI_REG DI;
WB_REG WB;
Issue_queue IQ;
Reorder_Buffer ROB;
Rename_map_table RMT;
Execute_list EL;

FILE *FP;               // File handler
char *trace_file;       // Variable that holds trace file name;
proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
int op_type, dest, src1, src2;  // Variables are read from trace file
unsigned long int pc; // Variable holds the pc read from input file

unsigned long int instr_count = 0, cycle_count = 0;
int debug_array_size;
int conservative_buffer_size;

unsigned long int total_cycles=0;
using namespace std;

int main (int argc, char* argv[])
{
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }

    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
   /* printf("rob_size:%lu "
            "iq_size:%lu "
            "width:%lu "
            "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);*/
    // Open trace_file in read mode

    debug_array_size = (params.width*(NUM*2 + 4)) + params.iq_size + params.rob_size;
    conservative_buffer_size = params.width * NUM;

    Debug.op =       new char[debug_array_size];
    Debug.valid =    new char[debug_array_size];
    Debug.age =      new unsigned long int[debug_array_size];
    Debug.dest =     new int[debug_array_size];
    Debug.src1 =     new int[debug_array_size];
    Debug.src2 =     new int[debug_array_size];
    Debug.FE_start = new int[debug_array_size];
    Debug.FE_end =   new int[debug_array_size];
    Debug.DE_start = new int[debug_array_size];
    Debug.DE_end =   new int[debug_array_size];
    Debug.RN_start=  new int[debug_array_size];
    Debug.RN_end =   new int[debug_array_size];
    Debug.RR_start = new int[debug_array_size];
    Debug.RR_end =   new int[debug_array_size];
    Debug.DI_start = new int[debug_array_size];
    Debug.DI_end =   new int[debug_array_size];
    Debug.IS_start = new int[debug_array_size];
    Debug.IS_end =   new int[debug_array_size];
    Debug.EX_start = new int[debug_array_size];
    Debug.EX_end =   new int[debug_array_size];
    Debug.WB_start = new int[debug_array_size];
    Debug.WB_end =   new int[debug_array_size];
    Debug.RT_start = new int[debug_array_size];
    Debug.RT_end =   new int[debug_array_size];

    for(int i=0; i<debug_array_size; i++)
    	Debug.valid[i] = 0;

    WB.DST =       new int[conservative_buffer_size];
    WB.OP =        new int[conservative_buffer_size];
    WB.ROB_entry = new int[conservative_buffer_size];
    WB.SRC1 =      new int[conservative_buffer_size];
    WB.SRC2 =      new int[conservative_buffer_size];
    WB.timer =     new unsigned long int[conservative_buffer_size];
    WB.age =       new unsigned long int[conservative_buffer_size];
    WB.valid =            new char[conservative_buffer_size];
    WB.time_to_complete = new char[conservative_buffer_size];

    for(int i=0; i<conservative_buffer_size; i++)
    	WB.valid[i] = 0;

    if(DEBUG)
    {
    	cout<<"WB initialized \n";
    }

    EL.time_to_complete = new char[conservative_buffer_size];
    EL.valid = 			  new char[conservative_buffer_size];
    EL.DST =       new int[conservative_buffer_size];
    EL.OP =        new int [conservative_buffer_size];
    EL.ROB_entry = new int[conservative_buffer_size];
    EL.SRC1 =      new int[conservative_buffer_size];
    EL.SRC2 =      new int[conservative_buffer_size];
    EL.timer =     new unsigned long int[conservative_buffer_size];
    EL.age = 	   new unsigned long int[conservative_buffer_size];

    for(int i=0;i<conservative_buffer_size ;i++)
    	EL.valid[i] = 0;

    if(DEBUG)
    	cout<<"EL initialized \n";

    DE.OP =    new int[params.width];
    DE.DST =   new int[params.width];
    DE.SRC1 =  new int[params.width];
    DE.SRC2 =  new int[params.width];
    DE.valid = new char[params.width];
    DE.age =   new unsigned long int[params.width];
    DE.PC =    new unsigned long int[params.width];

    for(int i=0;(unsigned int)i<params.width;i++)
    	DE.valid[i] = 0;

    if(DEBUG)
    	cout<<"DE initialized \n";

    FE.OP =    new int[params.width];
	FE.DST =   new int[params.width];
	FE.SRC1 =  new int[params.width];
	FE.SRC2 =  new int[params.width];
	FE.valid = new char[params.width];
	FE.age =   new unsigned long int[params.width];
	FE.PC =    new unsigned long int[params.width];

	for(int i=0;(unsigned int)i<params.width;i++)
		FE.valid[i] = 0;

	if(DEBUG)
		cout<<"FE initialized \n";

	RR.OP =        new int[params.width];
	RR.DST = 	   new int[params.width];
	RR.SRC1 =      new int[params.width];
	RR.SRC2 =      new int[params.width];
	RR.ROB_entry = new int[params.width];
	RR.S1_ready =  new char[params.width];
	RR.S2_ready =  new char[params.width];
	RR.valid =     new char[params.width];
	RR.age = new unsigned long int[params.width];
    RR.PC =  new unsigned long int[params.width];

    for(int i=0;(unsigned int)i<params.width;i++)
    {
    	RR.valid[i] = 0;
    }

    if(DEBUG)
    	cout<<"RR initialized \n";

    DI.S1_ready =  new char[params.width];
    DI.S2_ready =  new char[params.width];
	DI.valid =     new char[params.width];
	DI.OP =        new int[params.width];
	DI.DST =       new int[params.width];
	DI.SRC1 =      new int[params.width];
	DI.SRC2 =      new int[params.width];
	DI.ROB_entry = new int[params.width];
	DI.PC = 	   new unsigned long int[params.width];
	DI.age = 	   new unsigned long int[params.width];

    for(int i=0;(unsigned int)i<params.width;i++)
    	DI.valid[i] = 0;

    if(DEBUG)
    {
    	cout<<"DI initialized \n";
    }

	RN.valid = new char[params.width];
	RN.OP =    new int[params.width];
	RN.DST =   new int[params.width];
	RN.SRC1 =  new int[params.width];
	RN.SRC2 =  new int[params.width];
	RN.PC =    new unsigned long int[params.width];
	RN.age =   new unsigned long int[params.width];

    for(int i=0;(unsigned int)i<params.width;i++)
    	RN.valid[i] = 0;

    if(DEBUG)
    	cout<<"RN initialized \n";

    ROB.EMPTY = 1;
    ROB.head = ROB.tail = -1;           //empty
    ROB.FILLED = 0;
    ROB.ready = 	   new char[params.rob_size];
    ROB.miss_predict = new char[params.rob_size];
    ROB.dst =          new int[params.rob_size];
    ROB.PC =           new unsigned long int[params.rob_size];
    ROB.value =        new double[params.rob_size];
    ROB.age =          new unsigned long int[params.rob_size];

    if(DEBUG)
    	cout<<"ROB initialized \n";

    IQ.S1_ready =  new char[params.iq_size];
	IQ.S2_ready =  new char[params.iq_size];
	IQ.valid = 	   new char[params.iq_size];
    IQ.DST = 	   new int[params.iq_size];
    IQ.OP = 	   new int[params.iq_size];
    IQ.SRC1 = 	   new int[params.iq_size];
    IQ.SRC2 =	   new int[params.iq_size];
    IQ.ROB_entry = new int[params.iq_size];
    IQ.age = 	   new unsigned long int[params.iq_size];

    IQ.timer = new unsigned long int[params.iq_size];
    for(int i=0;(unsigned int)i<params.iq_size;i++)
    	IQ.valid[i] = 0;

    if(DEBUG)
    	cout<<"IQ initialized \n";

    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    do{
    	/*if(cycle_count>3564)
    		DEBUG=1;*/

    	Retire();
    	if(DEBUG)
    		cin.get();//cin>>a;//system("PAUSE");

    	Writeback();
    	if(DEBUG)
    		cin.get();//cin>>a;//system("PAUSE");

    	Execute();
    	if(DEBUG)
    		cin.get();//cin>>a;//system("PAUSE");

        Issue();
        if(DEBUG)
        	cin.get();//cin>>a;//system("PAUSE");

    	Dispatch();
    	if(DEBUG)
    		cin.get();//cin>>a;//system("PAUSE");

    	RegRead();
    	if(DEBUG)
    		cin.get();//cin>>a;//system("PAUSE");

        Rename();
        if(DEBUG)
        	cin.get();//cin>>a;//system("PAUSE");

        Decode();
        if(DEBUG)
        	cin.get();//cin>>a;//system("PAUSE");

        Fetch();
        if(DEBUG)
        	cin.get();//cin>>a;//system("PAUSE");

    }while(Advance_Cycle());

    cout<<"# === Simulator Command =========\n";
    cout<<"# "<<argv[0]<<"  "<<argv[1]<<"  "<<argv[2]<<"  "<<argv[3]<<"  "<<argv[4]<<endl;
    cout<<"# === Processor Configuration ===\n";
    cout<<"# ROB_SIZE = "<<argv[1]<<endl;
    cout<<"# IQ_SIZE  = "<<argv[2]<<endl;
    cout<<"# WIDTH    = "<<argv[3]<<endl;
    cout<<"# === Simulation Results ========\n";
    cout<<"# Dynamic Instruction Count    = "<<instr_count<<endl;
    cout<<"# Cycles                       = "<<total_cycles<<endl;
    cout<<"# Instructions Per Cycle (IPC) = "<<setprecision(2)<<fixed<<(double)instr_count/(cycle_count-1);
    return 0;
}

void Fetch()
{
	static unsigned long int age=0;
	unsigned long int fetched = 0;
	char fetch_full=0, de_full=0;

	if(DEBUG)
		cout<<" ----- ------ FETCH ----- ----- \n";


	for(int i = 0;(unsigned long int)i<params.width;i++)
	{
		if(DE.valid[i]==1)
		{
			de_full=1;
			break;
		}
	}

	//Fetch stage for mimicking operation
	for(int i=0; (unsigned int)i<params.width; i++)
	{
		if(FE.valid[i] == 1)
		{
			fetch_full=1;
			break;
		}
	}

	if(!de_full && fetch_full)
	{
		int i = 0;
		for(; (unsigned int)i<params.width; i++)
		{
			if(FE.valid[i])
			{
				DE.PC[i]=FE.PC[i];
				DE.OP[i]=FE.OP[i];
				DE.DST[i]=FE.DST[i];
				DE.SRC1[i]=FE.SRC1[i];
				DE.SRC2[i]=FE.SRC2[i];
				DE.valid[i] = 1;
				FE.valid[i] = 0;
				fetch_full=0;
				DE.age[i] = FE.age[i];

				//find a valid location to put this instruction
				for(int dbg=0; dbg <debug_array_size; dbg++)
				{
					if(Debug.valid[dbg] && (Debug.age[dbg]==DE.age[i]))
					{
						Debug.FE_end[dbg] = cycle_count;
						Debug.DE_start[dbg] = cycle_count;
						if(DEBUG)
						{
							cout<<Debug.DE_start[dbg]<<"  "<<Debug.age[dbg]<<endl;
						}

						break;
					}
				}
			}
		}

		if((unsigned int)i<params.width)
		{
			while((unsigned int)i<params.width)
			{
				DE.valid[i]=0;
				i++;
			}
		}
	}

	if(DEBUG)
		for(int i=0;(unsigned int)i<params.width;i++)
			cout<<DE.PC[i]<<"  "<<DE.OP[i]<<"  "<<DE.DST[i]<<"  "<<DE.SRC1[i]<<"  "<<DE.SRC2[i]<<"  "<<(int)DE.valid[i]<<"  "<<DE.age[i]<<endl;


	if(!fetch_full)
	{
		while(fetched<params.width && current_state!=EOF)
		{
			if((current_state = fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2)) != EOF)
			{
				FE.PC[fetched]=pc;
				FE.OP[fetched]=op_type;
				FE.DST[fetched]=dest;
				FE.SRC1[fetched]=src1;
				FE.SRC2[fetched]=src2;
				FE.valid[fetched]=1;
				FE.age[fetched] = age;

				//find a valid location to put this instruction
				for(int dbg=0; dbg <debug_array_size; dbg++)
				{
					if(!Debug.valid[dbg])
					{
						Debug.FE_start[dbg] = cycle_count;
						Debug.age[dbg] = FE.age[fetched];
						Debug.valid[dbg] = 1;
						Debug.dest[dbg] = FE.DST[fetched];
						Debug.op[dbg] = FE.OP[fetched];
						Debug.src1[dbg] = FE.SRC1[fetched];
						Debug.src2[dbg] = FE.SRC2[fetched];
						break;
					}
				}

				fetched++;
				age++;
				instr_count++;
			}
		}
	}

	if(DEBUG)
		cout<<"Current Cycle: "<<cycle_count;

    return;
}

void Decode()
{
	int DE_valid_bundle = 0, RN_Empty=1;

	if(DEBUG)
		cout<<"----- ----- DECODE ----- ----- \n";

	for(int i=0;(unsigned int)i<params.width;i++)
	{
		if(DE.valid[i])
		{
			DE_valid_bundle=1;
			break;
		}
	}

	if(DE_valid_bundle)
	{
		for(int i=0;(unsigned int)i<params.width;i++)
		{
			if(RN.valid[i]==1)
			{
				RN_Empty=0;
				break;
			}
		}

		if(RN_Empty)
		{
			for(int i=0;(unsigned int)i<params.width;i++)
			{
				if(DE.valid[i])
				{
					//copy the bundle from DE to RN and validate the RN bundle
					RN.PC[i] = DE.PC[i];
					RN.OP[i] = DE.OP[i];
					RN.DST[i] = DE.DST[i];
					RN.SRC1[i] = DE.SRC1[i];
					RN.SRC2[i] = DE.SRC2[i];
					RN.age[i] = DE.age[i];
					RN.valid[i] = 1;

					//In validate the DE entries simultaneously
					DE.valid[i] = 0;

					for(int dbg=0; dbg <debug_array_size; dbg++)
					{
						if(Debug.valid[dbg] && Debug.age[dbg] == RN.age[i])
						{
							Debug.DE_end[dbg] = cycle_count;
							Debug.RN_start[dbg] = cycle_count;
							break;
						}
					}
				}
			}
		}
	}

	if(DEBUG)
		for(int i=0;(unsigned int)i<params.width;i++)
			cout<<RN.PC[i]<<"  "<<RN.OP[i]<<"  "<<RN.DST[i]<<"  "<<RN.SRC1[i]<<"  "<<RN.SRC2[i]<<"  "<<(int)RN.valid[i]<<"  "<<RN.age[i]<<endl;


	return;
}

void Rename()
{
	int RN_valid_bundle=0, RR_Empty=1, ROB_space=0;

	if(DEBUG)
		cout<<"----- ----- RENAME ----- ----- \n";


	for(int i=0;(unsigned int)i<params.width;i++)
	{
		if(RN.valid[i])
		{
			RN_valid_bundle=1;
			break;
		}
	}

	if(RN_valid_bundle)
	{
		for(int i=0;(unsigned int)i<params.width;i++)
		{
			if(RR.valid[i])
			{
				RR_Empty=0;
				break;
			}
		}

		//check whether ROB has enough space to hold all of the instructions in the RN bundle - mistake
		if((ROB.tail - ROB.head) >= 0)
		{
			if((unsigned int)(params.rob_size - (ROB.tail - ROB.head + 1)) >= params.width)
				ROB_space=1;
		}
		else
		{
			if((unsigned int)(ROB.head - ROB.tail - 1) >= params.width)
				ROB_space=1;
		}

		if(RR_Empty &&  ROB_space)
		{
			for(int i=0; (unsigned int)i<params.width;i++)
			{
				if(RN.valid[i])
				{
					if(ROB.head == -1)
						ROB.head = ROB.tail = 0;
					else
						update_tail();

					//create a ROB entry for this instruction
					ROB.ready[ROB.tail] = 0;
					ROB.dst[ROB.tail] = RN.DST[i];
					ROB.PC[ROB.tail] = RN.PC[i];
					ROB.miss_predict[ROB.tail] = 0;
					ROB.age[ROB.tail] = RN.age[i];

					if(RN.SRC1[i] != -1)
					{
						if(RMT.valid[RN.SRC1[i]])           //if the source1 is already renamed in the RMT
						{
							RR.SRC1[i] = RMT.ROB_entry[RN.SRC1[i]];

							if(ROB.ready[RR.SRC1[i] - ROB_offset])
								RR.S1_ready[i] = 1;
							else
								RR.S1_ready[i] = 0;
						}
						else								//else the latest value resides in the ARF
						{
							RR.SRC1[i] = RN.SRC1[i];
							RR.S1_ready[i] = 1;
						}
					}
					else
					{
						RR.SRC1[i] = RN.SRC1[i];
						RR.S1_ready[i] = 1;
					}

					if(RN.SRC2[i] != -1)
					{
						if(RMT.valid[RN.SRC2[i]])           //if the source2 is already renamed in the RMT
						{
							RR.SRC2[i] = RMT.ROB_entry[RN.SRC2[i]];
							if(ROB.ready[RR.SRC2[i] - ROB_offset])
								RR.S2_ready[i] = 1;
							else
								RR.S2_ready[i] = 0;
						}
						else								//else the latest value resides in the ARF
						{
							RR.SRC2[i] = RN.SRC2[i];
							RR.S2_ready[i] = 1;
						}
					}
					else
					{
						RR.SRC2[i] = RN.SRC2[i];
						RR.S2_ready[i] = 1;
					}

					if(RN.DST[i] != -1)
					{
						RMT.ROB_entry[RN.DST[i]] = ROB.tail + ROB_offset;
						RMT.valid[RN.DST[i]] = 1;

						RR.DST[i] = RMT.ROB_entry[RN.DST[i]];
					}
					else
						RR.DST[i] = RN.DST[i];

					RR.ROB_entry[i] = ROB.tail + ROB_offset;      //we shall carry this with us to help us 'retire' this instruction.
					RR.OP[i] = RN.OP[i];
					RR.PC[i] = RN.PC[i];
					RR.age[i] = RN.age[i];
					RR.valid[i] = 1;

					RN.valid[i] = 0;						//invalidate the RN entry

					for(int dbg=0; dbg <debug_array_size; dbg++)
					{
						if(Debug.valid[dbg] && Debug.age[dbg] == RR.age[i])
						{
							Debug.RN_end[dbg] = cycle_count;
							Debug.RR_start[dbg] = cycle_count;
							break;
						}
					}
				}

			}
		}
	}

	if(DEBUG)
		for(int i=0;(unsigned int)i<params.width;i++)
			cout<<RR.PC[i]<<"  "<<RR.OP[i]<<"  "<<RR.DST[i]<<"  "<<RR.SRC1[i]<<"  "<<RR.SRC2[i]<<"  "<<(int)RR.valid[i]<<"  "
					<<(int)RR.S1_ready[i]<<"  "<<(int)RR.S2_ready[i]<<"  "<<RR.ROB_entry[i]<<"  "<<RR.age[i]<<endl;

	return;
}

void RegRead()
{
	int RR_valid_bundle=0, DI_empty=1;

	if(DEBUG)
		cout<<"----- ----- REGREAD ----- -----\n";

	for(int i=0;(unsigned int)i<params.width;i++)
	{
		if(RR.valid[i])
		{
			RR_valid_bundle=1;
			break;
		}
	}

	if(RR_valid_bundle)
	{
		for(int i=0;(unsigned int)i<params.width;i++)
		{
			if(DI.valid[i])
			{
				DI_empty=0;
				break;
			}
		}

		if(DI_empty && RR_valid_bundle)
		{
			for(int i=0;(unsigned int)i<params.width;i++)
			{
				if(RR.valid[i])
				{
					DI.DST[i] = RR.DST[i];
					DI.OP[i] = RR.OP[i];
					DI.PC[i] = RR.PC[i];
					DI.ROB_entry[i] = RR.ROB_entry[i];
					DI.SRC1[i] = RR.SRC1[i];
					DI.SRC2[i] = RR.SRC2[i];
					DI.age[i] = RR.age[i];
					DI.valid[i] = 1;


					//Just copying this in here. -> NOPE! This doesn't work as it is supposed to! I did all this later
					if(RR.S1_ready[i])
					{
						DI.S1_ready[i] = RR.S1_ready[i];
					}
					else
					{
						if(RR.SRC1[i] >= ROB_offset)
						{
							if(ROB.ready[RR.SRC1[i] - ROB_offset])
								DI.S1_ready[i] = 1;
							else
								DI.S1_ready[i] = 0;
						}
						else
							DI.S1_ready[i] = 0;
					}

					if(RR.S2_ready[i])
					{
						DI.S2_ready[i] = RR.S2_ready[i];
					}
					else
					{
						if(RR.SRC2[i] >= ROB_offset)
						{
							if(ROB.ready[RR.SRC2[i] - ROB_offset])
								DI.S2_ready[i] = 1;
							else
								DI.S2_ready[i] = 0;
						}
						else
							DI.S2_ready[i] = 0;
					}

					RR.valid[i] = 0;            //invalidate the RR entry

					for(int dbg=0; dbg <debug_array_size; dbg++)
					{
						if(Debug.valid[dbg] && Debug.age[dbg] == DI.age[i])
						{
							Debug.RR_end[dbg] = cycle_count;
							Debug.DI_start[dbg] = cycle_count;
							break;
						}
					}
				}
			}
		}
	}

	if(DEBUG)
		for(int i=0;(unsigned int)i<params.width;i++)
			cout<<DI.PC[i]<<"  "<<DI.OP[i]<<"  "<<DI.DST[i]<<"  "<<DI.SRC1[i]<<"  "<<DI.SRC2[i]<<"  "<<(int)DI.valid[i]<<"  "
					<<(int)DI.S1_ready[i]<<"  "<<(int)DI.S2_ready[i]<<"  "<<DI.ROB_entry[i]<<"  "<<DI.age[i]<<endl;

	return;
}

void Dispatch()
{
	int DI_instructions=0, Issue_queue_space=0;

	if(DEBUG)
		cout<<"----- ----- DISPATCH----- -----\n";


	for(int i=0; (unsigned int)i<params.width; i++)
	{
		if(DI.valid[i])
			DI_instructions++;
	}

	for(int i=0; (unsigned int)i<params.iq_size; i++)
	{
		if(IQ.valid[i] == 0)
			Issue_queue_space++;
	}

	if((DI_instructions <= Issue_queue_space) && (DI_instructions>0))
	{
		int j=0;
		for(int i=0; ((unsigned int)i<params.iq_size) && ((unsigned int)j<params.width); i++)
		{
			if(IQ.valid[i] == 0 && DI.valid[j])
			{
				IQ.valid[i] = 1;
				IQ.DST[i] = DI.DST[j];
				IQ.S1_ready[i] = DI.S1_ready[j];
				IQ.S2_ready[i] = DI.S2_ready[j];
				IQ.OP[i] = DI.OP[j];
				IQ.SRC1[i] = DI.SRC1[j];
				IQ.SRC2[i] = DI.SRC2[j];
				IQ.age[i] = DI.age[j];
				IQ.ROB_entry[i] = DI.ROB_entry[j];
				IQ.timer[i] = 0;

				DI.valid[j]=0;				//invalidate the instruction in DI

				j++;						//go to next instruction of DI

				for(int dbg=0; dbg <debug_array_size; dbg++)
				{
					if(Debug.valid[dbg] && Debug.age[dbg] == IQ.age[i])
					{
						Debug.DI_end[dbg] = cycle_count;
						Debug.IS_start[dbg] = cycle_count;
						break;
					}
				}
			}
		}
	}

	if(DEBUG)
		for(int i=0;(unsigned int)i<params.iq_size;i++)
		{
			if(IQ.valid[i])
			{
				cout<<i<<"  "<<IQ.OP[i]<<"  "<<IQ.DST[i]<<"  "<<IQ.SRC1[i]<<"  "<<IQ.SRC2[i]<<"  "<<(int)IQ.valid[i]<<"  ";
				cout<<(int)IQ.S1_ready[i]<<"  "<<(int)IQ.S2_ready[i]<<"  "<<IQ.ROB_entry[i]<<"  "<<IQ.timer[i]<<"  "<<IQ.age[i]<<endl;
			}
		}

	return;
}

void Issue()
{
	if(DEBUG)
		cout<<"----- ----- ISSUE ----- -----\n";

	Issue_queue_update();

	int issued = params.width;

	for(int j=0;(unsigned int)j<params.iq_size && issued;j++)
	{
		int flag = 1;
		int oldest;
		int index;
		for(int i=0; (unsigned int)i<params.iq_size;i++)
		{
			if(IQ.S1_ready[i] && IQ.S2_ready[i] && IQ.valid[i])
			{
				if(flag)
				{
					oldest = IQ.age[i];
					index=i;
					flag=0;
				}
				else
				{
					if(IQ.age[i] < (unsigned int)oldest)
					{
						oldest = IQ.age[i];
						index=i;
					}
				}
			}
		}

		if(!flag)             //if at least one valid/ready entry is found
		{
			int i = 0;
			for(; (unsigned int)i<(params.width*NUM);i++)
			{
				if(!EL.valid[i])
					break;
			}

			if((unsigned int)i<(params.width*NUM))   //if there is at least one vacant entry in the execute list
			{
				EL.DST[i] = IQ.DST[index];
				EL.SRC1[i] = IQ.SRC1[index];
				EL.SRC2[i] = IQ.SRC2[index];
				EL.age[i] = IQ.age[index];
				EL.ROB_entry[i] = IQ.ROB_entry[index];

				switch(IQ.OP[index])
				{
				case 0: //1 cycle latency
					EL.time_to_complete[i] = 1;
					break;
				case 1:	//2 cycle latency
					EL.time_to_complete[i] = 2;
					break;
				case 2:	//5 cycle latency
					EL.time_to_complete[i] = 5;
					break;
				}

				EL.timer[i] = 0;
				EL.valid[i]=1;

				for(int dbg=0; dbg <debug_array_size; dbg++)
				{
					if(Debug.valid[dbg] && Debug.age[dbg] == IQ.age[index])
					{
						Debug.IS_end[dbg] = cycle_count;
						Debug.EX_start[dbg] = cycle_count;
						break;
					}
				}

				IQ.valid[index] = 0;                //invalidate the instruction in IQ
				issued--;
			}
		}
	}

	if(DEBUG)
		for(int i=0;(unsigned int)i<params.width*NUM;i++)
		{
			if(EL.valid[i])
			{
				cout<<i<<"  "<<EL.DST[i]<<"  "<<EL.SRC1[i]<<"  "<<EL.SRC2[i]<<"  "<<(int)EL.valid[i]<<"  ";
				cout<<EL.ROB_entry[i]<<"  "<<EL.timer[i]<<"  "<<(int)EL.time_to_complete[i]<<"  "<<EL.age[i]<<endl;
			}
		}

	return;
}


void Execute()
{
	if(DEBUG)
		cout<<"----- ----- EXECUTE ----- -----\n";

	for(int i=0;(unsigned int)i<(params.width*NUM);i++)
	{
		if((EL.time_to_complete[i] == 1) && (EL.valid[i]==1))
		{
			//find any blank entry in WB register. Ideally this should always accommodate all the instructions exiting in one cycle
			int j=0;
			for(;(unsigned int)j<params.width*NUM;j++)
			{
				if(!WB.valid[j])
					break;
			}

			if((unsigned int)j<(params.width*NUM))
			{
				WB.DST[j] = EL.DST[i];
				WB.SRC1[j] = EL.SRC1[i];
				WB.SRC2[j] = EL.SRC2[i];
				WB.ROB_entry[j] = EL.ROB_entry[i];
				WB.age[j] = EL.age[i];
				WB.valid[j] = 1;

				//wake up all dependent instructions
				if(EL.DST[i] != -1)
				{
					wakeup_IQ_dependent(EL.DST[i]);
					wakeup_DI_dependent(EL.DST[i]);
					wakeup_RR_dependent(EL.DST[i]);
				}

				for(int dbg=0; dbg <debug_array_size; dbg++)
				{
					if(Debug.valid[dbg] && Debug.age[dbg] == WB.age[j])
					{
						Debug.EX_end[dbg] = cycle_count;
						Debug.WB_start[dbg] = cycle_count;
						break;
					}
				}

				EL.valid[i] = 0;
			}
		}
	}

	//update the time to complete value
	for(int i=0;(unsigned int)i<(params.width*NUM);i++)
	{
		if(EL.time_to_complete[i]>0)
			EL.time_to_complete[i]--;
	}

	if(DEBUG)
		for(int i=0;(unsigned int)i<(params.width*NUM);i++)
		{

			if(WB.valid[i])
			{
				cout<<i<<"  "<<WB.DST[i]<<"  "<<WB.SRC1[i]<<"  "<<WB.SRC2[i]<<"  "<<(int)WB.valid[i]<<"  ";
				cout<<WB.ROB_entry[i]<<"  "<<WB.age[i]<<endl;
			}
		}

	return;
}

void Writeback()
{
	if(DEBUG)
		cout<<"----- ----- WRITEBACK ----- -----\n";


	for(int i =0; (unsigned int)i<params.width*NUM; i++)
	{
		if(WB.valid[i])
		{
			ROB.ready[WB.ROB_entry[i] - ROB_offset] = 1;
			WB.valid[i] = 0;

			for(int dbg=0; dbg <debug_array_size; dbg++)
			{
				if(Debug.valid[dbg] && Debug.age[dbg] == WB.age[i])
				{
					Debug.WB_end[dbg] = cycle_count;
					Debug.RT_start[dbg] = cycle_count;
					break;
				}
			}
		}
	}

	if(DEBUG)
	{
		int my_pointer = ROB.head;

		if(my_pointer != -1)
		do
		{
			cout<<my_pointer<<"  "<<ROB.PC[my_pointer]<<"  "<<ROB.dst[my_pointer]<<"  "<<(int)ROB.ready[my_pointer]<<"  "<<ROB.age[my_pointer]<<endl;

			if(my_pointer == ROB.tail)
				break;
			if((unsigned int)my_pointer == (params.rob_size-1))
			{
				my_pointer=0;
			}
			else
			{
				my_pointer++;
			}
		}
		while(1);//(my_pointer!=ROB.tail);
	}


	return;
}

void Retire()
{
	int instructions_to_retire = params.width;

	ROB.EMPTY = 0;

	if(ROB.head != -1)
	while(ROB.ready[ROB.head]==1 && instructions_to_retire)
	{
		//If the entry matches that of the RMT then we need to update the RMT to indicate that the latest entry exists in ARF
		//for(int i=0;i<Architectural_Registers;i++)
		//{
		if(ROB.dst[ROB.head]!=-1)
			if((RMT.ROB_entry[ROB.dst[ROB.head]] == (ROB.head + ROB_offset)) && RMT.valid[ROB.dst[ROB.head]])
			{
				RMT.valid[ROB.dst[ROB.head]] = 0;
			}
		//}

		for(int dbg=0; dbg <debug_array_size; dbg++)
		{
			if(Debug.valid[dbg] && (Debug.age[dbg] == ROB.age[ROB.head]))
			{
				Debug.valid[dbg] = 0;
				Debug.RT_end[dbg] = cycle_count;

				cout<<Debug.age[dbg]<<" fu{"<<(int)Debug.op[dbg]<<"} src{"<<Debug.src1[dbg]<<","<<Debug.src2[dbg]<<"} dst{"<<Debug.dest[dbg]<<"} FE{"
						<<Debug.DE_start[dbg]-1<<","<<1/*Debug.FE_end[dbg]-Debug.FE_start[dbg]*/<<"} DE{"<<Debug.DE_start[dbg]<<","<<Debug.DE_end[dbg]-Debug.DE_start[dbg]<<"} RN{"
						<<Debug.RN_start[dbg]<<","<<Debug.RN_end[dbg]-Debug.RN_start[dbg]<<"} RR{"<<Debug.RR_start[dbg]<<","<<Debug.RR_end[dbg]-Debug.RR_start[dbg]<<"} DI{"
						<<Debug.DI_start[dbg]<<","<<Debug.DI_end[dbg]-Debug.DI_start[dbg]<<"} IS{"<<Debug.IS_start[dbg]<<","<<Debug.IS_end[dbg]-Debug.IS_start[dbg]<<"} EX{"
						<<Debug.EX_start[dbg]<<","<<Debug.EX_end[dbg]-Debug.EX_start[dbg]<<"} WB{"<<Debug.WB_start[dbg]<<","<<Debug.WB_end[dbg]-Debug.WB_start[dbg]<<"} RT{"
						<<Debug.RT_start[dbg]<<","<<Debug.RT_end[dbg]-Debug.RT_start[dbg]<<"}\n";

				//cin.get();

				total_cycles = Debug.RT_end[dbg];

				break;
			}
		}

		instructions_to_retire--;

		if(ROB.head==ROB.tail)
			ROB.head = ROB.tail = -1;
		else
		{
			if((unsigned long int)ROB.head<(params.rob_size-1))
				ROB.head++;
			else
				ROB.head = 0;
		}
	}
	return;
}

int Advance_Cycle()
{
	cycle_count++;
	if(current_state==-1 && ROB.head==-1 && !DE.valid[0] && !RN.valid[0] && !RR.valid[0])
		return 0;
	else
		return 1;
}

void update_tail()
{
	if((unsigned int)ROB.tail == (params.rob_size-1))
	{
		if(ROB.head != 0)
		{
		  ROB.tail=0;
		  ROB.FILLED = 0;
		}
		else
		{
			ROB.FILLED = 1;
			cout<<"Head and tail clashed! Checking error";
		}
	}
	else
	{
		if((ROB.tail+1) != ROB.head)
		{
			ROB.tail++;
			ROB.FILLED = 0;
		}
		else
		{
			ROB.FILLED = 1;
			cout<<"Head and tail clashed! Checking error";
		}
	}
	return;
}

void Issue_queue_update()
{
	for(int i=0; (unsigned int)i<params.iq_size;i++)
		IQ.timer[i]++;

	return;
}

void wakeup_IQ_dependent(int source)
{
	for(int i=0; (unsigned int)i<params.iq_size; i++)
	{
		if((source == IQ.SRC1[i]) && (IQ.valid[i]) && !IQ.S1_ready[i])
		{
			IQ.S1_ready[i] = 1;
		}

		if((source == IQ.SRC2[i]) && (IQ.valid[i]) && !IQ.S2_ready[i])
		{
			IQ.S2_ready[i] = 1;
		}
	}
	return;
}

void wakeup_DI_dependent(int source)
{
	for(int i=0; (unsigned int)i<params.width; i++)
	{
		if((source == DI.SRC1[i]) && (DI.valid[i]) && !DI.S1_ready[i])
		{
			DI.S1_ready[i] = 1;
		}

		if((source == DI.SRC2[i]) && (DI.valid[i]) && !DI.S2_ready[i])
		{
			DI.S2_ready[i] = 1;
		}
	}
	return;
}

void wakeup_RR_dependent(int source)
{
	for(int i=0; (unsigned int)i<params.width; i++)
	{
		if((source == RR.SRC1[i]) && (RR.valid[i]) && !RR.S1_ready[i])
		{
			RR.S1_ready[i] = 1;
		}

		if((source == RR.SRC2[i]) && (RR.valid[i]) && !RR.S2_ready[i])
		{
			RR.S2_ready[i] = 1;
		}
	}
	return;
}
