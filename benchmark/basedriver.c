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
*  Author: T. Kissinger <thomas.kissinger@tu-dresden.de>
*/

#include "basedriver.h"

int threadcount = -1;
ThreadInfo* threadinfos;


inline void HRTimerStart(HRTimer* hrt)
{
	gettimeofday(&hrt->start, 0);
}

inline double HRTimerStopp(HRTimer* hrt)
{
	gettimeofday(&hrt->stopp, 0);
	return (hrt->stopp.tv_sec - hrt->start.tv_sec) * 1000.0 + (hrt->stopp.tv_usec - hrt->start.tv_usec) / 1000.0;
}

inline int64_t getRandomValue(RngInfo* ri, u_int64_t a, u_int64_t b)
{
	if (ri->type == Uniform)
	{
		double anorm = ((double) a) /  UINT64_MAX;
		if (anorm > 1)
		{
			printf("rand error\n");
			exit(-1);
		}
		return (anorm * (ri->b - ri->a) + ri->a);
	}
	else if (ri->type == Normal)
	{
		double pi,r1,r2;
		pi =  4*atan(1);
		r1 = -log(1-(((double)  a) / UINT64_MAX ));
		r2 =  2*pi*(((double) b) / UINT64_MAX );
		r1 =  sqrt(2*r1);
		return ri->b * r1*cos(r2) + ri->a;
	}
	else
	{
		printf("RNG not implemented\n");
		exit(-1);
	}

	return 0;
}

inline void KeystoreInsert(Key* key, ThreadInfo* ti)
{
	int i;
	for (i = 0; i < BDR_DIMENSIONS[ti->indexid]; i++)
	{
		ti->keystore[ti->keypos * BDR_DIMENSIONS[ti->indexid] +i] = key->value[i]->int_value;
	}
	ti->keypos = (ti->keypos + 1) % BDR_KEY_PROVISIONING;
}

