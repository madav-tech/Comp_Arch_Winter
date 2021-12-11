/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Cache.cpp"

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;


double handle_line(Cache& L1, Cache& L2, char op, unsigned long int addr, unsigned MemCyc, bool WrAlloc){
	
	unsigned long int removed_addr = 0;
	bool removed = false;

	if (op == 'r'){
		bool L1_hit = L1.seek(addr, op, false);
		//L1 Hit
		if (L1_hit)
		{
			return L1.access_time;
		}
		//L1 Miss
		bool L2_hit = L2.seek(addr, op, false);
		//L2 Hit
		if (L2_hit){

			bool L2_was_dirty = L2.is_dirty(addr);
			bool L1_dirty = L1.add_address(addr, &removed_addr, &removed);

			if (L2_was_dirty){
				L1.mark_dirty(addr, true);
				L2.mark_dirty(addr, false);
			}

			if (L1_dirty){
				L2.seek(removed_addr, 'w', true);
			}
			return L1.access_time + L2.access_time;
		}
		//L2 Miss
		bool L2_dirty = L2.add_address(addr, &removed_addr, &removed); // L2_dirty unused because this program doesn't handle the memory side. CHECK: DO WE NEED TO ADD T_mem HERE?
		if (removed){
			L1.remove(removed_addr);
		}
		bool L1_dirty = L1.add_address(addr, &removed_addr, &removed);
		if (L1_dirty){
			L2.seek(removed_addr, 'w', true);
		}
		return L1.access_time + L2.access_time + MemCyc;
	}
	else { //if (op == 'w')
		bool L1_hit = L1.seek(addr, op, false);
		//L1 Hit
		if (L1_hit)
		{
			return L1.access_time;
		}

		//L1 Miss
		bool L2_hit = L2.seek(addr, op, false);
		//L2 Hit
		if (L2_hit){
			if (WrAlloc){
				bool L1_dirty = L1.add_address(addr, &removed_addr, &removed);
				L1.mark_dirty(addr, true);
				L2.mark_dirty(addr, false);
				if (L1_dirty){
					L2.seek(removed_addr, 'w', true);
				}
			}
			return L1.access_time + L2.access_time;
		}
		//L2 Miss
		if (WrAlloc){
			bool L2_dirty = L2.add_address(addr, &removed_addr, &removed); // L2_dirty unused because this program doesn't handle the memory side. CHECK: DO WE NEED TO ADD T_mem HERE?
			if (removed){
				L1.remove(removed_addr);
			}
			bool L1_dirty = L1.add_address(addr, &removed_addr, &removed);
			L1.mark_dirty(addr, true);
			if (L1_dirty){
				L2.seek(removed_addr, 'w', true);
			}
		}
		return L1.access_time + L2.access_time + MemCyc;
	}
}

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}


	Cache L1(L1Size, L1Assoc, BSize, L1Cyc, WrAlloc);
	Cache L2(L2Size, L2Assoc, BSize, L2Cyc, WrAlloc);
	double L1MissRate = 0;
	double L2MissRate = 0;
	double avgAccTime = 0;

	int counter = 0;
	double sum_cycles = 0;

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		// cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		// cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		// cout << " (dec) " << num << endl;

		unsigned long int addr = num;
		
		counter++;
		double current_time = handle_line(L1, L2, operation, addr, MemCyc, WrAlloc);
		sum_cycles += current_time;
	}

	avgAccTime = sum_cycles / counter;
	L1MissRate = L1.N_misses / (L1.N_hits + L1.N_misses);
	L2MissRate = L2.N_misses / (L2.N_hits + L2.N_misses);

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
