#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include "Cache.h"
#include "misc.h"

extern unsigned long long	cpu_cycle;
//
using namespace std;



bank_alloc_unit::bank_alloc_unit(int L2_cache_size, int num_of_banks, int block_size, int assoc, bool L2_banks_are_shared)
{
	m_L2_cache_size		= L2_cache_size;
	m_num_of_banks		= num_of_banks;
	m_block_size		= block_size;
	m_assoc			= assoc;
	m_L2_banks_are_shared	= L2_banks_are_shared;

	m_per_bank_cache_size	= (L2_cache_size/num_of_banks);
	m_per_bank_num_of_lines	= (m_per_bank_cache_size/block_size);
	m_per_bank_num_of_sets	= (m_per_bank_num_of_lines)/assoc;


	// Overall L2-cache	
	m_all_num_of_lines	= L2_cache_size/block_size;
	m_all_num_of_sets	= m_all_num_of_lines/assoc;

	// Common
	m_block_offset_bits	= log2(block_size);
	m_set_bits_for_all	= log2(m_all_num_of_sets);
	m_set_bits_per_bank	= log2(m_per_bank_num_of_sets);

	m_bank_offset_bits	= log2(num_of_banks);
	#ifdef _DEBUG_
	printf("Per_Bank_CacheSize=%d\n", m_per_bank_cache_size);
	printf("Per_Bank_Num_of_Lines=%d\n", m_per_bank_num_of_lines);
	printf("Per_Bank_Num_of_Sets=%d\n", m_per_bank_num_of_sets);
	printf("All_NumLines=%d\n", m_all_num_of_lines);
	printf("All_NumSets=%d\n", m_all_num_of_sets);
	printf("BlockOffsetBits=%d\n", m_block_offset_bits);
	printf("SetBits_ALL=%d\n", m_set_bits_for_all);
	printf("SetBits_PerBank=%d\n", m_set_bits_per_bank);
	#endif

	#ifdef _SANITY_
	assert((L2_cache_size%num_of_banks)==0);
	assert(m_per_bank_num_of_lines==((L2_cache_size/block_size)/num_of_banks));
	assert(m_all_num_of_sets==(m_per_bank_num_of_sets*num_of_banks));
	assert(m_set_bits_for_all==(m_set_bits_per_bank+m_bank_offset_bits));
	#endif
}

void bank_alloc_unit::print_queue_status()
{
	printf("\n\n[Current Cycle=%lld]\n",cpu_cycle);

	cout<<"[Redirection Queue Status] Size = "<<m_L1_to_L2_redirection_q.size()<<endl;
	vector<mem_request_t>::iterator it;
	for(it=m_L1_to_L2_redirection_q.begin(); it<m_L1_to_L2_redirection_q.end(); it++)
	{
		if((*it).m_opcode==STORE)		printf("[Opcode = ST]");
		else{	assert((*it).m_opcode==LOAD);	printf("[Opcode = LD]"); };
		
		printf("[WB=%d][Req_Core=%d][AccessAddr=%8x][CyclesLeft=%d][Requested_Time=%d]\n", (*it).m_is_writeback,(*it).m_core_id, (*it).m_access_addr, (*it).m_cycles_left_for_service, (*it).m_first_request_time);		
	}
	printf("\n");
}

void service_report_unit::advance_cycle()
{
	if(m_L2_to_L1_redirection_q.size()>0)
	{
		while(!m_L2_to_L1_redirection_q.empty())
		{
			mem_request_t	this_request	= m_L2_to_L1_redirection_q.back();
			int		req_core_id	= this_request.m_core_id;
			addr_type	req_access_addr	= this_request.m_access_addr;
			#ifdef _SANITY_
			assert(req_core_id<NUM_OF_CORES);
			assert(req_access_addr!=0x0);
			#endif

			// Insert to corresponding bank
			m_upper_level_serviced_q[req_core_id]->insert(m_upper_level_serviced_q[req_core_id]->begin(), this_request);

			// Erase redirected request
			m_L2_to_L1_redirection_q.pop_back();
		}
	}
	#ifdef _DEBUG_
	else
	{
		printf("Redirection Q is EMPTY!!\n");
	}
	#endif	
	#ifdef _SANITY_
	assert(m_L2_to_L1_redirection_q.size()==0);
	#endif
}

void bank_alloc_unit::advance_cycle()
{
	// If a request needs to be redirected to L2 ...
	if(m_L1_to_L2_redirection_q.size()>0)
	{
		while(!m_L1_to_L2_redirection_q.empty())
		{
			mem_request_t	this_request	= m_L1_to_L2_redirection_q.back();
			int		req_core_id	= this_request.m_core_id;
			addr_type	req_access_addr	= this_request.m_access_addr;
			int		req_bank_id	= -1;
			// Partitioned L2
			if(m_L2_banks_are_shared==false)
			{
				req_bank_id	= get_bank_id_when_partitioned(req_core_id, req_access_addr);
			}
			// Shared L2
			else
			{
				req_bank_id	= get_bank_id_when_shared(req_core_id, req_access_addr);
			}
			// Insert to corresponding bank
			m_lower_level_request_q[req_bank_id]->insert(m_lower_level_request_q[req_bank_id]->begin(), this_request);

			m_L1_to_L2_redirection_q.pop_back();
		}
	}
	#ifdef _DEBUG_
	else
	{
		printf("Redirection Q is EMPTY!!\n");
	}
	#endif	
	#ifdef _SANITY_
	assert(m_L1_to_L2_redirection_q.size()==0);
	#endif
}


