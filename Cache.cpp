#include <iostream>
#include <iomanip>
#include "Cache.h"

using namespace std;
void Cache::hello() {
	cout << "hello" << endl;
}
void Cache::Print() {
	cout << left << setw(20) << "Cache: " << name << endl;
	cout << setw(20) << "Size: " << size << "KB" << endl;
	cout << setw(20) << "Block Size: " <<block_size << "B" << endl;
	cout << setw(20) << "Associativity: " << associativity << endl;
	cout << setw(20) << "Number of Banks: " << num_banks << endl;
	cout << setw(20) << "Hit Latency: " << hit_latency << " cycles" << endl;
	cout << setw(20) << "Miss Latency: " <<miss_latency << " cycles" << endl;
}
