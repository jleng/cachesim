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

int 	num_cores	= NUM_OF_CORES;	// Defined in mish.h
int 	L1_size		= 32*1024;
int 	L2_size		= 1024*1024;

// (TODO)
int	L1_block_size	= 16;
int	L1_assoc	= 2;
int	L2_block_size	= 128;
int	L2_assoc	= 2;

int	L1_hit_latency	= 5;
int	L1_miss_latency	= 20;
int	L2_hit_latency	= 20;
int	L2_miss_latency	= 200;	

bool	L2_banks_are_shared	= false;// true;//false;	// If false, then banks are partitioned

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

	#ifdef _SANITY_
	int	total_banks = 0;
	for(int i=0; i<num_cores; i++)
	{
		total_banks += L2_number_of_banks_each_core[i];
	}
	assert(total_banks==L2_NUM_OF_BANKS);
	#endif				


	//============================================================
	// A. Instantiate Core & Cache
	//============================================================
	//-------------------------------------------
	// a. Instantiate Cores & L1 caches
	//-------------------------------------------
	Core	*core[NUM_OF_CORES];
	Cache	*L1[NUM_OF_CORES];
	for(unsigned i=0; i<NUM_OF_CORES; i++)
	{
		core[i]	= new	Core(i);
        	L1[i]	= new 	Cache(i, 1, 0, L1_size, L1_block_size, L1_assoc, L1_hit_latency, L1_miss_latency,  "Level 1");
	}
	#ifdef _SANITY_
	assert((L2_size%L2_NUM_OF_BANKS)==0);
	#endif
	//-------------------------------------------
	// b. Instantiate L2 cache-banks 
	//-------------------------------------------
	Cache	*L2[L2_NUM_OF_BANKS];
	int	L2_size_per_bank	= (L2_size/L2_NUM_OF_BANKS);
	// Bank-Partitioning Unit for "Partition" mode (equal/unequal)
	bank_alloc_unit		bank_alloc_unit(L2_size, L2_NUM_OF_BANKS, L2_block_size/*LineSize*/, L2_assoc, L2_banks_are_shared);
	bank_alloc_unit.set_bank_partition_number_among_cores(L2_number_of_banks_each_core[0], L2_number_of_banks_each_core[1],L2_number_of_banks_each_core[2],L2_number_of_banks_each_core[3]);

	for(unsigned i=0; i<L2_NUM_OF_BANKS; i++)
	{
		L2[i]	= new	Cache(726, 2, i, L2_size_per_bank, L2_block_size, L2_assoc, L2_hit_latency, L2_miss_latency, "Level 2");
		L2[i]->set_bank_alloc_unit(&bank_alloc_unit);
	}
	//============================================================
	// B. Connect Cores->L1->L2
	//============================================================
	//-------------------------------------------
        // a. Connect Cores to its L1-caches (common to all mode)
	//-------------------------------------------
	for(unsigned i=0; i<NUM_OF_CORES; i++)
	{
		core[i]	->set_lower_level_request_q	( L1[i]->get_incoming_request_q() 	);
		L1[i]	->set_upper_level_serviced_q	( core[i]->get_serviced_q()		);
		L1[i]	->set_lower_level_request_q	( bank_alloc_unit.get_redirection_q() );	
		// All requests from L1->L2 are sent to "Bank-alloc-unit"
		// Bank-alloc-unit determines which bank to send the request to
	}

	//-------------------------------------------
        // b. Connect BankAllocUnit->L2->MEM
	//-------------------------------------------
	for(unsigned i=0; i<L2_NUM_OF_BANKS; i++)
	{
		// Connect 'bank_alloc_unit' -> L2	
		// => Basically a "CHANNEL" that is used to send request to each banks
		bank_alloc_unit.set_lower_level_request_q ( i, L2[i]->get_incoming_request_q() );
		// Connect L2->MEM
		L2[i]->set_lower_level_request_q	(NULL);
	}
	//-------------------------------------------
        // c. Connect BankAllocUnit <- L2
	//-------------------------------------------
	int	bank_offset	= 0;
	#ifdef _SANITY_
	int	last_bank_assigned = 0;
	#endif
	// C-0. Instantiate "Service_reporting_module" (Used for shared-banks mode)
	service_report_unit	L2_to_L1_redirect_unit;
	for(int i=0; i<NUM_OF_CORES; i++)
	{
		L2_to_L1_redirect_unit.set_upper_level_serviced_q(i, L1[i]->get_serviced_q());
	}
	// C-1. L2-banks are PARTITIONED
	if(L2_banks_are_shared==false)
	{
		for(int i=0; i<NUM_OF_CORES; i++)
		{
			#ifdef _DEBUG_
			cout<<"Core_"<<i<<" has "<<L2_number_of_banks_each_core[i]<<" banks allocated!"<<endl;
			printf("Bank_Offset=%d\n", bank_offset);
			#endif

			for(int j=bank_offset; j<(bank_offset+L2_number_of_banks_each_core[i]); j++)
			{
				L2[j]->set_upper_level_serviced_q	(L1[i]->get_serviced_q() );

				#ifdef _DEBUG_
				printf("\n Bank-%d is assiged to L1_of_Core-%d\n", j, i);
				#endif
				#ifdef _SANITY_
				assert( j < L2_NUM_OF_BANKS);
				last_bank_assigned	= j;
				#endif
			}
			bank_offset	+= L2_number_of_banks_each_core[i];
		}
	}
	// C-2. L2-banks are SHARED
	else	
	{
		for(int i=0; i<L2_NUM_OF_BANKS; i++)
		{
			L2[i]->set_upper_level_serviced_q	( L2_to_L1_redirect_unit.get_redirection_q() ); 
		}
	}
	#ifdef _SANITY_
	if(L2_banks_are_shared==false)	assert(last_bank_assigned==(L2_NUM_OF_BANKS-1));
	#endif

	#ifdef _DEBUG_
	printf("\n\n===========================================\n");
	printf("Addr=%x BankAddr=%d\n", 0x116,bank_alloc_unit.get_bank_addr(0x116));
	printf("Addr=%x BankAddr=%d\n", 0x226,bank_alloc_unit.get_bank_addr(0x226));
	printf("Addr=%x BankAddr=%d\n", 0x336,bank_alloc_unit.get_bank_addr(0x336));
	printf("Addr=%x BankAddr=%d\n", 0xCD6,bank_alloc_unit.get_bank_addr(0xCD6));
	printf("_Addr=%x BankAddr=%d\n", 0x116,L2[0]->get_bank_alloc_unit()->get_bank_addr(0x116));
	printf("_Addr=%x BankAddr=%d\n", 0x116,L2[3]->get_bank_alloc_unit()->get_bank_addr(0x226));
	printf("_Addr=%x BankAddr=%d\n", 0x116,L2[5]->get_bank_alloc_unit()->get_bank_addr(0x336));
	printf("_Addr=%x BankAddr=%d\n", 0x116,L2[13]->get_bank_alloc_unit()->get_bank_addr(0xCD6));
	#endif
	//============================================================
	// B. Get trace and execute Core & Cycle
	//============================================================

	//============================================================
	// *********OVERVIEW************
	// read a line from each trace file. each time find a trace with smallest cycle number to feed the simulator
	//============================================================
	int num_lines;
	// Trace information
	vector<addr_type> 		trace_addr;
	vector<addr_type> 		trace_cycle;
	vector<enum opcode>		is_STORE;
	vector<bool>			core_still_have_trace; // if one core has trace to read

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
					for(int i=0; i<NUM_OF_CORES; i++)
					{
						core[i]->advance_cycle();
					}
					// Execute all banks	
					for(int i=0; i<L2_NUM_OF_BANKS; i++)
					{
                		        	L2[i]->advance_cycle();
					}
					L2_to_L1_redirect_unit.advance_cycle();
					for(int i=0; i<NUM_OF_CORES; i++)
					{
			                        L1[i]->advance_cycle();
					}
					bank_alloc_unit.advance_cycle();
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
			               		core[0]->insert_incoming_request(is_STORE[i], trace_addr[i]);
						#ifdef _DEBUG_
						printf("[MSRHU][InsertRequest][Addr=%x(%d)][Cycle=%d][STORE=%d][CoreID=%d]\n", trace_addr[i], trace_addr[i], trace_cycle[i], is_STORE[i],i);
					        core[0]->print_stats();
						#endif
						break;

					case	1:
			               		core[1]->insert_incoming_request(is_STORE[i], trace_addr[i]);
						#ifdef _DEBUG_
						printf("[MSRHU][InsertRequest][Addr=%x(%d)][Cycle=%d][STORE=%d][CoreID=%d]\n", trace_addr[i], trace_addr[i], trace_cycle[i], is_STORE[i],i);
					        core[1]->print_stats();
						#endif
						break;

					case	2:
			               		core[2]->insert_incoming_request(is_STORE[i], trace_addr[i]);
						#ifdef _DEBUG_
						printf("[MSRHU][InsertRequest][Addr=%x(%d)][Cycle=%d][STORE=%d][CoreID=%d]\n", trace_addr[i], trace_addr[i], trace_cycle[i], is_STORE[i],i);
					        core[2]->print_stats();
						#endif
						break;

					case	3:
			               		core[3]->insert_incoming_request(is_STORE[i], trace_addr[i]);
						#ifdef _DEBUG_
						printf("[MSRHU][InsertRequest][Addr=%x(%d)][Cycle=%d][STORE=%d][CoreID=%d]\n", trace_addr[i], trace_addr[i], trace_cycle[i], is_STORE[i],i);
					        core[3]->print_stats();
						#endif
						break;

					default:
						printf("Invalid\n");
						assert(0);
						break;

				}
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
	bool	all_finished	= true;
	for(int i=0; i<NUM_OF_CORES; i++)
	{
		if(core[i]->request_all_finished()==false)
		{
			all_finished	= false;
			break;
		}
	}
	while(all_finished==false)
	{
		for(int i=0; i<NUM_OF_CORES; i++)
		{
			core[i]->advance_cycle();
		}
                //core[0]->advance_cycle();
		// Execute all banks	
		for(int i=0; i<L2_NUM_OF_BANKS; i++)
		{
	        	L2[i]->advance_cycle();
		}
		L2_to_L1_redirect_unit.advance_cycle();
		for(int i=0; i<NUM_OF_CORES; i++)
		{
                        L1[i]->advance_cycle();
		}
		bank_alloc_unit.advance_cycle();
		cpu_cycle++;

		// Check if finished
		all_finished	=  true;
		for(int i=0; i<NUM_OF_CORES; i++)
		{
			if(core[i]->request_all_finished()==false)
			{
				all_finished	= false;
				break;
			}
		}
	}
	// Finished execution
	printf("\n\n[End of Execution]Current CPU-CYcle=%lld\n\n", cpu_cycle);

	for(int i=0; i<NUM_OF_CORES; i++)
	{
		core[i]->print_stats();
		L1[i]->print_stats();
	}
	for(int i=0; i<L2_NUM_OF_BANKS; i++)
	{
		L2[i]->print_stats();
	}

}
