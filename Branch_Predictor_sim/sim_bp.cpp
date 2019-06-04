/*
 * sim_bp.cpp
 *
 *  Created on: Nov 2, 2018
 *      Author: Aniruddha
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include "sim_bp.h"
#include <iomanip>

#define BIMODAL 1
#define GSHARE  2
#define HYBRID  3

using namespace std;

bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params

int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file
    char mode;              // Mode of operation

    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }

    params.bp_name  = argv[1];

    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if(strcmp(params.bp_name, "bimodal") == 0)              // Bimodal
    {
        if(argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }

        mode = BIMODAL;

        params.M2       = strtoul(argv[2], NULL, 10);
        trace_file      = argv[3];
        params.N = 0;

        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);

        bimodal_init();         //initialize bimodal variables

    }
    else if(strcmp(params.bp_name, "gshare") == 0)          // Gshare
    {
        if(argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M1       = strtoul(argv[2], NULL, 10);
        params.N        = strtoul(argv[3], NULL, 10);
        trace_file      = argv[4];

        if(params.M1 < params.N)
        {
        	printf("Error: the value of N should be less than or equal to M");
        	exit(EXIT_FAILURE);
        }

        mode = GSHARE;

        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);

        gshare_init();		//initialize gshare variables

    }
    else if(strcmp(params.bp_name, "hybrid") == 0)          // Hybrid
    {
        if(argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.K        = strtoul(argv[2], NULL, 10);
        params.M1       = strtoul(argv[3], NULL, 10);
        params.N        = strtoul(argv[4], NULL, 10);
        params.M2       = strtoul(argv[5], NULL, 10);
        trace_file      = argv[6];

        mode = HYBRID;

        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);

        hybrid_init();

    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    char str[2];

    while(fscanf(FP, "%lx %s", &addr, str) != EOF)
    {

        outcome = str[0];
        /*if (outcome == 't')
            printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
        else if (outcome == 'n')
            printf("%lx %s\n", addr, "n");          // Print and test if file is read correctly*/

        if(mode == BIMODAL)
        {
        	Bimodal_predictor(addr, outcome, 1);    //always update the counters
        }
        else if(mode == GSHARE)
        {
        	Gshare_predictor (addr, outcome, 1);    //always update the counters
        }
        else
        {
        	Hybrid_predictor (addr, outcome);
        }
    }

    print_output(mode);

    return 0;
}


char Hybrid_predictor(uint32_t PC, char outcome)
{
	char Bimodal_prediction, Gshare_prediction;
	char prediction;

	uint32_t index = (PC & params.hybrid.index_mask)>>2;

	//cout<<index<<endl;

	//Bimodal_prediction = Bimodal_predictor(PC, outcome, 0);
	//Gshare_prediction  = Gshare_predictor (PC, outcome, 0);

	if(params.hybrid.counter[index] < 2)             //1 or 0 - update bimodal but not gshare
	{
		Bimodal_prediction = Bimodal_predictor(PC, outcome, 1);
		Gshare_prediction  = Gshare_predictor (PC, outcome, 0);
		prediction = Bimodal_prediction;
	}
	else										 	 //2 or 3 - update gshare but not bimodal
	{
		Bimodal_prediction = Bimodal_predictor(PC, outcome, 0);
		Gshare_prediction  = Gshare_predictor (PC, outcome, 1);
		prediction = Gshare_prediction;
	}

	if((Gshare_prediction == outcome) && (Bimodal_prediction != outcome))
	{
		if(params.hybrid.counter[index] < 3)
			params.hybrid.counter[index]++;
	}
	else if((Gshare_prediction != outcome) && (Bimodal_prediction == outcome))
	{
		if(params.hybrid.counter[index] > 0)
					params.hybrid.counter[index]--;
	}
	else
	{
		//do nothing;
	}

	//Update the total number of predictions
	params.hybrid.total_predictions++;

	//Update miss - predicts
	if(prediction != outcome)
		params.hybrid.miss_predicts++;

	return prediction;
}

char Gshare_predictor(uint32_t PC, char outcome, int update)
{
	//This is the index-masked value of PC
	uint32_t PC_masked = (PC & params.gshare.index_mask)>>2;

	//This is the n-bit global branch history
	uint32_t history = params.gshare.global_history & (uint32_t)(pow(2,params.N) - 1);

	//XOR first n bits of masked PC with the n-bit branch history
	uint32_t index = ((PC_masked>>(params.M1 - params.N)) ^ (history));

	//Concatenate the n-bit XORed value with the last m-n bits of masked PC
	index = (index<<(params.M1 - params.N)) | (PC_masked & (uint32_t)(pow(2,(params.M1 - params.N)) - 1));

	char prediction;
	//Predict based on the counter value
	if(params.gshare.counter[index] > 1)
		prediction = 't';
	else
		prediction = 'n';

	if(update)
	{
		//update the counter based on the outcome
		if(outcome == 't' && params.gshare.counter[index] < 3)
			params.gshare.counter[index]++;
		else if(outcome == 'n' && params.gshare.counter[index]>0)
			params.gshare.counter[index]--;


		//Update the total number of predictions
		params.gshare.total_predictions++;

		//Update miss - predicts
		if(prediction != outcome)
			params.gshare.miss_predicts++;
	}

	//update the global history regardless of bimodal execution
	if(outcome == 't')
		params.gshare.global_history = (params.gshare.global_history>>1)|(0x01<<(params.N - 1));
	else
		params.gshare.global_history = (params.gshare.global_history>>1);

	return prediction;
}

