/*
 * sim_bp.h
 *
 *  Created on: Nov 2, 2018
 *      Author: Kalindi
 */

#ifndef SIM_BP_H_
#define SIM_BP_H_

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>

typedef struct Bimodal{
	uint32_t index_mask = 0;
	char *counter;
	uint32_t total_predictions=0;
	uint32_t miss_predicts=0;
}Bimodal;

typedef struct Gshare{
	uint32_t index_mask = 0;
	char *counter;
	uint32_t global_history=0;
	uint32_t total_predictions=0;
	uint32_t miss_predicts=0;
}Gshare;

typedef struct Hybrid{
	uint32_t index_mask = 0;
	char *counter;
	uint32_t total_predictions=0;
	uint32_t miss_predicts=0;
}Hybrid;

typedef struct bp_params{
    unsigned long int K;
    unsigned long int M1;
    unsigned long int M2;
    unsigned long int N;
    char* bp_name;
    Bimodal bimodal;
    Gshare gshare;
    Hybrid hybrid;
}bp_params;

typedef class Predictor{

}Predictor;

char Bimodal_predictor(uint32_t, char, int);
char Gshare_predictor (uint32_t, char, int);
char Hybrid_predictor (uint32_t, char);

void gshare_init(void);
void bimodal_init(void);
void hybrid_init(void);
void print_output(char);

#endif /* SIM_BP_H_ */
