#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

const int NUM_TESTS =  100000000;
					    //500000000;
/*
const char* search = "String to be searched";
const char* toSearch = "be se";
int main()
{
	void* result = 0;

	for (int i = 0; i < NUM_TESTS; ++i)
	{
		result = strstr(search, toSearch);
	}

	result = (char*)result + 1;

	return (int)result;
}   */

/*
int main()
{
	double result = 0.0;

//	printf("Starting\n");

	for (int i = 0; i < NUM_TESTS; ++i)
	{
		result += expf(100);
	}

//	printf("Finished\n");

	return (int) result;
} */

/*
int main()
{
	struct timespec time;

//	printf("Starting\n");

	for (int i = 0; i < NUM_TESTS; ++i)
	{
		clock_gettime(CLOCK_MONOTONIC, &time);
	}

//	printf("Finished\n");

	return 0;
} */

int main()
{
	int result = 0;

//	printf("Starting\n");

	for (int i = 0; i < NUM_TESTS; ++i)
	{
		result = atoi("2571825");
	}

//	printf("Finished\n");

	return result;
} 