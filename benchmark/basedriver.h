/** Copyright (c) 2011 TU Dresden - Database Technology Group
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files
* (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Author: T. Kissinger <thomas.kissinger@tu-dresden.de>
*/

#ifndef _BASEDRIVER_H

#include <contest_interface.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#define BDR_SIZE_GB ((size_t) 1 << 30)
#define BDR_SIZE_MB ((size_t) 1 << 20)
#define BDR_CORES (sysconf(_SC_NPROCESSORS_ONLN))

typedef enum  RngType
{
	Uniform, Normal, Zipf
} RngType;

typedef struct RngInfo
{
	RngType type;
	int a;
	int b;
} RngInfo;

#define BDR_INDEXCOUNT 4
size_t BDR_DATASIZES[] = {BDR_SIZE_MB * 32, BDR_SIZE_MB * 16, BDR_SIZE_MB * 32, BDR_SIZE_MB * 64};
u_int32_t BDR_DIMENSIONS[] = { 3, 1, 4, 8};
size_t BDR_PAYLOADS[] = { 8, 8, 64, 64};

RngInfo BDR_IDX_0[] = {{Uniform, 1, 1000}, {Uniform, 2001, 3000}, {Uniform, 3001, 4000}};
RngInfo BDR_IDX_1[] = {{Normal, 20000, 5000}};
RngInfo BDR_IDX_2[] = {{Uniform, 1, 1000}, {Normal, 2000, 100}, {Normal, 1000, 500}, {Uniform, 1, 100000}};
RngInfo BDR_IDX_3[] = {{Uniform, 1, 1000}, {Normal, 2000, 100}, {Normal, 1000, 500}, {Uniform, 1, 100000}, {Uniform, 1, 1000}, {Normal, 2000, 100}, {Normal, 1000, 500}, {Uniform, 1, 100000}};
RngInfo* BDR_RNGS[] = {BDR_IDX_0, BDR_IDX_1, BDR_IDX_2, BDR_IDX_3};

#define BDR_KEY_PROVISIONING (1024 * 1024 * 2)

#define BDR_RANGE_PROB 10
#define BDR_POINT_PROB 40
#define BDR_UPDATE_PROB 20
#define BDR_INSERT_PROB 15
#define BDR_DELETE_PROB 15

#define BDR_WARMUP 10
#define BDR_TESTRUN 30
#define BDR_STATS

#define BDR_RNG_VARS  \
		unsigned long rngx=rand(), rngy=362436069, rngz=521288629; \
		unsigned long rngt, rngz2;

//rngz contains the new value
#define BDR_RNG_NEXT \
		rngx ^= rngx << 16; \
		rngx ^= rngx >> 5; \
		rngx ^= rngx << 1; \
		rngt = rngx; \
		rngx = rngy; \
		rngy = rngz; \
		rngz = rngt ^ rngx ^ rngy;

//rngz, rngz2 contains the new values
#define BDR_RNG_NEXTPAIR \
		rngx ^= rngx << 16; \
		rngx ^= rngx >> 5; \
		rngx ^= rngx << 1; \
		rngt = rngx; \
		rngx = rngy; \
		rngy = rngz; \
		rngz = rngt ^ rngx ^ rngy; \
		rngz2 = rngz; \
		rngx ^= rngx << 16; \
		rngx ^= rngx >> 5; \
		rngx ^= rngx << 1; \
		rngt = rngx; \
		rngx = rngy; \
		rngy = rngz; \
		rngz = rngt ^ rngx ^ rngy;


typedef struct ThreadInfo
{
	u_int32_t id;
	char indexname[32];
	int indexid;
	volatile u_int8_t halt;
	u_int64_t ops;
	volatile u_int8_t incv;
	pthread_t pid;
	int64_t* keystore;
	u_int32_t keypos;
#ifdef BDR_STATS
	u_int64_t stat_range;
	u_int64_t stat_range_nexts;
	u_int64_t stat_point;
	u_int64_t stat_point_success;
	u_int64_t stat_update;
	u_int64_t stat_insert;
	u_int64_t stat_delete;
#endif
} ThreadInfo  __attribute__ ((aligned (128)));

typedef struct HRTimer
{
	struct timeval start;
	struct timeval stopp;
} HRTimer;

#endif
