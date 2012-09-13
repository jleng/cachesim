#include <iostream>
#include "Cache.h"

using namespace std;

//parameters
int l1_dcache_size=16; // in KB
int l1_dcache_block_size=32; //in Byte
int l1_dcache_associativity=2;
int l1_dcache_hit_latency=5; //cycles;
int l1_dcache_miss_latency=200; //cycles;
int l1_dcache_num_banks=1; //cycles;

//main
int main() {
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
	l1.Print();
}