void* BenchmarkTask(void* params)
{
	ThreadInfo* ti = (ThreadInfo*) params;
	u_int64_t i, j;
	BDR_RNG_VARS
	RngInfo opselect = { Uniform, 0, 100 };
	Record record;

	void* payload = malloc(BDR_PAYLOADS[ti->indexid]);
	Attribute attrs[BDR_DIMENSIONS[ti->indexid]];
	Attribute* ar[BDR_DIMENSIONS[ti->indexid]];
	record.payload.data = payload;
	record.payload.size = BDR_PAYLOADS[ti->indexid];
	record.key.attribute_count = BDR_DIMENSIONS[ti->indexid];
	record.key.value = ar;

	Key keymax;
	keymax.attribute_count = BDR_DIMENSIONS[ti->indexid];
	Attribute attrsmax[BDR_DIMENSIONS[ti->indexid]];
	Attribute* armax[BDR_DIMENSIONS[ti->indexid]];
	keymax.attribute_count = BDR_DIMENSIONS[ti->indexid];
	keymax.value = armax;

	Index* idx;
	OpenIndex(ti->indexname, &idx);
	for (i = 0; i < BDR_DIMENSIONS[ti->indexid]; i++)
	{
		record.key.value[i] = &attrs[i];
		attrs[i].type = kInt;
		keymax.value[i] = &attrsmax[i];
		attrsmax[i].type = kInt;
		attrsmax[i].int_value = INT64_MAX;
	}
	while(! ti->halt)
	{
		BDR_RNG_NEXTPAIR
		int64_t op = getRandomValue(&opselect, rngz, rngz2);
		if (op <= BDR_RANGE_PROB)
		{
			Transaction* tx;
			BeginTransaction(&tx);
			for (j = 0; j < BDR_DIMENSIONS[ti->indexid]; j++)
			{
				record.key.value[j] = &attrs[j];
				BDR_RNG_NEXTPAIR;
				attrs[j].int_value = getRandomValue(&BDR_RNGS[ti->indexid][j], rngz, rngz2);
			}

			Iterator *it;
			Record* itrecord;
			GetRecords(tx, idx, record.key, keymax, &it);
			for (j = 0; j < 200; j++)
			{
				if (GetNext(it, &itrecord) == kOk)
				{
					ti->ops += ti->incv;
#ifdef BDR_STATS
					ti->stat_range_nexts++;
#endif
				}
				else
				{
					break;
				}
			}
			CloseIterator(&it);
			CommitTransaction(&tx);
#ifdef BDR_STATS
			ti->stat_range++;
#endif
		}
		else if (op <= BDR_RANGE_PROB + BDR_POINT_PROB)
		{
			Transaction* tx;
			Record* itrecord;
			BeginTransaction(&tx);
			for (i = 0; i < 20; i++)
			{
				RngInfo colselectri = {Uniform, 0, BDR_DIMENSIONS[ti->indexid]};
				BDR_RNG_NEXTPAIR
				int colselect = getRandomValue(&colselectri, rngz, rngz2);
				for (j = 0; j < BDR_DIMENSIONS[ti->indexid]; j++)
				{
					if (j == colselect)
					{
						record.key.value[j] = &attrs[j];
						BDR_RNG_NEXTPAIR;
						attrs[j].int_value = getRandomValue(&BDR_RNGS[ti->indexid][j], rngz, rngz2);
					}
					else
					{
						record.key.value[j] = 0;
					}
				}

				Iterator* it;
				if (GetRecords(tx, idx, record.key, record.key, &it) != kOk)
				{
					printf("GetRecords failed\n");
					exit(-1);
				}
				if (GetNext(it, &itrecord) == kOk)
				{
#ifdef BDR_STATS
					ti->stat_point_success++;
#endif
				}
				CloseIterator(&it);
				ti->ops += ti->incv;
#ifdef BDR_STATS
				ti->stat_point++;
#endif
			}
			CommitTransaction(&tx);
		}
		else if (op <= BDR_RANGE_PROB + BDR_POINT_PROB + BDR_UPDATE_PROB)
		{
			Transaction* tx;
			BeginTransaction(&tx);
			for (i = 0; i < 5; i++)
			{
				RngInfo ksselectri = {Uniform, 0, BDR_KEY_PROVISIONING};
				BDR_RNG_NEXTPAIR;
				u_int32_t ksselect = getRandomValue(&ksselectri, rngz, rngz2);
				BDR_RNG_NEXT
				memset(record.payload.data, rngz, BDR_PAYLOADS[ti->indexid]);
				for (j = 0; j < BDR_DIMENSIONS[ti->indexid]; j++)
				{
					record.key.value[j] = &attrs[j];
					attrs[j].int_value = ti->keystore[ksselect * BDR_DIMENSIONS[ti->indexid] + j];
				}
				if (UpdateRecord(tx, idx, &record, &record.payload, kIgnorePayload) == kOk)
				{
					ti->ops += ti->incv;
#ifdef BDR_STATS
					ti->stat_update++;
#endif
				}
			}
			CommitTransaction(&tx);
		}
		else if (op <= BDR_RANGE_PROB + BDR_POINT_PROB + BDR_UPDATE_PROB + BDR_INSERT_PROB)
		{
			Transaction* tx;
			BeginTransaction(&tx);
			for (i = 0; i < 5; i++)
			{
				for (j = 0; j < BDR_DIMENSIONS[ti->indexid]; j++)
				{
					record.key.value[j] = &attrs[j];
					BDR_RNG_NEXTPAIR;
					attrs[j].int_value = getRandomValue(&BDR_RNGS[ti->indexid][j], rngz, rngz2);
				}
				if (InsertRecord(tx, idx, &record) == kOk)
				{
					ti->ops += ti->incv;
#ifdef BDR_STATS
					ti->stat_insert++;
#endif
					KeystoreInsert(&record.key, ti);
				}
			}
			CommitTransaction(&tx);
		}
		else
		{
			Transaction* tx;
			BeginTransaction(&tx);
			for (i = 0; i < 5; i++)
			{
				RngInfo ksselectri = {Uniform, 0, BDR_KEY_PROVISIONING};
				BDR_RNG_NEXTPAIR;
				u_int32_t ksselect = getRandomValue(&ksselectri, rngz, rngz2);
				for (j = 0; j < BDR_DIMENSIONS[ti->indexid]; j++)
				{
					record.key.value[j] = &attrs[j];
					attrs[j].int_value = ti->keystore[ksselect * BDR_DIMENSIONS[ti->indexid] + j];
				}
				if (DeleteRecord(tx, idx, &record, kIgnorePayload) == kOk)
				{
					ti->ops += ti->incv;
#ifdef BDR_STATS
					ti->stat_delete++;
#endif
				}
			}
			CommitTransaction(&tx);
		}
	}
	free(payload);

	return 0;
}