Cache::Cache(int core_id, int cache_level, int bank_id, int cache_size, int block_size, int assoc, int hit_latency, int miss_latency, string name, bool L2_banks_are_shared): m_core_id(core_id), m_cache_level(cache_level), m_tag_array(core_id, (cache_size/block_size), assoc), m_bank_id(bank_id), m_size(cache_size),m_block_size(block_size), m_associativity(assoc), m_hit_latency(hit_latency), m_miss_latency(miss_latency), m_name(name), m_L2_banks_are_shared(L2_banks_are_shared)

{
	// Get num of lines in cache
	m_num_of_lines	= (cache_size/block_size);

	// Derive info to configure set-associativity
	m_num_of_sets			= m_num_of_lines / assoc;
	m_block_offset_bits		= log2(block_size);
	m_set_bits			= log2(m_num_of_sets);

	// Used for "RRB"
	m_prioritized_core_id		= 0;

	// Clear stats
	m_num_accesses	= 0;
	m_num_hits	= 0;
	m_num_misses	= 0;

	#ifdef _SANITY_
	assert((m_num_of_lines%m_associativity)==0);
	assert((cache_size%block_size)==0);
	assert((cache_size%1024)==0);
	assert(block_size == pow(2.0, m_block_offset_bits));
	assert(m_num_of_sets == pow(2.0, m_set_bits));
	assert( (m_num_of_sets*m_associativity*m_block_size)==m_size );
	#endif			
}

enum cache_access_status Cache::access(int req_core_id, unsigned is_write, addr_type access_addr, unsigned cycle_time)
{
	addr_type	set_idx 	= get_set_index(access_addr);
	addr_type	tag_value	= get_tag_value(req_core_id, access_addr);
	#ifdef _DEBUG_
	printf("[AccessAddr=%8x] SetIdx=%x Tag=%x\n", access_addr,set_idx, tag_value);
	#endif
	// Below will 'ONLY' have cache-blocks modified to 'DIRTY' when it's a HIT -- otherwise, it only tells you whether it's a MISS-ALLOCATE or MISS-REPLACE
	return m_tag_array.access( is_write, access_addr, set_idx, tag_value, cycle_time);
}	

