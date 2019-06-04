/*
 * cache.h
 *
 *  Created on: Sep 12, 2018
 *      Author: Kalindi
 */

#ifndef CACHE_H_
#define CACHE_H_

typedef struct cache_params{
    unsigned long int block_size;
    unsigned long int l1_size;
    unsigned long int l1_assoc;
    unsigned long int vc_num_blocks;
    unsigned long int l2_size;
    unsigned long int l2_assoc;
}cache_params;

#endif /* CACHE_H_ */