void* PopulateIndexTask(void* params)
{
	ThreadInfo* ti = (ThreadInfo*) params;
	u_int64_t i, j;
	BDR_RNG_VARS

	ti->keystore = malloc(sizeof(int64_t) * BDR_DIMENSIONS[ti->id] * BDR_KEY_PROVISIONING);
	ti->keypos = 0;
	// Setup Index
	AttributeType atypes[BDR_DIMENSIONS[ti->id]];
	for (i = 0; i < BDR_DIMENSIONS[ti->id]; i++)
	{
		atypes[i] = kInt;
	}
	if (CreateIndex(ti->indexname, BDR_DIMENSIONS[ti->id], atypes ) != kOk)
	{
		printf("CreateIndex failed\n");
		exit(-1);
	}
	Index* idx;
	if (OpenIndex(ti->indexname, &idx) != kOk)
	{
		printf("OpenIndex failed\n");
		exit(-1);
	}

	// Populate
	Record record;
	void* payload = malloc(BDR_PAYLOADS[ti->id]);
	Attribute attrs[BDR_DIMENSIONS[ti->id]];
	Attribute* ar[BDR_DIMENSIONS[ti->id]];
	record.payload.data = payload;
	record.payload.size = BDR_PAYLOADS[ti->id];
	record.key.attribute_count = BDR_DIMENSIONS[ti->id];
	record.key.value = ar;
	for (i = 0; i < BDR_DIMENSIONS[ti->id]; i++)
	{
		record.key.value[i] = &attrs[i];
		attrs[i].type = kInt;
	}
	for (i = 0; i < BDR_DATASIZES[ti->id] / (8 * BDR_DIMENSIONS[ti->id] + BDR_PAYLOADS[ti->id]) ; i++)
	{
		for (j = 0; j < BDR_DIMENSIONS[ti->id]; j++)
		{
			BDR_RNG_NEXTPAIR;
			attrs[j].int_value = getRandomValue(&BDR_RNGS[ti->id][j], rngz, rngz2);
		}

		if (InsertRecord(0, idx, &record) != kOk)
		{
			printf("InsertRecord failed\n");
			exit(-1);

		}
		KeystoreInsert(&record.key, ti);
	}
	free(payload);
	printf("!%u... ", ti->id +1);
	fflush(stdout);

	return 0;
}