void	Cache::advance_one_incoming_request()
{
	// If a pending-request exists ...
	if(m_in_request_q.size()>0)
	{
		#ifdef _DEBUG_
		printf("\n[L%d][Incoming Request Queue OCCUPIED!!! at cycle=%lld]\n", m_cache_level, cpu_cycle);
		#endif
		vector<mem_request_t>::iterator to_be_serviced_req	= m_in_request_q.begin();
		unsigned long long		oldest_time	= (*to_be_serviced_req).m_first_request_time;
		int				oldest_core_id	= (*to_be_serviced_req).m_core_id;
		#ifdef _DEBUG_
		printf("[L%d][Search starts with BEGIN as...] [Req_Core=%d][AccessAddr=%8x][CyclesLeft=%d][Requested_Time=%d]\n", m_cache_level,(*to_be_serviced_req).m_core_id, (*to_be_serviced_req).m_access_addr, (*to_be_serviced_req).m_cycles_left_for_service, (*to_be_serviced_req).m_first_request_time);		
		#endif
		// Determine oldest time
		vector<mem_request_t>::iterator it;
		for(it=m_in_request_q.begin(); it<m_in_request_q.end(); it++)
		{
			// An older request exists!
			if( (*it).m_first_request_time < oldest_time )
			{
				oldest_time		= (*it).m_first_request_time;
			}
		}
		// Track 'core-id's requested simultanesouly
		int	num_cores_requested_simultaneously	= 0;
		int	core_ids_requested[NUM_OF_CORES];	memset(core_ids_requested, 10/*dummy*/, sizeof(int)*NUM_OF_CORES);

		bool	prioritized_core_id_exists	= false;
		// Find cores requested at 'oldest-time'
		for(it=m_in_request_q.begin(); it<m_in_request_q.end(); it++)
		{
			if( (*it).m_first_request_time == oldest_time )
			{
				core_ids_requested[num_cores_requested_simultaneously]	= (*it).m_core_id;
				num_cores_requested_simultaneously++;	

				if( (*it).m_core_id == m_prioritized_core_id )
				{
					prioritized_core_id_exists	= true;
				}
			}			
		}
		int	selected_core_id	= 10;
		if(num_cores_requested_simultaneously==1)
		{
			selected_core_id	= core_ids_requested[0];
		}
		else
		{
			#ifdef _SANITY_
			assert(num_cores_requested_simultaneously>1);
			#endif
			if(prioritized_core_id_exists==true)
			{
				#ifdef _SANITY_
				bool	arrived	= false;
				#endif
				for(int i=0; i<num_cores_requested_simultanously; i++)
				{
					if(core_ids_requested[i] == m_prioritized_core_id)
					{
						selected_core_id	= m_prioritized_core_id;
						// Rotate priority
						(++m_prioritized_core_id)%(NUM_OF_CORES);
						#ifdef _SANITY_
						arrived	= true;
						#endif
						break;
					}
				}	
				#ifdef _SANITY_
				assert(arrived==true);
				#endif
			}
			else	
			{
				#ifdef _SANITY_
				int	CNT = 0;
				#endif
				bool	found_core	= false;
				int	priority		= (++m_prioritized_core_id)%(NUM_OF_CORES);
				while(found_core==false)
				{
					for(int i=0; i<num_cores_requested_simultaneously; i++)
					{
						if(core_ids_requested[i] == priority)
						{
							selected_core_id	= priority;
							found_core		= true;
							break;
						}
					}
					// If not found, rotate priority
					if(found_core==false)
					{
						priority = (++priority)%(NUM_OF_CORES);
						CNT++;
					}
					#ifdef _SANITY_	
					// Check deadlock
					if(CNT>10)	assert(0);
					#endif
				}
			}
		}
		#ifdef _SANITY_
		assert((selected_core_id>=0)&&(selected_core_id<NUM_OF_CORES));
		#endif
		// Get 'to-be-serviced-core-id' info
		for(it=m_in_request_q.begin(); it<m_in_request_q.end(); it++)
		{
			// An older request exists!
			if(( (*it).m_first_request_time == oldest_time)&&( (*it).m_core_id==selected_core_id ))
			{
				oldest_core_id	= (*it).m_core_id;
				to_be_serviced_req	= it;
				break;	
			}
		}

		#ifdef _SANITY_
		assert(oldest_core_id!=-1);
		#endif
		#ifdef _DEBUG_
		printf("[L%d][Incoming request CHOSEN for SERVICE]!!! [Serviced_Core=%d][AccessAddr=%8x][CyclesLeft=%d][Requested_Time=%d]\n", m_cache_level,(*to_be_serviced_req).m_core_id, (*to_be_serviced_req).m_access_addr, (*to_be_serviced_req).m_cycles_left_for_service, (*to_be_serviced_req).m_first_request_time);		
		#endif

		// Service the request
		int		req_core_id		= (*to_be_serviced_req).m_core_id;	
		enum opcode	req_op			= (*to_be_serviced_req).m_opcode;
		addr_type	req_access_addr		= (*to_be_serviced_req).m_access_addr;	
		addr_type	req_init_requested_time	= (*to_be_serviced_req).m_first_request_time;
		bool		req_is_writeback	= (*to_be_serviced_req).m_is_writeback;
		#ifdef _SANITY_
		if(req_is_writeback==true)
		{
			assert(m_cache_level!=1);	// Writeback requests in a "incoming-service-queue" are only available in lower-than-L1 caches
			assert(req_op==STORE);
		}
		#endif

		#ifdef _STAT_
		//---------------------------------------
		// Increase num-of-accesses to this cache
		m_num_accesses++;	
		//---------------------------------------
		#endif

		// Below will 'ONLY' have cache-blocks modified to 'DIRTY' when it's a HIT -- otherwise, it only tells you whether it's a MISS-ALLOCATE or MISS-REPLACE
		enum cache_access_status 	result	= access( req_core_id, req_op, req_access_addr, cpu_cycle);

#ifdef _DEBUG_
//if(req_access_addr == 48311296)
if(1)
{
	addr_type	set_idx 	= get_set_index(req_access_addr);
	addr_type	tag_value	= get_tag_value(req_core_id, req_access_addr);
	printf("\n[DEBUG][L%d][STORE=%d][HIT=%d] Addr=%x(%d) QUERIED at CYCLE=%lld\n",m_cache_level, req_op, result, req_access_addr, req_access_addr, cpu_cycle);
	printf("SetIdx=%x Tag=%x\n\n", set_idx, tag_value);

}
#endif

		switch(result)
		{
			case	HIT:
				#ifdef _STAT_
				//---------------------------------------
				// Increase num-of-hits to this cache
				m_num_hits++;	
				//---------------------------------------
				#endif

				#ifdef _DEBUG_
				printf("HIT!\n");
				#endif
				// If this is a writeback and is a HIT in this level then NO need to notify above level
				if(req_is_writeback==true)
				{
					// If writeback is a HIT, then access() above will have made the cache-block DIRTY! (no other operation necessary)
					// NO NEED to notify upper level about "Writeback-HIT"
				}
				else
				{
					// Reply back to 'upper' level hierarchy to inform 'HIT' -- insert 'req' info to 'upper-level-q
					if(m_upper_level_serviced_q!=NULL)
					{
						mem_request_t	this_request(req_core_id, req_op, req_access_addr, req_init_requested_time, m_hit_latency, false);
	
						m_upper_level_serviced_q->insert(m_upper_level_serviced_q->begin(), this_request);
						// (NOTE!!) For "L2" cache, "m_upper_level_serviced_q" should be "L1's m_serviced_q"
					}
					else	assert(0);
				}	
				break;

			case	MISS_WITH_ENTRY_ALLOCATION:
			case	MISS_WITH_ENTRY_REPLACEMENT:
				#ifdef _STAT_
				//---------------------------------------
				// Increase num-of-misses to this cache
				m_num_misses++;	
				//---------------------------------------
				#endif

				if(req_is_writeback==true)
				{
					#ifdef _SANITY_
					assert(req_op==STORE);
					#endif
					// Below level is 'Main-memory', so have it serviced later in N-cycles 
					if(m_lower_level_request_q==NULL)
					{
						#ifdef _DEBUG_
						printf("You don't have a LOWER-LEVEL assigned -- It's MEMORY! (A Writeback to MEMORY requires no other operation)\n");
						#endif
					}
					// There's a below-level CACHE, so insert it in it's incoming q
					else
					{
						// If this request is an 'incoming-writeback' then I'm L2, so below SHOULD-BE MEMORY (no L3 in this project)
						assert(0);
						mem_request_t	writeback_request(req_core_id, STORE/* can be req_op */, req_access_addr, req_init_requested_time, 1818, true);	
						m_lower_level_request_q->insert(m_lower_level_request_q->begin(), writeback_request);
					}
				}
				else
				{
					// Below level is 'Main-memory', so have it serviced later in N-cycles 
					if(m_lower_level_request_q==NULL)
					{
						#ifdef _DEBUG_
						printf("You don't have a LOWER-LEVEL assigned -- It's MEMORY!\n");
						#endif
						mem_request_t	serviced_from_memory_req(req_core_id, req_op, req_access_addr, req_init_requested_time, m_miss_latency, false);	// 999 is the 'memory-service latency'	
						m_serviced_q.insert(m_serviced_q.begin(), serviced_from_memory_req);	
					}
					// There's a below-level CACHE, so insert it in it's incoming q
					else
					{
						mem_request_t	this_request(req_core_id, req_op, req_access_addr, req_init_requested_time, 1818, false);	
	
						m_lower_level_request_q->insert(m_lower_level_request_q->begin(), this_request);
						// (NOTE!!) For "L1" cache, 'm_lower_level_request_q" should be "L2's m_in_request_q" 
					}
				}
				break;
			default:
				printf("Invalid access results\n");
				assert(0);
				break;
		}
		// Delete the 'serviced' request from 'in-request-q'
		m_in_request_q.erase(to_be_serviced_req);
	}
	#ifdef _DEBUG_
	else
	{
		printf("\n\n[L%d][Incoming Request Queue EMPTY!!! at cycle=%lld]\n\n", m_cache_level, cpu_cycle);
	}
	#endif

}