char Bimodal_predictor(uint32_t PC, char outcome, int update)
{
	uint32_t index = (PC & params.bimodal.index_mask)>>2;

	char prediction;
	//Predict based on the counter value
	if(params.bimodal.counter[index]>1)
		prediction = 't';
	else
		prediction = 'n';

	if(update)
	{
		//update the counter based on the outcome
		if(outcome == 't' && params.bimodal.counter[index] < 3)
			params.bimodal.counter[index]++;
		else if(outcome == 'n' && params.bimodal.counter[index] > 0)
			params.bimodal.counter[index]--;

		//Update the total number of predictions
		params.bimodal.total_predictions++;

		//Update miss - predicts
		if(prediction != outcome)
			params.bimodal.miss_predicts++;
	}

	return prediction;
}

void print_output(char mode)
{
        cout<<"OUTPUT"<<endl;

	if(mode == BIMODAL)
	{
		cout<<"Number of Predictions: "<<params.bimodal.total_predictions<<endl;
		cout<<"Number of Mispredictions: "<<params.bimodal.miss_predicts<<endl;
		cout<<"Misprediction Rate: "<<setprecision(2)<<fixed<<((double)params.bimodal.miss_predicts * 100)/((double)params.bimodal.total_predictions)<<"%"<<endl;
		cout<<"FINAL BIMODAL CONTENTS"<<endl;
		for(uint32_t i=0; i<=(params.bimodal.index_mask>>2); i++)
		{
			//cout<<i<<"\t"<<(int)params.bimodal.counter[i]<<endl;
		}
	}
	else if(mode == GSHARE)
	{
		cout<<"Number of predictions: "<<params.gshare.total_predictions<<endl;
		cout<<"Number of mispredictions: "<<params.gshare.miss_predicts<<endl;
		cout<<"Misprediction rate: "<<setprecision(2)<<fixed<<((double)params.gshare.miss_predicts * 100)/((double)params.gshare.total_predictions)<<"%";
		cout<<endl<<"FINAL GSHARE CONTENTS"<<endl;
		for(uint32_t i=0; i <= (params.gshare.index_mask>>2); i++)
		{
			cout<<i<<"  "<<(int)params.gshare.counter[i]<<endl;
		}
	}
	else
	{
	    cout<<"Number of predictions: "<<params.hybrid.total_predictions<<endl;
	    cout<<"Number of mispredictions: "<<params.hybrid.miss_predicts<<endl;
	    cout<<"Misprediction rate: "<<setprecision(2)<<fixed<<((double)params.hybrid.miss_predicts * 100)/((double)params.hybrid.total_predictions)<<"%"<<endl;
	    cout<<"FINAL CHOOSER CONTENTS"<<endl;
	    for(uint32_t i=0; i <= (params.hybrid.index_mask>>2); i++)
	    {
	    	cout<<i<<"  "<<(int)params.hybrid.counter[i]<<endl;
	    }
	    cout<<endl<<"FINAL GSHARE CONTENTS"<<endl;
	    for(uint32_t i=0; i <= (params.gshare.index_mask>>2); i++)
		{
			cout<<i<<"  "<<(int)params.gshare.counter[i]<<endl;
		}
	    cout<<endl<<"FINAL BIMODAL CONTENTS"<<endl;
	    for(uint32_t i=0; i <= (params.bimodal.index_mask>>2); i++)
		{
			cout<<i<<"  "<<(int)params.bimodal.counter[i]<<endl;
		}
	}
    return;
}

void bimodal_init(void)
{
	//Declare the counter array for the bimodal predictor
	params.bimodal.counter = new char[(int)pow(2,params.M2)];

	//Initialize it to 2 for every counter => weakly taken
	for(int i =0; i < (int)pow(2,params.M2);i++)
		params.bimodal.counter[i] = 2;

	//Make the index mask for getting index from the PC
	params.bimodal.index_mask = ((int)pow(2,params.M2)-1)<<2;

	return;
}

void gshare_init(void)
{
	//Declare the counter array
	params.gshare.counter = new char[(int)pow(2,params.M1)];

	//Initialize it to 2 for every counter => weakly taken
	for(int i =0; i < (int)pow(2,params.M1);i++)
		params.gshare.counter[i] = 2;

	//Make the index mask for getting index from the PC
	params.gshare.index_mask = ((int)pow(2,params.M1)-1)<<2;

	return;
}

void hybrid_init(void)
{
	gshare_init();
	bimodal_init();

	params.hybrid.counter = new char[(int)pow(2,params.K)];
	for(int i=0; i< pow(2,params.K); i++)
	{
		params.hybrid.counter[i] = 1;
	}

	params.hybrid.index_mask = ((int)pow(2,params.K)-1)<<2;

	return;
}
