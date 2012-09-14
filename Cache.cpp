#include <stdio.h>
#include <iostream>
#include <iomanip>
#include "Cache.h"

//
using namespace std;

void Cache::Print() {
	cout << left << setw(20) << "Cache: " << m_name << endl;
	cout << setw(20) << "Size: " << m_size << "KB" << endl;
	cout << setw(20) << "Block Size: " <<m_block_size << "B" << endl;
	cout << setw(20) << "Associativity: " << m_associativity << endl;
	cout << setw(20) << "Number of Banks: " << m_num_banks << endl;
	cout << setw(20) << "Hit Latency: " << m_hit_latency << " cycles" << endl;
	cout << setw(20) << "Miss Latency: " <<m_miss_latency << " cycles" << endl;

	cout<<"[Cache Line Status]\n";
	printf("[Cache Size 	= %4d-KB]\n", m_size);
	printf("[Block Size 	= %4d-B ]\n", m_block_size);
	printf("[Num of Sets	= %4d   ]\n", m_num_of_sets);
	printf("[Num of Lines	= %4d   ]\n", m_num_of_lines);
	printf("[Block-Offset_Bits = %d ]\n", m_block_offset_bits);


	for(unsigned i=0; i<m_num_of_lines; i++)
	{
		m_tag_array.print_line(i);
	}

}

tag_array::~tag_array()
{
    delete m_blocks;
}

tag_array::tag_array(int core_id, int num_of_lines, int associativity) 
: m_core_id(core_id), m_associativity(associativity)
{
    m_blocks = new cache_block_t[ num_of_lines ];
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

enum cache_access_status tag_array::access(addr_type set_index, addr_type tag_value, /*cache_block_t &evicted_block, unsigned &block_idx,*/ unsigned cycle_time)
{
	unsigned	block_idx;
	enum	cache_access_status	access_result	= probe(set_index, tag_value, block_idx);

	switch(access_result)
	{
		case	HIT:
			printf("Inside: HIT\n");
			#ifdef _SANITY_
			assert( m_blocks[block_idx].m_tag==tag_value);
			#endif
			// Update 'last-accessed' time
			m_blocks[block_idx].m_last_access_time	= cycle_time;
			break;

		case	MISS_WITH_ENTRY_ALLOCATION:
			printf("Inside: MISS with ALLOC\n");
			#ifdef _SANITY_
			assert( m_blocks[block_idx].m_status==INVALID);
			#endif
			// There's an emptry entry available, so no need to 'write-back' dirty lines	
			// Allocate an entry
			m_blocks[block_idx].allocate(tag_value, set_index, cycle_time); 
			break;

		case	MISS_WITH_ENTRY_REPLACEMENT:
			printf("Inside: MISS with REPLACE\n");
			#ifdef _SANITY_
			assert( (m_blocks[block_idx].m_status==VALID)||(m_blocks[block_idx].m_status==DIRTY) );
			#endif
			// If the 'evicted-line' is 'dirty', then 'write-back' needed
			if(m_blocks[block_idx].m_status==DIRTY)
			{
				// Evict!!! (TODO)
			}
			else
			{
				m_blocks[block_idx].allocate(tag_value, set_index, cycle_time);
			}
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
	printf("[Line-%4d][Valid = %d][Tag = %8x][Last access time = %8x]\n", line_id, m_blocks[line_id].m_status, m_blocks[line_id].m_tag, m_blocks[line_id].m_last_access_time);
}
