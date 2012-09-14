#ifndef CACHE_H
#define CACHE_H
#include <string>
#include <assert.h>
#include <cmath>
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
	enum cache_access_status 	access( addr_type set_index, addr_type tag_value, /*cache_block_t &evicted_block, unsigned &block_idx,*/ unsigned cycle_time ); 
    	enum cache_access_status 	probe( addr_type set_index, addr_type tag_value, unsigned &block_idx );

	void print_line(int line_id);
protected:

	// Members
    	cache_block_t *m_blocks; /* nbanks x nset x assoc lines in total */
    	int 	m_core_id; 
	int	m_associativity;
};

class Cache {
	public:
		Cache(int core_id, int num_banks, int cache_size, int block_size, int assoc, int hit_latency, int miss_latency, string name): m_tag_array(core_id, (cache_size/block_size), assoc), m_num_banks(num_banks), m_size(cache_size),m_block_size(block_size), m_associativity(assoc), m_hit_latency(hit_latency), m_miss_latency(miss_latency), m_name(name)
		{
			// Get num of lines in cache
			m_num_of_lines	= (cache_size/block_size);

			// Derive info to configure set-associativity
			m_num_of_sets		= m_num_of_lines / assoc;
			m_block_offset_bits	= log2(block_size);
			m_set_bits		= log2(m_num_of_sets);

			#ifdef _SANITY_
			assert((m_num_of_lines%m_associativity)==0);
			assert((cache_size%block_size)==0);
			assert((cache_size%1024)==0);
			assert(block_size == pow(2.0, m_block_offset_bits));
			assert(m_num_of_sets == pow(2.0, m_set_bits));
			assert( (m_num_of_sets*m_associativity*m_block_size)==m_size );
			#endif			
		}
		addr_type	get_block_addr(addr_type this_addr)	{	return 	(this_addr >> m_block_offset_bits);	}
		addr_type	get_set_index(addr_type	this_addr)	{	return	((this_addr >> m_block_offset_bits) & (m_num_of_sets-1));	}
		addr_type	get_tag_value(addr_type	this_addr)	{	return	(this_addr >> (m_block_offset_bits+m_set_bits));		}
	
		enum cache_access_status access(addr_type access_addr, unsigned cycle_time)
		{
			addr_type	set_idx 	= get_set_index(access_addr);
			addr_type	tag_value	= get_tag_value(access_addr);
			printf("SetIdx=%x Tag=%x\n", set_idx, tag_value);
			return m_tag_array.access( set_idx, tag_value, cycle_time);
		}	
		void Print();
	private:
		// Configurable options
		int 	m_size; //cache size 16KB for l1 and 1MB for l2
		int 	m_num_banks; // l1:1, l2: 16
		int 	m_block_size; //32B for l1 and 128B for l2
		int 	m_associativity; // l1: 2; l2: 2
		int 	m_hit_latency; // l1: 5; l2:20
		int 	m_miss_latency; // l1: 200; l2: 200
		string 	m_name;

		// Detailed info derived from configurable-options
		int	m_num_of_lines;
		int	m_num_of_sets;
		int	m_block_offset_bits;
		int	m_set_bits;
		tag_array	m_tag_array;
};




#endif
