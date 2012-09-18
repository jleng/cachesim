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

int num_cores=4;
int L1_size=32*1024;
int L2_size=1024*1024;
vector<int> L2_number_of_banks_each_core;
vector<string> trace_file_name_each_core;
vector<ifstream*> trace_file_each_core;

int main(int argc, char* argv[]) {

        //input: now just need to specify the input file name
        if (argc < 6) {
                cout << "Provide at least 5 parameters!" << std::endl;
                cout << "Usage: " << argv[0] << " num_cores L1_size_in_\033[1;31mKB\033[0m L2_size_in_\033[1;31mKB\033[0m L2_num_banks_for_each_core trace_file_name_for_each_core" << std::endl;
                exit(4);
        } else { 
				
	}

	istringstream(argv[1]) >> num_cores;
	if (argc != (num_cores*2+4)) {
		cout << "No enough trace files or banks for " << num_cores << " cores!" << endl;
		exit(4);
	}
	
	istringstream(argv[2]) >> L1_size;
	L1_size*=1024;
	istringstream(argv[3]) >> L2_size;
	L2_size*=1024;

	L2_number_of_banks_each_core.clear();
	trace_file_name_each_core.clear();
	for (int i = 0; i < num_cores; i++) {
		int tmp;
		istringstream(argv[i+4]) >> tmp; 
		L2_number_of_banks_each_core.push_back(tmp);
		trace_file_name_each_core.push_back(string(argv[i+4+num_cores]));
		trace_file_each_core.push_back(new ifstream);
		trace_file_each_core[i]->open(trace_file_name_each_core[i].c_str(),ifstream::in);
	}

	cout << left << setw(40) << "Number of cores: " << num_cores << endl;
	cout << left << setw(40) << "L1 Size: " << L1_size << "\033[1;31mKB\033[0m" << endl;
	cout << left << setw(40) << "L2 Size: " << L2_size << "\033[1;31mKB\033[0m" << endl;
	for (int i = 0; i < num_cores; i++) {
		cout << left << setw(40) << "Core " << i << endl;
		cout << left << setw(40) << "number of banks allocated in L2 = " <<L2_number_of_banks_each_core[i] << endl;
		cout << left << setw(40) << "trace file name = " <<trace_file_name_each_core[i] << endl;
	}

				


	//============================================================
	// A. Instantiate Core & Cache
	//============================================================
        // Instantiate Modules
        Core Core_0(/*core_id*/0);
        Cache *L1       = new Cache(0, 1, 1, L1_size*1024, 32, 2, 5, 20,  "Level 1");
        Cache *L2       = new Cache(/*no meaning of core_id as L2 is shared*/726, 2, 1, L2_size*1024, 128, 2, 20, 200, "Level 2");

        // Connect Core_0->L1
        Core_0.set_lower_level_request_q        ( L1->get_incoming_request_q() );
        L1->set_upper_level_serviced_q          ( Core_0.get_serviced_q() );
        L1->set_lower_level_request_q           ( L2->get_incoming_request_q() );
        // Connect L1->L2
        L2->set_upper_level_serviced_q  	( L1->get_serviced_q() );
        L2->set_lower_level_request_q   	( NULL);


	//============================================================
	// B. Get trace and execute Core & Cycle
	//============================================================

	//============================================================
	// *********OVERVIEW************
	// read a line from each trace file. each time find a trace with smallest cycle number to feed the simulator
	//============================================================


num_cores	= 1;
	int num_lines;
	// Trace information
	vector<addr_type> 		trace_addr;
	vector<addr_type> 		trace_cycle;
	vector<enum opcode>		is_STORE;
	vector<bool>					core_still_have_trace; // if one core has trace to read
	bool all_trace_parsed = false;
	for ( int i=0; i<num_cores; i++) {
		string line;
		getline(*(trace_file_each_core[i]),line);
		stringstream line_stream(line);
		string field[3];
		for (int j = 0; j < 3; j++) {

			if (getline(line_stream, field[j], ' ')) {
#ifdef _DEBUG_
				cout << j << " filed:" << field[j] << endl;
#endif
			} else {
				cout << "Error: expect 3 fields\n";
			}
		}

		addr_type tmp;
		stringstream tmp_ss;
		tmp_ss << hex << field[0];
		tmp_ss >> tmp;
		trace_addr.push_back(tmp);
		istringstream(field[1]) >> tmp;
		trace_cycle.push_back(tmp);
		bool tmp_is_store = field[2].compare("LOAD");
		if(tmp_is_store==true)	is_STORE.push_back(STORE);
		else			is_STORE.push_back(LOAD);
#ifdef _DEBUG_
		printf("[MSRHU][Addr=%x(%d)][Cycle=%d][STORE=%d][CoreID=%d]\n", trace_addr[i], trace_addr[i], trace_cycle[i], is_STORE[i],i);
#endif
		core_still_have_trace.push_back(true);
	
	}

	while(!all_trace_parsed)
	{

		// select the the smallest cycle from trace files
		addr_type smallest_cycle=(addr_type) -1;
		for (int i=0; i < num_cores; i++) {
			if (core_still_have_trace[i]) {
				smallest_cycle = (smallest_cycle > trace_cycle[i]) ? trace_cycle[i] : smallest_cycle;
			}
		}

		for (int i=0; i < num_cores; i++) {
			if (core_still_have_trace[i] && smallest_cycle == trace_cycle[i]) 
			{

				/* *************************
				 * Execute until trace's CYCLE equals the smallest cycle
				 * *********************/
				while(cpu_cycle != smallest_cycle)
				{
		                        Core_0.advance_cycle();
                		        L2->advance_cycle();
		                        L1->advance_cycle();
					#ifdef _DEBUG_
					printf("[Executing Cache Cycles=%lld\n", cpu_cycle);
					#endif
					cpu_cycle++;
				}
				//printf("[MSRHU][Addr=%x(%d)][Cycle=%d][STORE=%d][CoreID=%d]\n", trace_addr[i], trace_addr[i], trace_cycle[i], is_STORE[i],i);
				/* *************************
				 * Insert the request for this core
				 * *********************/
				int	core_id	= i;
				switch(core_id)
				{		
					case	0:
						printf("[MSRHU][InsertRequest][Addr=%x(%d)][Cycle=%d][STORE=%d][CoreID=%d]\n", trace_addr[i], trace_addr[i], trace_cycle[i], is_STORE[i],i);
			               		Core_0.insert_incoming_request(is_STORE[i], trace_addr[i]);
					        Core_0.print_stats();
						break;

					default:
						printf("Invalid\n");
						assert(0);
						break;

				}
				//L1[core_id]->insert_incoming_request(0, is_STORE, trace_addr);
				/* *************************
				 * read the next trace info for this core
				 * *********************/
				string line;
				if (trace_file_each_core[i]->good()) {
					getline(*(trace_file_each_core[i]),line);
					if(line.size() > 0) {
						stringstream line_stream(line);
						string field[3];
						for (int j = 0; j < 3; j++) {

							if (getline(line_stream, field[j], ' ')) {
								#ifdef _DEBUG_
								cout << j << " filed:" << field[j] << endl;
								#endif
							} else {
								cout << "Error: expect 3 fields\n";
							}
						}

						addr_type tmp;
						stringstream tmp_ss;
						tmp_ss << hex << field[0];
						tmp_ss >> tmp;
						trace_addr[i] = tmp;
						istringstream(field[1]) >> tmp;
						trace_cycle[i] = tmp;
						bool tmp_is_store = field[2].compare("LOAD");
						if(tmp_is_store==true)	is_STORE[i] = STORE;
						else			is_STORE[i]= LOAD;
						#ifdef _DEBUG_
						printf("[MSRHU][NextTraceFetched][Addr=%x(%d)][Cycle=%d][STORE=%d][CoreID=%d]\n", trace_addr[i], trace_addr[i], trace_cycle[i], is_STORE[i],i);
						#endif
						core_still_have_trace[i] = true;
					} else {
						core_still_have_trace[i] = false;
					}
				}
			}
		}

		all_trace_parsed = true;
		for (int i=0; i < num_cores; i++) {
			if (core_still_have_trace[i])
				all_trace_parsed = false;
		}	
	}
        // Execute whatever's left in 'incoming-Queue'
        while(Core_0.request_all_finished()==false)
        {
                Core_0.advance_cycle();
                L2->advance_cycle();
                L1->advance_cycle();

                cpu_cycle++;
        }
	
	// Finished execution
	printf("\n\n[End of Execution]Current CPU-CYcle=%lld\n\n", cpu_cycle);
        Core_0.print_stats();
	L1->print_stats();
	L2->print_stats();
/*
	L1->print_queue_status();
	L1->print_cache_block_status();
	L2->print_queue_status();
	L2->print_cache_block_status();
*/

}
