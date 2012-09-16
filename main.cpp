#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <vector>
#include "Cache.h"
#include "misc.h"

using namespace std;

unsigned long long	cpu_cycle = 0;

//vector<mem_request_t>	core_0_serviced_req_q;


int main(int argc, char* argv[]) {

        //input: now just need to specify the input file name
        if (argc != 2) {
                cout << "Please give me the trace file name!" << std::endl;
                exit(4);
        }

        string trace_file_name(argv[1]);
        ifstream trace_file;
        trace_file.open(trace_file_name.c_str(),ifstream::in);

        int num_lines;
	#ifdef _DEBUG_
        cout << "\n\nInput number of lines to read in trace file: " << endl;
        cin >> num_lines;
        cout << "\n\n" << endl;
	#else
	num_lines	= 10000;
	#endif

        string line;
	printf("NumLines to Simulate=%d\n", num_lines);

	//============================================================
	// A. Instantiate Core & Cache
	//============================================================
	/*
		Cache(int core_id, int cache_level, int num_banks, int cache_size, int block_size, int assoc, int hit_latency, int miss_latency, string name, vector<mem_request_t> *upper_level_serviced_q, vector<mem_request_t> *lower_level_req_q);
	*/
	Core Core_0(/*core_id*/0);
	// Instantiate "L1D" for core-0 
	Cache *L1	= new Cache(0, 1, 1, 1024, 16, 4, 3, 10,  "Level 1");
	Cache *L2	= new Cache(/*no meaning of core_id as L2 is shared*/726, 2, 1, 1024, 16, 4, 10, 20, "Level 2");

	// Connect Core_0->L1
	L1->set_upper_level_serviced_q( Core_0.get_serviced_q() );
	L1->set_lower_level_request_q( L2->get_incoming_request_q() );
	// Connect L1->L2
	L2->set_upper_level_serviced_q	( L1->get_serviced_q() );
	L2->set_lower_level_request_q	( NULL);

	//============================================================
	// B. Get trace and execute Core & Cycle
	//============================================================

        for (int i =0; i< num_lines;i++) 
	{
			// Trace information
                        addr_type 		trace_addr	= 0;
                        unsigned long long 	trace_cycle	= 0;
			enum opcode		is_STORE;
                        bool 			is_store	= false;

		// Get 'One' trace
                if (trace_file.good()) 
		{
                        //trace_file.getline(line);
                        getline(trace_file,line);
                        stringstream line_stream(line);
                        string field[3];
			#ifdef _DEBUG_
                        cout << "Parsing line: " << line << endl;
			#endif
                        for (int j = 0; j < 3; j++) {

                                if (getline(line_stream, field[j], ' ')) {
					#ifdef _DEBUG_
                                        cout << j << " filed:" << field[j] << endl;
					#endif
                                } else {
                                        cout << "Error: expect 3 fields\n";
                                }
                        }

                        stringstream tmp_ss;
                        tmp_ss << hex << field[0];
                        tmp_ss >> trace_addr;
                        istringstream(field[1]) >> trace_cycle;
                        is_store = field[2].compare("LOAD");
			if(is_store==true)	is_STORE	= STORE;
			else			is_STORE	= LOAD;
			#ifdef _DEBUG_
                        cout << string(5, ' ') << setw(20) << "Address :" << "0x" << hex << trace_addr << endl;
                        cout << string(5, ' ') << setw(20) << "CPU Cycle :" << dec << trace_cycle << endl;
			#endif
			#ifndef _DEBUG_
			printf("[MSRHU][Addr=%x(%d)][Cycle=%d][STORE=%d]\n", trace_addr, trace_addr, trace_cycle, is_STORE);
			#endif
		}

		// Execute until trace's CYCLE equals Core-Cache's cycle
		while(cpu_cycle!= trace_cycle)
		{
			#ifdef _DEBUG_
			printf("\n\n-----------------------<Start> Cycle=%lld--------------------\n", cpu_cycle);
			#endif
			Core_0.advance_cycle();
			L2->advance_cycle();
			L1->advance_cycle();
			#ifdef _DEBUG_
			L1->print_queue_status();
			L1->print_cache_block_status();
			L2->print_queue_status();
			L2->print_cache_block_status();
			#endif
			cpu_cycle++;
			#ifdef _DEBUG_
			printf("=====================End of cycle = %lld=================\n", cpu_cycle);
			#endif
		}
		// If you're here, then trace_cycle==cpu_cycle => Insert mem-request-ops
		#ifdef _SANITY_
		assert(cpu_cycle==trace_cycle);
		#endif
		L1->insert_incoming_request(0, is_STORE, trace_addr);
        }
	printf("Current CPU-CYcle=%lld\n", cpu_cycle);
	L1->print_stats();
	L2->print_stats();
/*
	L1->print_queue_status();
	L1->print_cache_block_status();
	L2->print_queue_status();
	L2->print_cache_block_status();
*/

	/*
	#ifdef _DEBUG_
	//L1->Print();	
	// The following 'four' accesses will fill in '4-entries' in a set (this example is 4-way associativity cache, so a single set will be full after four insertions)
	L1->insert_incoming_request(4, LOAD, 74744);
	L1->print_queue_status();
	cpu_cycle++;

	L1->insert_incoming_request(3, STORE, 74488);
	cpu_cycle++;
	L1->print_queue_status();

	L1->insert_incoming_request(2, LOAD, 74232);
	L1->print_queue_status();
	cpu_cycle++;

	L1->insert_incoming_request(1,STORE, 73976);
	L1->print_queue_status();
	cpu_cycle++;


	L1->insert_incoming_request(0, LOAD, 73720);
	L1->print_queue_status();
	cpu_cycle++;

	L1->insert_incoming_request(5, STORE, 73464);
	L1->print_queue_status();
	cpu_cycle++;

	L1->insert_incoming_request(6,LOAD, 73208);
	L1->print_queue_status();
	cpu_cycle++;


	// Service request
	printf("[SERVICING]\n");

	for(unsigned i=0; i<300; i++)
	{
		printf("\n\n-----------------------<Start> Cycle=%lld--------------------\n", cpu_cycle);
		Core_0.advance_cycle();
		L2->advance_cycle();
		L1->advance_cycle();
		L1->print_queue_status();
		L1->print_cache_block_status();
		L2->print_queue_status();
		L2->print_cache_block_status();

		cpu_cycle++;

		printf("=====================End of cycle = %lld=================\n", cpu_cycle);
	}

	L1->insert_incoming_request(100,STORE, 73208);
	L1->insert_incoming_request(101,STORE, 28920);

	printf("[TEST_HIT]\n");
	for(unsigned i=0; i<40; i++)
	{
		printf("\n\n-----------------------<Start> Cycle=%lld--------------------\n", cpu_cycle);

		Core_0.advance_cycle();
		L2->advance_cycle();
		L1->advance_cycle();
		L1->print_queue_status();
		L1->print_cache_block_status();
		L2->print_queue_status();
		L2->print_cache_block_status();

		cpu_cycle++;
		printf("=====================End of cycle = %lld=================\n", cpu_cycle);

	}
	#endif
	*/
}
