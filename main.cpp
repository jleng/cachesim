#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include "Cache.h"

using namespace std;

#define DEBUG
//parameters
int l1_dcache_size=16; // in KB
int l1_dcache_block_size=32; //in Byte
int l1_dcache_associativity=2;
int l1_dcache_hit_latency=5; //cycles;
int l1_dcache_miss_latency=200; //cycles;
int l1_dcache_num_banks=1; //cycles;

//main
int main(int argc, char* argv[]) {
	
	//input: now just need to specify the input file name
	if (argc != 2) {
		cout << "Please give me the trace file name!" << std::endl;
		exit(4);
	}

	string trace_file_name(argv[1]);
	ifstream trace_file;
	trace_file.open(trace_file_name.c_str(),ifstream::in);


	Cache l1;
	//l1.hello();
	l1.SetCacheSize(l1_dcache_size);
	l1.SetCacheBlockSize(l1_dcache_block_size);
	l1.SetCacheNumBanks(l1_dcache_num_banks);
	l1.SetCacheAssociativity(l1_dcache_associativity);
	l1.SetCacheHitLatency(l1_dcache_hit_latency);
	l1.SetCacheMissLatency(l1_dcache_miss_latency);
	string l1_cache_name("L1 Data Cache");
	l1.SetCacheName(l1_cache_name);
#ifdef DEBUG
	l1.Print();
#endif

	//read trace file
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
#ifdef DEBUG
			cout << "Parsing line: " << line << endl;
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
			
#ifdef DEBUG
			cout << setw(20) << "Address :" << hex << address << endl;
			cout << setw(20) << "CPU Cycle :" << dec << cpu_cycle << endl;
			if (is_load) {
				cout << "Load operation\n" << endl;
			} else {
				cout << "Store operation\n" << endl;
			}
#endif
		}
		
	}
	



}