void	Cache::advance_one_serviced_request()
{
	// If a pending-request exists ...
	if(m_serviced_q.size()>0)
	{
		#ifdef _DEBUG_
		printf("\n\n\n[L%d][Serviced Queue OCCUPIED!!! at cycle=%lld]\n", m_cache_level, cpu_cycle);
		#endif
		vector<mem_request_t>::iterator it;
		for(it=m_serviced_q.begin(); it<m_serviced_q.end(); it++)
		{
			// Decrement 'CyclesLeft'
			(*it).m_cycles_left_for_service--;
			#ifdef _DEBUG_
			printf("[L%d][AFTER][Serviced_Core=%d][AccessAddr=%8x][CyclesLeft=%d][Requested_Time=%d]\n",m_cache_level, (*it).m_core_id, (*it).m_access_addr, (*it).m_cycles_left_for_service, (*it).m_first_request_time);	
			#endif
			// When 'CyclesLeft' is ZERO, then 1) allocate that block to 'this cache' and delete it from QUEUE,,, 2) inform 'HIT' to upper-level
			if( (*it).m_cycles_left_for_service==0 )
			{
				#ifdef _SANITY_
				assert((*it).m_is_writeback==false);
				#endif
				int		serviced_core_id	= (*it).m_core_id;
				addr_type	serviced_access_addr	= (*it).m_access_addr;
				enum opcode	serviced_opcode		= (*it).m_opcode;
				addr_type	serviced_init_req_time	= (*it).m_first_request_time;

				addr_type	set_idx 		= get_set_index(serviced_access_addr);
				addr_type	tag_value		= get_tag_value(serviced_core_id, serviced_access_addr);
				#ifdef _DEBUG_
				printf("\n\n[SERVICED_Q_latency_is_ZERO!!!][L%d-cache][Cycle=%lld] [Opcode=%d][AccessAddr=%8x][InitReqTime=%lld] SetIdx=%x Tag=%x\n", m_cache_level,cpu_cycle,serviced_opcode, serviced_access_addr, serviced_init_req_time, set_idx, tag_value);
				print_queue_status();
				#endif
	
				// "ALLOCATE' a block at "this-cache"
				// (In case a dirty block is evicted, use it to request a "WRITE" down the lower level)
				cache_block_t	evicted_block;
				bool		dirty_block_evicted = false;

				enum cache_access_status access_result = m_tag_array.allocate_block( serviced_opcode, serviced_access_addr, set_idx, tag_value, cpu_cycle, &evicted_block, &dirty_block_evicted);
				switch(access_result)
				{
					case	HIT:					
						// This means that a concurrent MISS closer than this MISS has come back earlier.
						break;

					case	MISS_WITH_ENTRY_ALLOCATION:
						// No need to do anything -- the block would've been allocated at 'allocate-block()'
						#ifdef _SANITY_
						assert(dirty_block_evicted==false);
						assert(evicted_block.m_block_addr==0);
						assert(evicted_block.m_status==INVALID);
						#endif
						break;

					case	MISS_WITH_ENTRY_REPLACEMENT:
						if(dirty_block_evicted==true)
						{
							#ifdef _SANITY_
							assert(evicted_block.m_block_addr!=0);
							assert(evicted_block.m_status==DIRTY);
							#endif
							// Below level is 'Main-memory', so have it serviced later in N-cycles 
							if(m_lower_level_request_q==NULL)
							{
								#ifdef _DEBUG_
								printf("\n\n=============================================================================\n");
								printf("You don't have a LOWER-LEVEL to this Cache-LEVEL -- Below is MEMORY! and this is WRITEBACK!!\n");
								printf("=============================================================================\n");
								// Nothing particular need to happen as this is the lowest-level in cache
								#endif
							}
							// There's a below-level CACHE, so insert it in it's incoming q
							else
							{
								#ifdef _DEBUG_
								printf("[This is the evicted core] [L%d][WRITEBACK][Opcode=%d][AccessAddr=%8x]\n", m_cache_level,STORE, evicted_block.m_block_addr);
								print_cache_block_status();
								#endif
								// THIS is where 'writeback' request is "FIRST" generated!!!!
								mem_request_t	writeback_request(m_core_id, STORE, evicted_block.m_block_addr, cpu_cycle, 1818, true);	
								m_lower_level_request_q->insert(m_lower_level_request_q->begin(), writeback_request);
							}
						}
						break;

					default:
						printf("Invalid access results\n");
						assert(0);
						break;
				}
#ifdef _DEBUG_
if(serviced_access_addr == 48311296)
{
	printf("[DEBUG][L%d][STORE=%d] Addr=%x(%d) accessed at CYCLE=%lld\n\n",m_cache_level,serviced_opcode, serviced_access_addr, serviced_access_addr, cpu_cycle);
}
#endif

				// If this is L1 and is a 'write', then we should "MARK" this block as "DIRTY"
				#ifdef _SANITY_
				unsigned dummy_idx;
				assert( HIT==m_tag_array.probe(set_idx, tag_value, dummy_idx));
				#endif
				if(m_cache_level==1)	// (TODO) Or maybe both L1/2 (ACTUALLY, I think BOTH should NOT be dirty-fied)!!!
							// -> What makes sense is have BOTH the allocated entry CLEAN
				{
					if(serviced_opcode==STORE)
					{
						m_tag_array.mark_dirty_if_needed(serviced_opcode, serviced_access_addr, set_idx, tag_value, cpu_cycle);
					}
				}
				// Delete the 'this request' from queue
				m_serviced_q.erase(it);

				// Reply back to 'upper' level hierarchy to inform 'HIT' -- insert 'req' info to 'upper-level-q
				if(m_upper_level_serviced_q!=NULL)
				{
					// Writeback-requests NEVER goes up!!
					mem_request_t	this_request(serviced_core_id, serviced_opcode, serviced_access_addr, serviced_init_req_time, m_hit_latency, false);

					m_upper_level_serviced_q->insert(m_upper_level_serviced_q->begin(), this_request);
					// (NOTE!!) For "L2" cache, "m_upper_level_serviced_q" should be "L1's m_serviced_q"
				}
				else	assert(0);
			}	
		}
	}
	#ifdef _DEBUG_
	else
	{
		printf("\n\n[L%d][Serviced Queue EMPTY!!! at cycle=%lld]\n", m_cache_level, cpu_cycle);
	}
	#endif
}

