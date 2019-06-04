/*
 * cache.cpp
 *
 * Created on: September 12, 2018
 * Author: Aniruddha Kanhere
*/

#include<stdio.h>
#include<stdlib.h>
#include "cache.h"
#include<iostream>
#include<math.h>
#include<stdint.h>
#include<iomanip>

#define DEBUG 0
#define Debug_run 0

#if Debug_run == 1
    FILE *Debug_file;
#endif

using namespace std;

typedef struct cache
{
	unsigned long int dirty;					//Allocate dirty bits
	unsigned long int valid_bits;		        //Allocate valid bits
	unsigned long int recently_used;		    //Allocate recently used table
	unsigned long int tag;			        	//Allocate address space (can call it the 'tag' for a block
}
cache;

typedef struct cache_modifer
{
	unsigned long int index_shift_value;
	unsigned long int index_mask;
	unsigned long int set;						//Number of rows
	unsigned long int byte_shift_value;
}
cache_modifer;

//================================= Function declaration ==============================
//Takes address and read/write command
void cache_l1_sim(unsigned long int, char);
void cache_l2_sim(unsigned long int, char);

//Takes addresses of evicted and required blocks
int victim_cache1(unsigned long, int, unsigned long, int *);

//========================== Global Variables for easy reference ========================
cache_params params;
cache_modifer l1_cache;
cache_modifer l2_cache;
cache_modifer VC1_cache;
cache_modifer VC2_cache;

cache **cache_mem_l1;
cache **cache_mem_l2;
cache *VC_cache;

uint32_t hit_L1, hit_L2, read_L1, read_miss_L1, write_L1, write_miss_L1, swap_request_L1, swap_success_L1;
int write_back_L1;
uint32_t read_L2, read_miss_L2, write_L2, write_miss_L2, swap_request_L2, swap_success_L2, write_back_L2;

static int VC1_init=0;
//=========================================================================================

