#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include "Cache.h"
#include "misc.h"

using namespace std;

int l1_dcache_core_id;
int l1_dcache_num_banks=1; //banks
int l1_dcache_size=1024; // in B
int l1_dcache_block_size=16; //in Byte
int l1_dcache_associativity=4;
int l1_dcache_hit_latency=7; //cycles;
int l1_dcache_miss_latency=26; //cycles;

int main(int argc, char* argv[]) {

	//input: now just need to specify the input file name
	if (argc != 2) {
		cout << "Please give me the trace file name!" << std::endl;
		exit(4);
	}

	string trace_file_name(argv[1]);
	ifstream trace_file;
	trace_file.open(trace_file_name.c_str(),ifstream::in);

	/*
	Cache(int core_id, int num_banks, int cache_size, int block_size, int assoc, int hit_latency, int miss_latency, string name): m_tag_array(core_id, (cache_size/block_size)), m_num_banks(num_banks), m_size(cache_size),m_block_size(block_size), m_associativity(assoc), m_hit_latency(hit_latency), m_miss_latency(miss_latency), m_name(name)
	*/
	// Instantiate "L1D" for core-0
	//Cache *L1	= new Cache(0, 1, 1024, 16, 4, 7, 26, "Try this");
	Cache *L1	= new Cache(l1_dcache_core_id, l1_dcache_num_banks, l1_dcache_size, l1_dcache_block_size, l1_dcache_associativity, l1_dcache_hit_latency, l1_dcache_miss_latency, "Try this");

	#ifdef _DEBUG_
	L1->Print();	

	// The following 'four' accesses will fill in '4-entries' in a set (this example is 4-way associativity cache, so a single set will be full after four insertions)
	/*enum cache_access_status result	= L1->access(74744, 1);
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
	*/
	#endif

	int num_lines;
	cout << "\n\nInput number of lines to read in trace file: " << endl;
	cin >> num_lines;

	cout << "\n\n" << endl;

	string line;
	for (int i =0; i< num_lines;i++) {
		if (trace_file.good()) {
			//trace_file.getline(line);
			getline(trace_file,line);
			stringstream line_stream(line);
			string field[3];
#ifdef _DEBUG_
			//cout << "Parsing line: " << line << endl;
#endif

			for (int j = 0; j < 3; j++) {

				if (getline(line_stream, field[j], ' ')) {
					//cout << j << " filed:" << field[j] << endl;
				} else {
					cout << "Error: expect 3 fields\n";
				}
			}

			unsigned long address;
			unsigned long cpu_cycle;
			bool is_load=false;
			stringstream tmp_ss;
			tmp_ss << hex << field[0];
			tmp_ss >> address;
			istringstream(field[1]) >> cpu_cycle;
			is_load=field[2].compare("STORE");	
			
#ifdef _DEBUG_
			cout << string(5, ' ') << setw(20) << "Address :" << "0x" << hex << address << endl;
			cout << string(5, ' ') << setw(20) << "CPU Cycle :" << dec << cpu_cycle << endl;
#endif
			enum cache_access_status result	= L1->access(address, cpu_cycle);
			if(result==HIT)	printf("HIT\n");
			else		printf("MISS\n");
			}

			cout << "\n";
		
	}
	

}