void	Cache::advance_cycle()
{
	// B. (Requests 'SERVICED' from below level cache should be accomodated
	// (Missed requests that are serviced should 'FIRST' be allocated a block HERE and then queried again to mark it as HIT
	advance_one_serviced_request();

	// Handle 'incoming' requests
	// A. (Pick '1' request among ones inserted in "m_in_request_q")
	advance_one_incoming_request();
}

void Cache::print_stats()
{
	printf("[Core-%d][L%d Cache][BankId=%d][Stats]\n", m_core_id, m_cache_level,m_bank_id);
	printf("- Total Accesses	= %d\n", m_num_accesses);
	printf("- Total Hits		= %d\n", m_num_hits);
	printf("- Total Misses		= %d\n", m_num_misses);
	assert(m_num_accesses==(m_num_hits+m_num_misses));
	printf("=> Hit  rate		= %lf\n", ((double)m_num_hits)/((double)m_num_accesses));
	printf("=> Miss rate		= %lf\n", ((double)m_num_misses)/((double)m_num_accesses)); 
}

void Cache::print_cache_block_status()
{
	cout<<"\n\n[L"<<m_cache_level<<"][Cache Line Status]\n";
	printf("[Cache Size 	= %4d-KB]\n", m_size);
	printf("[Block Size 	= %4d-B ]\n", m_block_size);
	printf("[Num of Sets	= %4d   ]\n", m_num_of_sets);
	printf("[Num of Lines	= %4d   ]\n", m_num_of_lines);
	printf("[Block-Offset_Bits = %d ]\n", m_block_offset_bits);

	for(unsigned i=0; i<m_num_of_lines; i++)
	{
		m_tag_array.print_line(i);
	}
	printf("\n\n");
}

