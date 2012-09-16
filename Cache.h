#ifndef CACHE_H
#define CACHE_H
#include <string>
#include <assert.h>
#include <cmath>
#include <vector>
#include "misc.h"

using namespace std;

typedef unsigned long long addr_type;

enum	cache_access_status
{
	HIT,
	MISS_WITH_ENTRY_REPLACEMENT,
	MISS_WITH_ENTRY_ALLOCATION
};

enum 	cache_block_state 
{
    	INVALID,
    	VALID,
    	DIRTY
};

enum	opcode
{
	LOAD = 0,
	STORE
};

struct mem_request_t{
	mem_request_t(int core_id, enum opcode is_write, addr_type access_addr, unsigned request_time, unsigned cache_service_latency, bool is_writeback)
	{

		m_core_id	= core_id;
		m_opcode	= is_write;
		m_access_addr	= access_addr;
		m_first_request_time = request_time;
		m_cycles_left_for_service = cache_service_latency;
		m_is_writeback	= is_writeback;
	}

	int		m_core_id;
	enum opcode	m_opcode;
	addr_type	m_access_addr;
	
	unsigned long long	m_first_request_time;
	unsigned long long	m_cycles_left_for_service;	

	bool		m_is_writeback;
};

struct cache_block_t {
    cache_block_t()
    {
        m_tag=0;
        m_block_addr=0;
        m_alloc_time=0;
        m_fill_time=0;
        m_last_access_time=0;
        m_status=INVALID;
    }
    void allocate( addr_type tag, addr_type block_addr, unsigned cycle_time )
    {
        m_tag		= tag;
        m_block_addr	= block_addr;
        m_alloc_time	= cycle_time;
        m_last_access_time = cycle_time;
        m_fill_time	= 0;
        m_status	= VALID;
    }
    void fill( unsigned cycle_time )
    {
        m_status	= VALID;
        m_fill_time	= cycle_time;
    }

    addr_type		m_tag;
    addr_type   	m_block_addr;
    unsigned    	m_alloc_time;
    unsigned    	m_last_access_time;
    unsigned    	m_fill_time;
    cache_block_state   m_status;
};

class tag_array {
public:
	tag_array(int core_id, int num_of_lines, int associativity);
    	~tag_array();

	// Cache-block access
	enum cache_access_status 	access( unsigned is_write, addr_type access_addr, addr_type set_index, addr_type tag_value, /*cache_block_t &evicted_block, unsigned &block_idx,*/ unsigned cycle_time ); 
    	enum cache_access_status 	probe( addr_type set_index, addr_type tag_value, unsigned &block_idx );

	enum cache_access_status 	allocate_block( unsigned is_write, addr_type access_addr, addr_type set_index, addr_type tag_value, /*cache_block_t &evicted_block, unsigned &block_idx,*/ unsigned cycle_time, cache_block_t* evicted_block, bool *dirty_block_evicted ); 
	void	mark_dirty_if_needed(unsigned is_write, addr_type access_addr, addr_type set_index, addr_type tag_value, unsigned cycle_time ); 


	void print_line(int line_id);
	
	void print_all_lines();
protected:

	// Members
    	cache_block_t *m_blocks; /* nbanks x nset x assoc lines in total */
    	int 	m_core_id; 
	int	m_associativity;
	int	m_num_lines;
};

class Core {
	public:
		Core(int core_id){ 	m_core_id = core_id;	}
		void			advance_cycle();
		int			get_core_id()		{ return m_core_id; }
		vector<mem_request_t>* 	get_serviced_q()	{ return &m_core_serviced_q; }

	private:
		int m_core_id;
		vector<mem_request_t> m_core_serviced_q;
};

class Cache {
	public:
		Cache(int core_id, int cache_level, int num_banks, int cache_size, int block_size, int assoc, int hit_latency, int miss_latency, string name); //, vector<mem_request_t> *upper_level_serviced_q, vector<mem_request_t> *lower_level_req_q);

		// 'Cycle-level'
		void				advance_cycle();
		void				advance_one_incoming_request();
		void				advance_one_serviced_request();
		// Insert-to-queue
		void				insert_incoming_request(int req_core_id, enum opcode is_write, addr_type access_addr);


		// Servicing requests
		enum cache_access_status 	access(int req_core_id, unsigned is_write, addr_type access_addr, unsigned cycle_time);


		addr_type	get_block_addr(addr_type this_addr)	{	return 	(this_addr >> m_block_offset_bits);	}
		addr_type	get_set_index(addr_type	this_addr)	{	return	((this_addr >> m_block_offset_bits) & (m_num_of_sets-1));	}
		addr_type	get_tag_value(addr_type	this_addr)	{	return	(this_addr >> (m_block_offset_bits+m_set_bits));		}

		// Access methods
		void	set_upper_level_serviced_q(vector<mem_request_t> *upper_level_serviced_q)
		{
			m_upper_level_serviced_q	= upper_level_serviced_q;			
		}
		void	set_lower_level_request_q(vector<mem_request_t> *lower_level_request_q)
		{
			m_lower_level_request_q		= lower_level_request_q;
		}
		vector<mem_request_t>* get_incoming_request_q()	{ return &m_in_request_q; }
		vector<mem_request_t>* get_serviced_q()		{ return &m_serviced_q; }

		// Printing
		void print_stats();
		void print_cache_info();
		void print_queue_status();
		void print_cache_block_status();
	private:
		// Configurable options
		int	m_core_id;
		int 	m_size; //cache size 16KB for l1 and 1MB for l2
		int 	m_num_banks; // l1:1, l2: 16
		int 	m_block_size; //32B for l1 and 128B for l2
		int 	m_associativity; // l1: 2; l2: 2
		int 	m_hit_latency; // l1: 5; l2:20
		int 	m_miss_latency; // l1: 200; l2: 200

		int	m_cache_level;
		string 	m_name;

		// Detailed info derived from configurable-options
		int	m_num_of_lines;
		int	m_num_of_sets;
		int	m_block_offset_bits;
		int	m_set_bits;
		tag_array	m_tag_array;

		// "Requested" accesses 'to' this cache
		vector<mem_request_t> m_in_request_q;
		// "To-request" accesses 'from' this cache
		//vector<mem_request_t> m_out_request_q;
		// "Returning" requests from lower-level mem-hierarchy
		vector<mem_request_t> m_serviced_q;

		// This is where a 'HIT' request will be reported back to
		vector<mem_request_t>	*m_upper_level_serviced_q;

		// This is where a 'MISS' request will be requested to
		vector<mem_request_t>	*m_lower_level_request_q;

		// Stats		
		unsigned int	m_num_accesses;
		unsigned int	m_num_hits;
		unsigned int	m_num_misses;
};




#endif
