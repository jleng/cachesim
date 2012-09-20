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
int 	L1_size		= 32*1024;
int 	L2_size		= 1024*1024/4;

// (TODO)
int	L1_block_size	= 16;
int	L1_assoc	= 2;
int	L2_block_size	= 128;
int	L2_assoc	= 2;

int	L1_hit_latency	= 5;
int	L1_miss_latency	= 20;
int	L2_hit_latency	= 20;
int	L2_miss_latency	= 200;	


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
	#ifndef _DEBUG_
        //cout << "\n\nInput number of lines to read in trace file: " << endl;
        //cin >> num_lines;
        //cout << "\n\n" << endl;
		printf("NumLines to Simulate=Whole File\n");
		num_lines	= -1;
	#else
	num_lines	= 10000;
	#endif

        string line;
	printf("NumLines to Simulate=%d\n", num_lines);

	//============================================================
	// A. Instantiate Core & Cache
	//============================================================
       // Instantiate Modules
        Core Core_0(/*core_id*/0);
        Cache *L1       = new Cache(0, 1, 1, L1_size, L1_block_size, L1_assoc, L1_hit_latency, L1_miss_latency,  "Level 1");
        Cache *L2       = new Cache(/*no meaning of core_id as L2 is shared*/726, 2, 1, L2_size, L2_block_size, L2_assoc, L2_hit_latency, L2_miss_latency, "Level 2");

        // Connect Core_0->L1
        Core_0.set_lower_level_request_q        ( L1->get_incoming_request_q() );
        L1->set_upper_level_serviced_q          ( Core_0.get_serviced_q() );
        L1->set_lower_level_request_q           ( L2->get_incoming_request_q() );
        // Connect L1->L2
        L2->set_upper_level_serviced_q  ( L1->get_serviced_q() );
        L2->set_lower_level_request_q   ( NULL);


	//============================================================
	// B. Get trace and execute Core & Cycle
	//============================================================

  //      for (int i =0; i< num_lines;i++) 
	addr_type parsed_line=0;
	while(trace_file.good())
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
												parsed_line++;
                        getline(trace_file,line);
							if (line.size() > 0) {
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
																				cout << "Parsing line: " << line << endl;
																				cout << "Parsed line: " << parsed_line << endl;
																				cout << "line size: " << line.size() << endl;
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
			#ifdef _DEBUG_
			printf("[MSRHU][Addr=%x(%d)][Cycle=%d][STORE=%d]\n", trace_addr, trace_addr, trace_cycle, is_STORE);
			#endif
		} else {
			break;
		}
		}
                // Execute until trace's CYCLE equals Core-Cache's cycle
                while(cpu_cycle!= trace_cycle)
                {
                        #ifdef _DEBUG_
                        printf("[Cpu_Cycle=%lld][TraceCycle=%lld]\n", cpu_cycle, trace_cycle);
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
                #ifdef _SANITY_
                assert(cpu_cycle==trace_cycle);
                #endif
                // If you're here, then trace_cycle==cpu_cycle => Insert mem-request-ops
                Core_0.insert_incoming_request(is_STORE, trace_addr);
        }

        // Execute whatever's left in 'incoming-Queue'
        while(Core_0.request_all_finished()==false)
        {
                #ifdef _DEBUG_
                printf("[CPU-Cycle=%lld]\n", cpu_cycle);
                #endif

                Core_0.advance_cycle();
                L2->advance_cycle();
                L1->advance_cycle();

                cpu_cycle++;
        }



        printf("Current CPU-CYcle=%lld\n", cpu_cycle);


        Core_0.print_stats();
        L1->print_stats();
        L2->print_stats();






}