void Cache::print_queue_status()
{
	printf("\n\n[L%d][Current Cycle=%lld]\n",m_cache_level, cpu_cycle);
	printf("[L%d]\n", m_cache_level);
	cout<<"[Requested Queue Status] Size = "<<m_in_request_q.size()<<endl;
	printf("[NOTE!! CyclesLeft means NOTHING here]\n");
	vector<mem_request_t>::iterator it;
	for(it=m_in_request_q.begin(); it<m_in_request_q.end(); it++)
	{
		if((*it).m_opcode==STORE)		printf("[Opcode = ST]");
		else{	assert((*it).m_opcode==LOAD);	printf("[Opcode = LD]"); };
		
		printf("[WB=%d][Req_Core=%d][AccessAddr=%8x][CyclesLeft=%d][Requested_Time=%d]\n", (*it).m_is_writeback,(*it).m_core_id, (*it).m_access_addr, (*it).m_cycles_left_for_service, (*it).m_first_request_time);		
	}
	printf("\n");

	cout<<"[Serviced Queue Status] Size = "<<m_serviced_q.size()<<endl;
	for(it=m_serviced_q.begin(); it<m_serviced_q.end(); it++)
	{
		if((*it).m_opcode==STORE)		printf("[Opcode = ST]");
		else{	assert((*it).m_opcode==LOAD);	printf("[Opcode = LD]"); };
		
		printf("[WB=%d][Req_Core=%d][AccessAddr=%8x][CyclesLeft=%d][Requested_Time=%d]\n", (*it).m_is_writeback,(*it).m_core_id, (*it).m_access_addr, (*it).m_cycles_left_for_service, (*it).m_first_request_time);		
	}

	cout<<"\n[Upper-level Serviced Status] Size = "<<m_upper_level_serviced_q->size()<<endl;
	for(it=m_upper_level_serviced_q->begin(); it<m_upper_level_serviced_q->end(); it++)
	{
		if((*it).m_opcode==STORE)		printf("[Opcode = ST]");
		else{	assert((*it).m_opcode==LOAD);	printf("[Opcode = LD]"); };
		
		printf("[WB=%d][Req_Core=%d][AccessAddr=%8x][CyclesLeft=%d][Requested_Time=%d]\n", (*it).m_is_writeback,(*it).m_core_id, (*it).m_access_addr, (*it).m_cycles_left_for_service, (*it).m_first_request_time);		
	}
	printf("\n\n");

}

void Cache::print_cache_info() {
	printf("\n\n==============================================\n");
	cout << left << setw(20) << "Cache: " << m_name << endl;
	cout << setw(20) << "Size: " << m_size << "KB" << endl;
	cout << setw(20) << "Block Size: " <<m_block_size << "B" << endl;
	cout << setw(20) << "Associativity: " << m_associativity << endl;
	cout << setw(20) << "Bank-ID: " << m_bank_id << endl;
	cout << setw(20) << "Hit Latency: " << m_hit_latency << " cycles" << endl;
	cout << setw(20) << "Miss Latency: " <<m_miss_latency << " cycles" << endl;
	printf("\n");

	printf("Num_of_Lines	  = %d\n", m_num_of_lines);
	printf("Num_of_Sets	  = %d\n", m_num_of_sets);
	printf("Block_Offset_Bits = %d\n", m_block_offset_bits);
	printf("Set_Bits	  = %d\n", m_set_bits);
	printf("==============================================\n");
}

tag_array::~tag_array()
{
    delete m_blocks;
}

