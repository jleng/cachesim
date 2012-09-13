#ifndef CACHE_H
#define CACHE_H
#include <string>

using namespace std;

class Cache {
	public:
		void hello();
		void SetCacheSize(int sz) {size = sz; return;}
		void SetCacheBlockSize(int sz) {block_size=sz; return;}
		void SetCacheNumBanks(int sz) {num_banks=sz; return;}
		void SetCacheAssociativity(int sz) {associativity=sz; return;}
		void SetCacheHitLatency(int sz) {hit_latency=sz; return;}
		void SetCacheMissLatency(int sz) {miss_latency=sz; return;}
		void SetCacheName(string nm) {name=nm; return;}
		void Print();
	private:
		int size; //cache size 16KB for l1 and 1MB for l2
		int num_banks; // l1:1, l2: 16
		int block_size; //32B for l1 and 128B for l2
		int associativity; // l1: 2; l2: 2
		int hit_latency; // l1: 5; l2:20
		int miss_latency; // l1: 200; l2: 200
		string name;
};

#endif
