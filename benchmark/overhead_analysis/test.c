#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <emmintrin.h>

void externalEmpty();


void* findPLTStub(void* callerFn, size_t callSiteIndex)
{
	unsigned char* op = (unsigned char*) callerFn;

	size_t index = 0;

	while (*op != 0xC3)
	{
		if (*op == 0xE8 || *op == 0xE9)
		{			
			if (index == callSiteIndex)
			{
				op++;

				int32_t offset = *((int32_t*)op);
				op += 4;

				intptr_t address = (intptr_t)op + offset;

				return (void*)address;
			}

			index++;
		}

		op++;
	}

	assert(0);

	return NULL;
}

void* findGOTEntry(void* pltStub)
{
	unsigned char* op = (unsigned char*) pltStub;

	op += 2; // Skip opcode and register byte.

	int32_t offset = *((int32_t*)op);
	op += 4;

	intptr_t address = (intptr_t)op + offset;

	return (void*)address;
}



void __attribute__ ((naked)) __attribute__ ((noinline)) empty()
{
	__asm__ ("retq");
} 

struct timespec diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec - start.tv_nsec) < 0) {
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return temp;
}

size_t getNanoseconds(struct timespec duration)
{
	return 1000000000ul * duration.tv_sec + duration.tv_nsec;
}

const size_t NUM_RUNS = 10;

const size_t NUM_TESTS = 500000000;
const size_t WARMUP    = 100000000;

#define TRASH_DATA_CACHE
#define TRASH_INSTR_CACHE

int main()
{
	externalEmpty(); // Let the dynamic linker look the function up.

	void* gotEntry = findGOTEntry(findPLTStub(main,0));

	struct timespec time1, time2;

	double avg = 0.0;

	const size_t arraySize = 10;

	double* array = malloc(sizeof(double)*arraySize);

	for (size_t i = 0; i < arraySize; ++i)
	{
		array[i] = i * 1.02;
	}

	for (size_t r = 0; r < NUM_RUNS; ++r)
	{ 		
		for (size_t i = 0; i < WARMUP; ++i)
		{
			__asm__("callq empty");
		}

		clock_gettime(CLOCK_MONOTONIC, &time1);
		for (size_t i = 0; i < NUM_TESTS; ++i)
		{
#ifdef TRASH_DATA_CACHE
			_mm_clflush(gotEntry);
#endif
#ifdef TRASH_INSTR_CACHE
			_mm_clflush(externalEmpty);
#endif	
			__asm__("callq empty");
		}
		clock_gettime(CLOCK_MONOTONIC, &time2);


		size_t nsDirect = getNanoseconds(diff(time1, time2));

		for (size_t i = 0; i < WARMUP; ++i)
		{
			externalEmpty();
		}

		clock_gettime(CLOCK_MONOTONIC, &time1);
		for (size_t i = 0; i < NUM_TESTS; ++i)
		{	
#ifdef TRASH_DATA_CACHE
			_mm_clflush(gotEntry);
#endif
#ifdef TRASH_INSTR_CACHE
			_mm_clflush(externalEmpty);
#endif	
			externalEmpty();
		}
		clock_gettime(CLOCK_MONOTONIC, &time2);

		size_t nsExternal = getNanoseconds(diff(time1, time2));
		
		double ratio = nsExternal / (double)nsDirect;

		printf("Ratio: %f\tDiff: %f\n", ratio, 1.0 - ratio);
		avg += ratio;
	}

	free(array);

	avg /= NUM_RUNS;
	
	printf("Ratio (external/direct): %f\n", avg );

	return 0;
}