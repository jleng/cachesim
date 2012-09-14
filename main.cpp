#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include "Cache.h"
#include "misc.h"

using namespace std;


int main() {

	/*
	Cache(int core_id, int num_banks, int cache_size, int block_size, int assoc, int hit_latency, int miss_latency, string name): m_tag_array(core_id, (cache_size/block_size)), m_num_banks(num_banks), m_size(cache_size),m_block_size(block_size), m_associativity(assoc), m_hit_latency(hit_latency), m_miss_latency(miss_latency), m_name(name)
	*/
	// Instantiate "L1D" for core-0
	Cache *L1	= new Cache(0, 1, 1024, 16, 4, 7, 26, "Try this");

	#ifdef _DEBUG_
	L1->Print();	

	// The following 'four' accesses will fill in '4-entries' in a set (this example is 4-way associativity cache, so a single set will be full after four insertions)
	enum cache_access_status result	= L1->access(74744, 1);
	if(result==HIT)	printf("HIT_1\n");
	else		printf("MISS_1\n");

	result	=	L1->access(74488, 2);
	if(result==HIT)	printf("HIT_2\n");
	else		printf("MISS_2\n");

	result = L1->access(74232, 3);
	if(result==HIT)	printf("HIT_3\n");
	else		printf("MISS_3\n");

	result	=	L1->access(73976, 4);
	if(result==HIT)	printf("HIT_4\n");
	else		printf("MISS_4\n");
	// At this point, a set is 'FULL' 
	L1->Print();	


	// Below will 'evict' and 'replace' one entry
	result	=	L1->access(73720, 6);
	if(result==HIT)	printf("HIT_6\n");
	else		printf("MISS_6\n");
	L1->Print();	

	// Below will 'evict' and 'replace' another entry
	result = L1->access(73464, 7);
	if(result==HIT)	printf("HIT_7\n");
	else		printf("MISS_7\n");
	L1->Print();	

	// Below will 'evict' and 'replace' another entry
	result = L1->access(73208, 8);
	if(result==HIT)	printf("HIT_8\n");
	else		printf("MISS_8\n");
	L1->Print();	
	#endif
}
