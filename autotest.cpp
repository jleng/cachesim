#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <vector>
#include "Cache.h"
#include "misc.h"

using namespace std;

int main(int argc, char* argv[]) {

	if (argc != 2) {
		cout << "Usage: " << argv[0] << " array_size" << std::endl;
		exit(4);
	}

	addr_type array_size;
	istringstream(argv[1]) >> array_size;
	addr_type stride;
	//istringstream(argv[1]) >> stride;
	cout << "array size = " << array_size << "\n";
	//cout << "stride      = " << stride << "\n";
	

	addr_type cycle;
	addr_type cycle_step=50;
	assert(array_size % 1024 == 0);
	//assert(array_size % stride == 0);

	for (int i=0; pow(2,i) < array_size; i++) {
		stride = pow(2,i);
		cout << "stride      = " << stride << "\n";
		ofstream out;
		stringstream out_name;
		out_name << "traces/array" << (array_size/1024) << "KB" << "_stride" << i << ".trace";
		out.open(out_name.str().c_str()); 
		//out.open("test.txt"); 
		//cout <<  out_name.str()<< "\n";
		cycle = cycle_step;
		for (addr_type addr=0; addr<array_size; addr+=stride) {
			out << "0x" << hex << addr << " ";
			out << dec << cycle << " ";
			out << "LOAD" << endl;
			cycle += cycle_step;
		}
		out.close();
	}

	
}