int main (int argc, char* argv[])
{
    FILE *FP;               // File handler

    char *trace_file;       // Variable that holds trace file name;
    char str[2];			// variable to read the address descriptor
    unsigned long int addr; // Variable holds the address read from input file

    if(argc != 8)           // Checks if correct number of inputs have been given. Throw error and exit if wrong
    {
        printf("Error: Expected inputs:7 Given inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }

    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    params.block_size       = strtoul(argv[1], NULL, 10);
    params.l1_size          = strtoul(argv[2], NULL, 10);
    params.l1_assoc         = strtoul(argv[3], NULL, 10);
    params.vc_num_blocks    = strtoul(argv[4], NULL, 10);
    params.l2_size          = strtoul(argv[5], NULL, 10);
    params.l2_assoc         = strtoul(argv[6], NULL, 10);
    trace_file              = argv[7];

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
#if Debug_run == 1
    Debug_file = fopen("result.txt", "w");
#endif

    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

#if Debug_run == 1
    if(Debug_file == NULL)
    {
            // Throw error and exit if fopen() failed
            printf("Error: Unable to open file %s\n", "result.txt");
            exit(EXIT_FAILURE);
    }
#endif

    // Print params
    printf("  ===== Simulator configuration =====\n"
            "  BLOCKSIZE:                     %lu\n"
            "  L1_SIZE:                          %lu\n"
            "  L1_ASSOC:                         %lu\n"
            "  VC_NUM_BLOCKS:                    %lu\n"
            "  L2_SIZE:                          %lu\n"
            "  L2_ASSOC:                         %lu\n"
            "  trace_file:                       %s\n"
            , params.block_size, params.l1_size, params.l1_assoc, params.vc_num_blocks, params.l2_size, params.l2_assoc, trace_file);

    int i=0;
    if(params.l1_size)
    {
		while(fscanf(FP, "%s %lx", str, &addr) != EOF)
		{
			i++;

			#if DEBUG == 1

			char rw = str[0];

				if (rw == 'r')
					printf("%s %lx\n", "read", addr);           // Print and test if file is read correctly
				else if (rw == 'w')
					printf("%s %lx\n", "write", addr);          // Print and test if file is read correctly
			#endif

			cache_l1_sim(addr, str[0]);
		}
    }
    else
    	cout<<"L1 Cache must have a non zero size value!!!";

#if Debug_run == 1
    fclose(Debug_file);
#endif

    cout<<"\n=====L1 contents=====\n";
    for(unsigned long int j=0;j<params.l1_size/(params.l1_assoc * params.block_size);j++)
    {
    	cout<<"Set  "<<std::dec<<j<<": "<<"\t\t";
    	unsigned long int current_LRU=1;
    	for(int k=params.l1_assoc-1;k>=0;k--)
    	{
			for(int i=params.l1_assoc-1;i>=0;i--)
			{

				if(cache_mem_l1[j][i].recently_used==current_LRU)
				{

					current_LRU++;
					cout<<std::hex<<cache_mem_l1[j][i].tag;
					if(cache_mem_l1[j][i].dirty)
						cout<<" D\t";
					else
						cout<<"  \t";
					break;
				}
			}
    	}
    	cout<<endl;
    }

    if(params.vc_num_blocks &&  VC1_init)
    {
		cout<<endl<<endl<<endl<<"=====VC contents=====\n\n";
		cout<<"Set 0:	";
		int current_LRU=1;
		for(unsigned long int i=0;i<params.vc_num_blocks;i++)
		{
			for(int k=params.vc_num_blocks-1;k>=0;k--)
			{
				if(VC_cache[k].recently_used==current_LRU)
				{
					current_LRU++;
					cout<<std::hex<<VC_cache[k].tag;
					if(VC_cache[k].dirty)
						cout<<" D\t";
					else
						cout<<"  \t";
					break;
				}
			}
		}
    }
    else if(!VC1_init && params.vc_num_blocks)
    {
    	cout<<endl<<endl<<endl<<"=====VC contents=====\n\n";
    	cout<<"Set 0:	-EMPTY-";
    }


    if(params.l2_size)
    {
    	cout<<"\n\n=====L2 contents=====\n";
		for(unsigned long int j=0;j<params.l2_size/(params.l2_assoc * params.block_size);j++)
		{
			if(j<100)
				cout<<"Set  "<<std::dec<<j<<": "<<"\t";
			else
				cout<<"Set  "<<std::dec<<j<<": "<<"\t";
			int current_LRU=1;
			for(int k=params.l2_assoc-1;k>=0;k--)
			{
				for(int i=params.l2_assoc-1;i>=0;i--)
				{
					if(cache_mem_l2[j][i].recently_used==(unsigned long int)current_LRU)
					{
						current_LRU++;
						cout<<std::hex<<cache_mem_l2[j][i].tag;
						if(cache_mem_l2[j][i].dirty)
							cout<<" D\t";
						else
							cout<<"  \t";
						break;
					}
				}
			}
			cout<<endl;
		}
    }

    cout<<"\n\n=====Simulation results=====\n";
    cout<<  "a. Number of L1 reads:                "<<std::dec<<read_L1;
    cout<<"\nb. Number of L1 read misses:          "<<read_miss_L1;
    cout<<"\nc. Number of L1 writes:               "<<write_L1;
    cout<<"\nd. Number of L1 write misses:         "<<write_miss_L1;
    cout<<"\ne. Number of swap requests:           "<<swap_request_L1;

    if((double)read_L1 + write_L1)
    	cout<<"\nf. Swap request rate:                 "<<setprecision(4)<<fixed<<(double)swap_request_L1/((double)read_L1 + write_L1);
    else
    	cout<<"\nf. Swap request rate:                 "<<setprecision(4)<<fixed<<0;

    cout<<"\ng. Number of swaps:                   "<<swap_success_L1;

    if((double)read_L1 + write_L1)
    	cout<<"\nh. Combined L1 + VC miss rate:        "<<setprecision(4)<<fixed<<((double)read_miss_L1+(double)write_miss_L1-(double)swap_success_L1)/((double)read_L1+(double)write_L1);
    else
    	cout<<"\nh. Combined L1 + VC miss rate:        "<<setprecision(4)<<fixed<<(double)0;

    cout<<"\ni. Number writebacks from L1/VC:   "<<write_back_L1;
    cout<<"\nj. Number of L2 reads:                "<<read_L2;
    cout<<"\nk. Number of L2 read misses:          "<<read_miss_L2;
    cout<<"\nl. Number of L2 writes:               "<<write_L2;
    cout<<"\nm. Number of L2 write misses:         "<<write_miss_L2;

    if(read_L2)
    	cout<<"\nn. L2 miss rate:                      "<<setprecision(4)<<fixed<<(double)read_miss_L2/read_L2;
    else
    	cout<<"\nn. L2 miss rate:                      "<<setprecision(4)<<fixed<<(double)0;

    cout<<"\no. Number of writebacks from L2:      "<<write_back_L2;

    if(params.l2_size)
    	cout<<"\np. Total memory traffic:              "<<read_miss_L2+write_miss_L2+write_back_L2;
    else
    	cout<<"\np. Total memory traffic:              "<<read_miss_L1+write_miss_L1+write_back_L1-swap_success_L1;

    return 0;
}

void cache_l1_sim(unsigned long int address, char read_write)
{
	static int cache_initialised = 0;

	if(!cache_initialised)									//run this one time only(during first access)
	{
		//cout<<"Cache 1 Initialized!"<<endl;
		cache_initialised = 1;

		l1_cache.byte_shift_value = (unsigned long int)ceil(log2(params.block_size));
		l1_cache.set = params.l1_size/(params.l1_assoc * params.block_size);

		l1_cache.index_shift_value = (unsigned long int)ceil(log2(l1_cache.set));
		l1_cache.index_mask = ((unsigned long int)pow(2,l1_cache.index_shift_value) - 1);

		cache_mem_l1 = new cache *[l1_cache.set];
				for(unsigned long int i=0; i<l1_cache.set; i++)
					cache_mem_l1[i] = new cache[params.l1_assoc];

		//INITIALIZE THE VALID BIT & DIRTY ARRAY WITH 0s
        for(unsigned long int i=0; i < l1_cache.set; i++)
		{
			for(unsigned long int j=0; j < params.l1_assoc; j++)
			{
				cache_mem_l1[i][j].valid_bits = 0;
				cache_mem_l1[i][j].dirty = 0;
				cache_mem_l1[i][j].recently_used = j+1;
			}
		}
	}

	unsigned long int block_req = address>>l1_cache.byte_shift_value;
	unsigned long int index_req = l1_cache.index_mask & block_req;
	unsigned long int tag_req = block_req>>(l1_cache.index_shift_value);

#if Debug_run == 1
	static uint32_t counter=0;
	counter++;
	fprintf(Debug_file,"\n----------------------------\n #%d: ",counter);
#endif

	if(read_write == 'w')
	{
#if Debug_run == 1
		fprintf(Debug_file,"\nL1 write: %lx (tag %lx, index: %d)", address, tag_req,(int)index_req);
#endif
		write_L1++;
	}
	else
	{
#if Debug_run == 1
		fprintf(Debug_file,"\nL1 read: %lx (tag %lx, index: %d)", address, tag_req,(int)index_req);
#endif
		read_L1++;
	}


	int found = 0;                                            //Flag setting whether we found the block or not
	int addr1; 										  //for index values

	if(read_write == 'w' || read_write == 'r')
	{
		for(unsigned long int i=0; i < params.l1_assoc && found==0; i++)
		{
			if((cache_mem_l1[index_req][i].tag == tag_req) && (cache_mem_l1[index_req][i].valid_bits == 1))
			{
				found = 1;
				addr1=i;

				hit_L1++;
				#if DEBUG == 1
					cout<<"found "<<i;
				#endif
			}
		}
	}
	else
	{
		printf("Error: Invalid entry in the file (command should be 'r' or 'w')\n");
		exit(EXIT_FAILURE);
	}

	//If an address is found to reside in the L1 cache, we can just use it without reaching out to L2 or VC. If it is a write command,
	//we can just change its dirty bit. If it is a read command, then we don't even need to change the dirty bit.
	if(found)
	{
#if Debug_run == 1
		fprintf(Debug_file,"\nL1 Hit\nL1 update LRU");
		if(read_write == 'w')
			fprintf(Debug_file,"\nL1 set dirty");
#endif
		unsigned long int LRUi;         //variable reused in finding invalid blocks

		//cache_mem_l1[index_req][addr1].tag = tag_req;
		//cache_mem_l1[index_req][addr1].valid_bits = 1;
		unsigned long int prv_LRUval = cache_mem_l1[index_req][addr1].recently_used;
		cache_mem_l1[index_req][addr1].recently_used = 1;

		if(read_write == 'w')
			cache_mem_l1[index_req][addr1].dirty = 1;

		LRUi = addr1;

		for(unsigned long int i=0; i < params.l1_assoc; i++)
		{
			if(i!=LRUi && cache_mem_l1[index_req][i].recently_used<prv_LRUval)
			{
				cache_mem_l1[index_req][i].recently_used+=1;  //reorder other elements
			}
		}
	}
	else     //i.e. if the required block is not found to be residing in the cache
	{
#if Debug_run == 1
		fprintf(Debug_file,"\nL1 Miss");
#endif
		if(read_write == 'w')
			write_miss_L1++;
		else
			read_miss_L1++;

		//we should first check for any invalid blocks and try to write there. If there are none, we should...
		//...find the least recently used block and then replace it
		unsigned long int found_invalid=0, LRUval=0,LRUi, updated_j=0;

		for(unsigned long int i=0; i < params.l1_assoc && (found_invalid==0); i++)
		{
			if(cache_mem_l1[index_req][i].valid_bits == 0)
			{
				found_invalid = 1;
				addr1=i;
			}
		}

		//if we find the invalid blocks from the above loop then we don't need to go to the VC since we haven't discarded any block yet
		//but we still have to go the the L2 cache to fetch the given address
		if(found_invalid)
		{
			if(params.l2_size)                               //if L2 is enabled but VC is not
			{
				cache_l2_sim(address, 'r');
			}

			cache_mem_l1[index_req][addr1].tag = tag_req;							//this we get from the L2
			cache_mem_l1[index_req][addr1].valid_bits = 1;

			if(read_write=='w')
				cache_mem_l1[index_req][addr1].dirty = 1;
			else
				cache_mem_l1[index_req][addr1].dirty = 0;

			cache_mem_l1[index_req][addr1].recently_used = 1;

			updated_j = addr1;

#if Debug_run == 1
			if(read_write == 'w')
			{
				fprintf(Debug_file,"\nL1 set dirty");
			}
			fprintf(Debug_file,"\nL1 update LRU");
#endif
#if DEBUG == 1
    		cout<<"\nFound invalid at: "<<addr1;
#endif

			//reorder other blocks as per requirement
			for(unsigned long int j=0;j<params.l1_assoc;j++)
			{
				if(updated_j!=j)
				{
					cache_mem_l1[index_req][j].recently_used += 1;
				}
			}
		}
		//If we did not find any the requested block and neither did we find any invalid block, we must try and search the VC first and swap
		//the required block with the LRU block which we will evict. If found, then that's it. But if not found, we need to evict the LRU
		//block (which will be taken by the VC, and if dirty we have to write it through to L2 cache too) and then we will ask the requested
		//block from the L2 cache.
		else							//we have to find the least recently used
		{
			LRUi=0;
			for(unsigned long int i=0; i < params.l1_assoc; i++)
			{
				if(cache_mem_l1[index_req][i].recently_used > LRUval)
				{
					LRUval = cache_mem_l1[index_req][i].recently_used;
					LRUi=i;
				}
			}

			static int temp_dirty_store;

			if(params.vc_num_blocks)                             //If victim cache is enabled
			{
#if Debug_run == 1
				fprintf(Debug_file,"\nL1 victim  %lx (tag %lx, index %d, %s)", ((((cache_mem_l1[index_req][LRUi].tag)<<l1_cache.index_shift_value)|(index_req))
						<<l1_cache.byte_shift_value), cache_mem_l1[index_req][LRUi].tag, index_req,
						cache_mem_l1[index_req][LRUi].dirty?"Dirty":"Clean");
#endif
				swap_request_L1++;
				if(!victim_cache1((((cache_mem_l1[index_req][LRUi].tag)<<l1_cache.index_shift_value)|(index_req))
						<<l1_cache.byte_shift_value, (int)cache_mem_l1[index_req][LRUi].dirty,address, &temp_dirty_store))
				//if we didn't get the block from the VC then...
				{
#if Debug_run == 1
					fprintf(Debug_file,"\nVC miss");
					if(read_write == 'w')
						fprintf(Debug_file,"\nL1 set dirty");
#endif
					if(params.l2_size)                           //if L2 is enabled
					{
						cache_l2_sim(address,'r');                                              //read it from L2
					}
						cache_mem_l1[index_req][LRUi].tag = tag_req;							//this we get from the L2
						cache_mem_l1[index_req][LRUi].valid_bits = 1;

						if(read_write=='w')
							cache_mem_l1[index_req][LRUi].dirty = 1;
						else
							cache_mem_l1[index_req][LRUi].dirty = 0;

						cache_mem_l1[index_req][LRUi].recently_used = 1;
				}
				else											  //if we get it from VC
				{
#if Debug_run == 1
					fprintf(Debug_file,"\nVC hit");
					if(read_write == 'w')
						fprintf(Debug_file,"\nL1 set dirty");
#endif
					swap_success_L1++;

					cache_mem_l1[index_req][LRUi].tag = tag_req;							//this we get from the VC
					cache_mem_l1[index_req][LRUi].valid_bits = 1;
					if(read_write=='r')
					 cache_mem_l1[index_req][LRUi].dirty = (int)temp_dirty_store;
					else
					 cache_mem_l1[index_req][LRUi].dirty = 1;

					cache_mem_l1[index_req][LRUi].recently_used = 1;
				}
			}
			else if(params.l2_size)                               //if L2 is enabled but VC is not
			{
				if(cache_mem_l1[index_req][LRUi].dirty == 1)     //if the evicted block is dirty
				{
					//========== WRITE to L2 ===========
					write_back_L1++;
					cache_l2_sim((((cache_mem_l1[index_req][LRUi].tag)<<l1_cache.index_shift_value)|index_req)<<l1_cache.byte_shift_value, 'w');
				}
				cache_l2_sim(address,'r');                                               //read it from L2           CONSIDER
				cache_mem_l1[index_req][LRUi].tag = tag_req;							//this we get from the L2
				cache_mem_l1[index_req][LRUi].valid_bits = 1;
				if(read_write=='w')
					cache_mem_l1[index_req][LRUi].dirty = 1;
				else
					cache_mem_l1[index_req][LRUi].dirty = 0;
				cache_mem_l1[index_req][LRUi].recently_used = 1;
			}
			else												//if L2 and VC are disabled
			{
				if(cache_mem_l1[index_req][LRUi].dirty == 1)     //if the evicted block is dirty
				{
					//========== WRITE to MAIN MEMORY ===========
					//cout<<"\nNo VC -> Main memory accessed\n";
					write_back_L1++;
				}

				cache_mem_l1[index_req][LRUi].tag = tag_req;					       //read it from main memory
				cache_mem_l1[index_req][LRUi].valid_bits = 1;
				if(read_write=='w')
					cache_mem_l1[index_req][LRUi].dirty = 1;
				else
					cache_mem_l1[index_req][LRUi].dirty = 0;
				cache_mem_l1[index_req][LRUi].recently_used = 1;
			}

#if Debug_run == 1
			fprintf(Debug_file,"\nL1 update LRU");
#endif

			for(unsigned long int i=0; i < params.l1_assoc; i++)
			{
				if(i!=LRUi)
				{
					cache_mem_l1[index_req][i].recently_used+=1;  //reorder other elements
				}
			}
		}
	}

#if DEBUG == 1

	cout<<endl<<"################### L1 CACHE Start ############################";
	for(unsigned long int i=0; i < l1_cache.set; i++)
	{
		for(unsigned long int j=0; j < params.l1_assoc; j++)
		{
			cout<<endl<<"==========================================="<<endl;
			cout<<"Dirty bit:            "<<cache_mem_l1[i][j].dirty<<endl;
			cout<<"Valid bit:            "<<cache_mem_l1[i][j].valid_bits<<endl;
			cout<<"Recently used value:  "<<cache_mem_l1[i][j].recently_used<<endl;
			cout<<"Tag:                  "<<cache_mem_l1[i][j].tag<<endl;
			cout<<"===========================================\n";
		}
	}
	cout<<endl<<"################### L1 CACHE End ############################\n";
#endif

	return;
}


void cache_l2_sim(unsigned long int address, char read_write)
{
	static int cache_initialised = 0;

		if(!cache_initialised)									//run this one time only(during first access)
		{
			//cout<<"Cache 1 Initialized!"<<endl;
			cache_initialised = 1;

			l2_cache.byte_shift_value = (unsigned long int)ceil(log2(params.block_size));
			l2_cache.set = params.l2_size/(params.l2_assoc * params.block_size);

			l2_cache.index_shift_value = (unsigned long int)ceil(log2(l2_cache.set));
			l2_cache.index_mask = ((unsigned long int)pow(2,l2_cache.index_shift_value) - 1);

			cache_mem_l2 = new cache *[l2_cache.set];
					for(unsigned long int i=0; i<l2_cache.set; i++)
						cache_mem_l2[i] = new cache[params.l2_assoc];

			//INITIALIZE THE VALID BIT & DIRTY ARRAY WITH 0s
	        for(unsigned long int i=0; i < l2_cache.set; i++)
			{
				for(unsigned long int j=0; j < params.l2_assoc; j++)
				{
					cache_mem_l2[i][j].valid_bits = 0;
					cache_mem_l2[i][j].dirty = 0;
					cache_mem_l2[i][j].recently_used = j+1;
				}
			}
		}

		unsigned long int block_req = address>>l2_cache.byte_shift_value;
		unsigned long int index_req = l2_cache.index_mask & block_req;
		unsigned long int tag_req = block_req>>(l2_cache.index_shift_value);

		if(read_write == 'w')
			write_L2++;
		else
			read_L2++;


		int found = 0;                                            //Flag setting whether we found the block or not
		int addr1; 										  //for index values

		if(read_write == 'w' || read_write == 'r')
		{
			for(unsigned long int i=0; i < params.l2_assoc && found==0; i++)
			{
				if((cache_mem_l2[index_req][i].tag == tag_req)  && (cache_mem_l2[index_req][i].valid_bits == 1))
				{
					found = 1;
					addr1=i;

					hit_L2++;
					#if DEBUG == 1
						cout<<"found "<<i;
					#endif
				}
			}
		}
		else
		{
			printf("Error: Invalid entry in the file (command should be 'r' or 'w')\n");
			exit(EXIT_FAILURE);
		}

		//If an address is found to reside in the L2 cache, we can just use it without reaching out to L2 or VC. If it is a write command,
		//we can just change its dirty bit. If it is a read command, then we don't even need to change the dirty bit.
		if(found)
		{
			static unsigned long int LRUi, prv_LRUval;         //variable reused in finding invalid blocks

			//cache_mem_l2[index_req][addr1].tag = tag_req;
			//cache_mem_l2[index_req][addr1].valid_bits = 1;
			prv_LRUval = cache_mem_l2[index_req][addr1].recently_used;
			cache_mem_l2[index_req][addr1].recently_used = 1;

			if(read_write == 'w')
				cache_mem_l2[index_req][addr1].dirty = 1;

			LRUi = addr1;

			for(unsigned long int i=0; i < params.l2_assoc; i++)
			{
				if((cache_mem_l2[index_req][i].recently_used < prv_LRUval) && (i!=LRUi))
				{
					cache_mem_l2[index_req][i].recently_used+=1;  //reorder other elements
				}
			}
		}
		else     //i.e. if the required block is not found to be residing in the cache
		{
			if(read_write == 'w')
				write_miss_L2++;
			else
				read_miss_L2++;

			//we should first check for any invalid blocks and try to write there. If there are none, we should...
			//...find the least recently used block and then replace it
			unsigned long int found_invalid=0, LRUval=0,LRUi, updated_j=0;

			for(unsigned long int i=0; i < params.l2_assoc && (found_invalid==0); i++)
			{
				if(cache_mem_l2[index_req][i].valid_bits == 0)
				{
					found_invalid = 1;
					addr1=i;
				}
			}

			//if we find the invalid blocks from the above loop then we don't need to go to the VC since we haven't discarded any block yet
			//but we still have to go the the L2 cache to fetch the given address
			if(found_invalid)
			{
				//cout<<"\nMain memory accessed\n";
				//READ from MAIN MEMORY
				cache_mem_l2[index_req][addr1].tag = tag_req;					       //read it from main memory
				cache_mem_l2[index_req][addr1].valid_bits = 1;
				if(read_write=='w')
					cache_mem_l2[index_req][addr1].dirty = 1;
				else
					cache_mem_l2[index_req][addr1].dirty = 0;

				cache_mem_l2[index_req][addr1].recently_used = 1;

				#if DEBUG == 1
					cout<<"\nFound invalid at: "<<addr1;
				#endif

				updated_j = addr1;

				//reorder other blocks as per requirement
				for(unsigned long int j=0;j<params.l2_assoc;j++)
				{
					if(updated_j!=j)
					{
						cache_mem_l2[index_req][j].recently_used += 1;
					}
				}
			}
			//If we did not find any the requested block and neither did we find any invalid block, we must try and search the VC first and swap
			//the required block with the LRU block which we will evict. If found, then that's it. But if not found, we need to evict the LRU
			//block (which will be taken by the VC, and if dirty we have to write it through to L2 cache too) and then we will ask the requested
			//block from the L2 cache. Too much for the poor PC.. :p
			else							//we have to find the least recently used
			{
				for(unsigned long int i=0; i < params.l2_assoc; i++)
				{
					if(cache_mem_l2[index_req][i].recently_used > LRUval)
					{
						LRUval = cache_mem_l2[index_req][i].recently_used;
						LRUi=i;
					}
				}

				if(cache_mem_l2[index_req][LRUi].dirty == 1)     //if the evicted block is dirty
				{
					//========== WRITE to MAIN MEMORY ===========
					//cout<<"\nNo VC -> Main memory accessed\n";
					write_back_L2++;
				}

				cache_mem_l2[index_req][LRUi].tag = tag_req;					       //read it from main memory
				cache_mem_l2[index_req][LRUi].valid_bits = 1;
				if(read_write=='w')
					cache_mem_l2[index_req][LRUi].dirty = 1;
				else
					cache_mem_l2[index_req][LRUi].dirty = 0;
				cache_mem_l2[index_req][LRUi].recently_used = 1;

				for(unsigned long int i=0; i < params.l2_assoc; i++)
				{
					if(i!=LRUi)
					{
						cache_mem_l2[index_req][i].recently_used+=1;  //reorder other elements
					}
				}
			}
		}

	#if DEBUG == 1

		cout<<endl<<"################### L2 CACHE Start ############################";
		for(unsigned long int i=0; i < l2_cache.set; i++)
		{
			for(unsigned long int j=0; j < params.l2_assoc; j++)
			{
				cout<<endl<<"==========================================="<<endl;
				cout<<"Dirty bit:            "<<cache_mem_l2[i][j].dirty<<endl;
				cout<<"Valid bit:            "<<cache_mem_l2[i][j].valid_bits<<endl;
				cout<<"Recently used value:  "<<cache_mem_l2[i][j].recently_used<<endl;
				cout<<"Tag:                  "<<cache_mem_l2[i][j].tag<<endl;
				cout<<"===========================================\n";
			}
		}
		cout<<endl<<"################### L2 CACHE End ############################\n";
	#endif

		return;
}


int victim_cache1(unsigned long evicted, int evicted_dirty_bit, unsigned long required ,
		int *required_dirty_bit)
{
	if(!VC1_init)
	{
		VC1_init = 1;

		VC_cache = new cache [params.vc_num_blocks];

		VC1_cache.byte_shift_value = (unsigned long int)ceil(log2(params.block_size));
		VC1_cache.set = params.vc_num_blocks;

		for(unsigned long int i=0; i < params.vc_num_blocks; i++)
		{
			VC_cache[i].valid_bits = 0;
			VC_cache[i].dirty = 0;
			VC_cache[i].recently_used = i+1;
		}
	}

	unsigned long int found = 0, addr1=0;
	unsigned long int block_addr_req = required >> VC1_cache.byte_shift_value;
	unsigned long int block_addr_evict = evicted >> VC1_cache.byte_shift_value;

	for(unsigned long int i=0; i < params.vc_num_blocks && (found==0); i++)
	{
		if((VC_cache[i].valid_bits == 1) && (VC_cache[i].tag == block_addr_req))
		{
			found=1;
			addr1 = i;
		}
	}

	if(found)
	{
#if Debug_run == 1
		fprintf(Debug_file,"\nVC update LRU");
#endif
		unsigned long int LRUi, prv_LRUval = 0;

		//cout<<(uint64_t)required_dirty_bit;

		*required_dirty_bit = VC_cache[addr1].dirty;
		prv_LRUval = VC_cache[addr1].recently_used;

		VC_cache[addr1].tag = block_addr_evict;
		VC_cache[addr1].valid_bits = 1;
		VC_cache[addr1].recently_used = 1;
		VC_cache[addr1].dirty = evicted_dirty_bit;

		LRUi = addr1;

		for(unsigned long int i=0; i < params.vc_num_blocks; i++)
		{
			if((VC_cache[i].recently_used < prv_LRUval) && (i!=LRUi))
			{
				VC_cache[i].recently_used+=1;  //reorder other elements
			}
		}
		return 1;
	}
	else     //i.e. if the required block is not found to be residing in the VC
	{
		//we should first check for any invalid blocks and try to write there. If there are none, we should...
		//...find the least recently used block and then replace it

		unsigned long int found_invalid=0, LRUval=0,LRUi=0, updated_j=0;

		for(unsigned long int i=0; i < params.vc_num_blocks && (found_invalid==0); i++)
		{
			if(VC_cache[i].valid_bits == 0)
			{
				found_invalid = 1;
				addr1=i;
			}
		}

		//if we find the invalid blocks from the above loop then we write the data of block evicted from cache here
		if(found_invalid)
		{
			#if DEBUG == 1
				cout<<"\nIn VC found invalid at: "<<addr1;
			#endif

			VC_cache[addr1].tag = evicted>>VC1_cache.byte_shift_value;
			VC_cache[addr1].valid_bits = 1;
			VC_cache[addr1].recently_used = 1;
			VC_cache[addr1].dirty = evicted_dirty_bit;

			updated_j = addr1;

			//unsigned long int current_replacement = 1;

			//reorder other blocks as per requirement
			for(unsigned long int j=0;j<params.vc_num_blocks;j++)
				if((updated_j!=j))
				{
					VC_cache[j].recently_used += 1;
				}

		}
		//If we did not find the requested block and neither did we find any invalid block, we must try and search the LRU block to
		//be evicted.
		else							//we have to find the least recently used
		{
			for(unsigned long int i=0; i < params.vc_num_blocks; i++)
			{
				if(VC_cache[i].recently_used > LRUval)
				{
					LRUval = VC_cache[i].recently_used;
					LRUi=i;
				}
			}

			if(VC_cache[LRUi].dirty==1)
			{
				write_back_L1++;
				if(params.l2_size)     //if L2 exists
				{
					//=========== WRITE to L2 ===========
					cache_l2_sim(VC_cache[LRUi].tag<<VC1_cache.byte_shift_value,'w');
				}
				else
				{
					//WRITE to MAIN MEMORY
					//cout<<"Main memory written by VC";
				}
			}

#if Debug_run == 1
		fprintf(Debug_file,"\nVC victim: %lx (tag %lx, index 0, %s)", (VC_cache[LRUi].tag<<VC1_cache.byte_shift_value), VC_cache[LRUi].tag,
				VC_cache[LRUi].dirty?"Dirty":"Clean");
#endif

			VC_cache[LRUi].tag = evicted>>VC1_cache.byte_shift_value;
			VC_cache[LRUi].valid_bits = 1;
			VC_cache[LRUi].recently_used = 1;
			VC_cache[LRUi].dirty = evicted_dirty_bit;

			for(unsigned long int i=0; i < params.vc_num_blocks; i++)
			{
				if(i!=LRUi)
				{
					VC_cache[i].recently_used+=1;  //reorder other elements
				}
			}
		}

#if DEBUG == 1

	cout<<endl<<"################### VC1 CACHE Start ############################";
	for(unsigned long int j=0; j < params.vc_num_blocks; j++)
		{
			cout<<endl<<"==========================================="<<endl;
			cout<<"Dirty bit:            "<<VC_cache[j].dirty<<endl;
			cout<<"Valid bit:            "<<VC_cache[j].valid_bits<<endl;
			cout<<"Recently used value:  "<<VC_cache[j].recently_used<<endl;
			cout<<"Tag:                  "<<VC_cache[j].tag<<endl;
			cout<<"===========================================\n";
		}

	cout<<endl<<"################### VC1 CACHE End ############################\n";
#endif
		return 0;
	}
}