int main(int argc, char* argv[])
{
	printf("SIGMOD Programming Contest 2012 - BaseDriver\n=====================================================\n\n");
	if (argc > 1)
	{
		threadcount = atoi(argv[1]);
	}
	if (threadcount < 0)
	{
		printf("Thread count not set. Using %ld threads on this platform.\n", BDR_CORES);
		threadcount = BDR_CORES;
	}

	printf("Number of Indexes: %d\n", BDR_INDEXCOUNT);
	printf("\n");

	/*
	 * Populate Indexes
	 */
	printf("Populating indexes... ");
	HRTimer hrtimer;
	HRTimerStart(&hrtimer);
	threadinfos = malloc(sizeof(ThreadInfo) * BDR_INDEXCOUNT);
	int i;
	for (i = 0; i < BDR_INDEXCOUNT; i++)
	{
		threadinfos[i].halt = 0;
		threadinfos[i].id = i;
		threadinfos[i].indexid = i;
		snprintf((char*)&threadinfos[i].indexname, 16, "%d", i);
		threadinfos[i].ops = 0;
		if (pthread_create(&threadinfos[i].pid, 0, PopulateIndexTask, (void*) &threadinfos[i]) != 0)
		{
			printf("pthread_create failed\n");
			exit(-1);
		}
		printf("#%d... ", i+1);
		fflush(stdout);
	}
    fflush(stdout);

	for (i = 0; i < BDR_INDEXCOUNT; i++)
	{
		if (pthread_join(threadinfos[i].pid, 0) != 0)
		{
			printf("pthread_join failed\n");
			exit(-1);
		}
	}
	printf("%f ms\n", HRTimerStopp(&hrtimer) );

	printf("Creating Threads... ");
	ThreadInfo* inittis = threadinfos;
	threadinfos = malloc(sizeof(ThreadInfo) * threadcount);
	for (i = 0; i < threadcount; i++)
	{
		threadinfos[i].indexid = i % BDR_INDEXCOUNT;
		threadinfos[i].keystore = malloc(sizeof(int64_t) * BDR_DIMENSIONS[threadinfos[i].indexid] * BDR_KEY_PROVISIONING);
		memcpy(threadinfos[i].keystore, inittis[threadinfos[i].indexid].keystore, sizeof(int64_t) * BDR_DIMENSIONS[threadinfos[i].indexid] * BDR_KEY_PROVISIONING);
		threadinfos[i].keypos = 0;
	}
	for (i = 0; i < BDR_INDEXCOUNT; i++)
	{
		free(inittis[i].keystore);
	}
	free(inittis);
	for (i = 0; i < threadcount; i++)
	{
#ifdef BDR_STATS
		threadinfos[i].stat_delete = 0;
		threadinfos[i].stat_insert = 0;
		threadinfos[i].stat_point = 0;
		threadinfos[i].stat_point_success = 0;
		threadinfos[i].stat_range = 0;
		threadinfos[i].stat_range_nexts = 0;
		threadinfos[i].stat_update = 0;
#endif
		threadinfos[i].halt = 0;
		threadinfos[i].id = i;
		snprintf((char*)&threadinfos[i].indexname, 16, "%d", threadinfos[i].indexid);
		threadinfos[i].incv = 0;
		threadinfos[i].ops = 0;
		if (pthread_create(&threadinfos[i].pid, 0, BenchmarkTask, (void*) &threadinfos[i]) != 0)
		{
			printf("pthread_create failed\n");
			exit(-1);
		}
		printf("#%d... ", i+1);
		fflush(stdout);
	}
	printf("done\n");
	printf("Warming up for %d s... ", BDR_WARMUP);
	fflush(stdout);
	sleep(BDR_WARMUP);
	for (i = 0; i < threadcount; i++)
	{
		threadinfos[i].incv = 1;
	}
	printf("done\n");
	printf("Running benchmark for %d s... ", BDR_TESTRUN);
	fflush(stdout);
	sleep(BDR_TESTRUN);
	for (i = 0; i < threadcount; i++)
	{
		threadinfos[i].halt = 1;
	}
	for (i = 0; i < threadcount; i++)
	{
		if (pthread_join(threadinfos[i].pid, 0) != 0)
		{
			printf("pthread_join failed\n");
			exit(-1);
		}
	}

	u_int64_t sum = 0;
	for (i = 0; i < threadcount; i++)
	{
		sum += threadinfos[i].ops;
	}

	printf("done\n");
	printf("Score: %f operations/s\n\n", ((double) sum) / BDR_TESTRUN);
	fflush(stdout);

#ifdef BDR_STATS
	for (i = 0; i < threadcount; i++)
	{
		printf("Thread %d (index: %u)\n--------------------------------------\n", i +1, threadinfos[i].indexid);
		printf("Range Queries: %lu (iterations: %f)\n", threadinfos[i].stat_range, (double) (threadinfos[i].stat_range_nexts) / threadinfos[i].stat_range);
		printf("Point Queries: %lu (successfull: %lu)\n", threadinfos[i].stat_point, threadinfos[i].stat_point_success);
		printf("Updates      : %lu\n", threadinfos[i].stat_update);
		printf("Inserts      : %lu\n", threadinfos[i].stat_insert);
		printf("Deletes      : %lu\n", threadinfos[i].stat_delete);
		printf("\n");
	}
	fflush(stdout);
#endif

	return 0;
}















