tag_array::tag_array(int core_id, int num_of_lines, int associativity) 
: m_core_id(core_id), m_associativity(associativity)
{
    m_blocks = new cache_block_t[ num_of_lines ];

	m_num_lines	= num_of_lines;
}

enum cache_access_status tag_array::probe(addr_type set_index, addr_type tag_value, unsigned &block_idx)
{
    	unsigned invalid_block 	= (unsigned)-1;
    	unsigned valid_block	= (unsigned)-1;
	// To find-out oldest entry for LRU replacement
	unsigned oldest_time	= (unsigned)-1;

	for(unsigned way=0; way<m_associativity; way++)
	{
		unsigned index	= (set_index*m_associativity) + way;

		cache_block_t	*this_block	= &m_blocks[index];

		// Tag matches
		if(this_block->m_tag == tag_value)
		{
			// Valid-entry exists
			if( (this_block->m_status==VALID)||(this_block->m_status==DIRTY) )
			{
				block_idx	= index;
				return	HIT;
			}
		}

		if(this_block->m_status==INVALID)
		{
			// This entry is 'blank' and can be used for allocation
			invalid_block	= index;
		}
		else
		{
			#ifdef _SANITY_
			assert( (this_block->m_status==VALID)||(this_block->m_status==DIRTY) );
			#endif
			if(this_block->m_last_access_time < oldest_time)
			{
				oldest_time	= this_block->m_last_access_time;
				valid_block	= index;
			}
		}
	}
	// Designate the 'invalid-block-entry' that should be used for allocation
	if(invalid_block!=(unsigned)-1)
	{
		block_idx	= invalid_block;
		return	MISS_WITH_ENTRY_ALLOCATION;
	}
	// If all entries are full, then pick the LRU-entry for eviction
	else if(valid_block!=(unsigned)-1)
	{
		block_idx	= valid_block;
		return	MISS_WITH_ENTRY_REPLACEMENT;
	}
	else
	{
		printf("Can't happen!\n");
		assert(0);
	}	
}

enum cache_access_status tag_array::allocate_block(unsigned is_write, addr_type access_addr, addr_type set_index, addr_type tag_value, unsigned cycle_time, cache_block_t* evicted_block, bool *dirty_block_evicted)
{
	unsigned	block_idx;
	enum	cache_access_status	access_result	= probe(set_index, tag_value, block_idx);

	switch(access_result)
	{
		case	HIT:
			// A concurrent MISS closer than this MISS has come back earlier (only need to make it DIRTY if this is STORE)
			break;

		case	MISS_WITH_ENTRY_ALLOCATION:
			#ifdef _DEBUG_
			printf("[CacheBlock_Alloc] MISS with ALLOC\n");
			#endif
			#ifdef _SANITY_
			assert( m_blocks[block_idx].m_status==INVALID);
			#endif
			// There's an emptry entry available, so no need to 'write-back' dirty lines	
			// Allocate an entry (BELOW SHOULD HAPPEN when SERVICE is returned from lower level)
			m_blocks[block_idx].allocate(tag_value, access_addr, cycle_time); 
			break;

		case	MISS_WITH_ENTRY_REPLACEMENT:
			#ifdef _DEBUG_
			printf("[CacheBlock_Alloc] MISS with REPLACE\n");
			#endif
			#ifdef _SANITY_
			assert( (m_blocks[block_idx].m_status==VALID)||(m_blocks[block_idx].m_status==DIRTY) );
			#endif
			// If the 'evicted-line' is 'dirty', then 'write-back' needed
			if(m_blocks[block_idx].m_status==DIRTY)
			{
				#ifdef _SANITY_
				assert((*dirty_block_evicted)==false);
				#endif
				(*dirty_block_evicted)	= true;
				(*evicted_block)	= m_blocks[block_idx];
			}
			m_blocks[block_idx].allocate(tag_value, access_addr, cycle_time); 
			break;

		default:
			printf("Invalid access results\n");
			assert(0);
			break;
	}
	return	access_result;
}

void tag_array::mark_dirty_if_needed(unsigned is_write, addr_type access_addr, addr_type set_index, addr_type tag_value, unsigned cycle_time )
{
	unsigned	block_idx;
	enum	cache_access_status	access_result	= probe(set_index, tag_value, block_idx);
	assert(access_result==HIT);
	// If this is a "STORE", then mark it "DIRTY"
	if(is_write==1)
	{
		m_blocks[block_idx].m_status	= DIRTY;
	}
}

enum cache_access_status tag_array::access(unsigned is_write, addr_type access_addr, addr_type set_index, addr_type tag_value, /*cache_block_t &evicted_block, unsigned &block_idx,*/ unsigned cycle_time)
{
	unsigned	block_idx;
	enum	cache_access_status	access_result	= probe(set_index, tag_value, block_idx);

	switch(access_result)
	{
		case	HIT:
			#ifdef _DEBUG_
			printf("Inside: HIT\n");	
			#endif
			#ifdef _SANITY_
			assert( m_blocks[block_idx].m_tag==tag_value);
			assert( (m_blocks[block_idx].m_status==VALID)||(m_blocks[block_idx].m_status==DIRTY) );
			#endif
			// Update 'last-accessed' time
			m_blocks[block_idx].m_last_access_time	= cycle_time;
			
			// If this is a "STORE", then mark it "DIRTY"
			if(is_write==1)
			{
				m_blocks[block_idx].m_status	= DIRTY;
			}
			break;

		case	MISS_WITH_ENTRY_ALLOCATION:
			#ifdef _DEBUG_
			printf("Inside: MISS with ALLOC\n");
			#endif
			#ifdef _SANITY_
			assert( m_blocks[block_idx].m_status==INVALID);
			#endif
			// There's an emptry entry available, so no need to 'write-back' dirty lines	
			// Allocate an entry (BELOW SHOULD HAPPEN when SERVICE is returned from lower level)
			// m_blocks[block_idx].allocate(tag_value, access_addr, cycle_time); 
			break;

		case	MISS_WITH_ENTRY_REPLACEMENT:
			#ifdef _DEBUG_
			printf("Inside: MISS with REPLACE\n");
			#endif
			#ifdef _SANITY_
			assert( (m_blocks[block_idx].m_status==VALID)||(m_blocks[block_idx].m_status==DIRTY) );
			#endif
			break;

		default:
			printf("Invalid access results\n");
			assert(0);
			break;
	}

	return	access_result;
}

void tag_array::print_line(int line_id)
{
	printf("[Line-%4d][Valid = %d][Tag = %8x][BlockAddr = %8x][Last access time = %8d]\n", line_id, m_blocks[line_id].m_status, m_blocks[line_id].m_tag, m_blocks[line_id].m_block_addr, m_blocks[line_id].m_last_access_time);
}

void tag_array::print_all_lines()
{
	for(unsigned i=0; i<m_num_lines; i++)
	{
		print_line(i);
	}
	printf("\n\n");
}

void	Core::advance_cycle()
{
	// If a pending-request exists ...
	if(m_core_serviced_q.size()>0)
	{
		#ifdef _DEBUG_
		printf("\n\n\n[Core-%d][Serviced Queue OCCUPIED!!! at cycle=%lld]\n", m_core_id, cpu_cycle);
		#endif
		vector<mem_request_t>::iterator it;
		for(it=m_core_serviced_q.begin(); it<m_core_serviced_q.end(); it++)
		{
			// Decrement 'CyclesLeft'
			(*it).m_cycles_left_for_service--;
			#ifdef _DEBUG_
			printf("[Core-%d][AFTER][Serviced_Core=%d][AccessAddr=%8x][CyclesLeft=%d][Requested_Time=%d]\n",m_core_id, (*it).m_core_id, (*it).m_access_addr, (*it).m_cycles_left_for_service, (*it).m_first_request_time);	
			#endif

			// When 'CyclesLeft' is ZERO, then 1) allocate that block to 'this cache' and delete it from QUEUE,,, 2) inform 'HIT' to upper-level
			if( (*it).m_cycles_left_for_service==0 )
			{
				#ifdef _DEBUG_
				printf("[TOTALLY_DONE][Addr=%x] is SERVICED COMPLETEDLY!!!!!!! [CurrentTime=%lld][InitialReqTime=%lld]\n", (*it).m_access_addr, cpu_cycle, (*it).m_first_request_time);
				#endif
				// Delete the 'this request' from queue
				m_core_serviced_q.erase(it);

				// Increment number of requests actually serviced
				m_num_serviced++;	
	
				// Accumulate access-latency
				m_access_latencies_accumulated	+= ( cpu_cycle - (*it).m_first_request_time );
			}
		}
	}
	#ifdef _DEBUG_
	else
	{
		printf("\n\n[Core-%d][Serviced Queue EMPTY!!! at cycle=%lld]\n", m_core_id, cpu_cycle);
	}
	#endif

}

void	Core::insert_incoming_request(enum opcode is_write, addr_type access_addr)
{
	// Track total number of accesses inserted to this core
	m_num_accesses++;

	mem_request_t   this_request(m_core_id, is_write, access_addr, cpu_cycle, 1818, false);

	m_lower_level_request_q->insert(m_lower_level_request_q->begin(), this_request);
}

void	Core::print_stats()
{
	printf("\n=====================================\n");
	printf("[Core-%d] Stats\n", m_core_id);
	printf("- Num of access 	= %d\n", m_num_accesses);
	printf("- Num of serviced	= %lld\n", m_num_serviced); 
	printf("-------------------------------------\n");
	printf("- Total latencies accumulated	 = %lld\n", m_access_latencies_accumulated);
	printf("- (AMAT) Average Mem Access Time = %lf\n", ((double)m_access_latencies_accumulated)/((double)m_num_serviced));
	printf("=====================================\n\n");
}



