/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

// Libraries
#include "bp_api.h"
#include <vector>
#include <math.h>
#include <stdio.h>

// Typedefs
using std::vector;


#define SNT 0 // 0b00
#define WNT 1 // 0b01
#define WT 2  // 0b10
#define ST 3  // 0b11

#define NO_SHARE 0
#define SHARE_USING_LSB 1
#define SHARE_USING_MID 2

#define TARGET_ADDR_SIZE 30
typedef struct{
	bool valid;
	unsigned tag;
	unsigned target;
	unsigned history;
	vector<int> state_machine;
} Entry;

class BranchPredictor
{
private:
	//Sizes
	unsigned BTB_size;
	unsigned hist_size;
	unsigned tag_size;

	//BTB
	vector<Entry> BTB;
	vector<int> global_machines;
	unsigned global_history;

	//Booleans
	bool is_global_machine;
	bool is_global_history;
	int is_shared;

    //Defaults
    unsigned def_fsm;

	//Stat Counters
	int num_branches;
	int num_flushes;

public:
	BranchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int is_shared);
	BranchPredictor();
	uint32_t predict(uint32_t pc);
	unsigned get_bits(unsigned number, int start, int end);
	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	SIM_stats get_stats();
	int get_valid_entries();
};

BranchPredictor::BranchPredictor(){

}

BranchPredictor::BranchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int shared):
				BTB_size(btbSize), hist_size(historySize), tag_size(tagSize), def_fsm(fsmState), is_global_history(isGlobalHist), is_global_machine(isGlobalTable), is_shared(shared) {
		

	Entry def_entry;
	def_entry.tag = 0;
	def_entry.target = 0;
	def_entry.history = 0;
	def_entry.valid = false;

	int state_size = pow(2, this->hist_size);

	if (this->is_global_machine) {
		def_entry.state_machine = vector<int>();
		this->global_machines = vector<int>(state_size, this->def_fsm);
	}
	else {
		def_entry.state_machine = vector<int>(state_size, this->def_fsm);
	}

	if (this->is_global_history) {
		this->global_history = 0;
	}


	this->BTB = vector<Entry>(this->BTB_size, def_entry);

	this->num_branches = 0;
	this->num_flushes = 0;
	
}


unsigned BranchPredictor::get_bits(unsigned number, int start, int N_bits){
	unsigned n = number; // n = ZZZZZ YYYY 00
	n = n >> start; //    	ZZZZZ YYYY 00 => ZZZZZ YYYY
	unsigned mask = 1 << N_bits; // mask = 00001 0000
	mask -= 1; // 0001000 => 0000111
	return n & mask;
}

uint32_t BranchPredictor::predict(uint32_t pc){
	int index_N_bits = log2(this->BTB_size);
	unsigned index = this->get_bits(pc, 2, index_N_bits);
	Entry entry = this->BTB[index];

	unsigned tag = this->get_bits(pc, 2 + index_N_bits, this->tag_size);

	if (tag != entry.tag){
		return pc+4;
	}

	unsigned hist_index;
	if (this->is_global_history){
		hist_index = global_history;
	}
	else {
		hist_index = entry.history;
	}

	// LShare or GShare Handling
	if (this->is_shared != NO_SHARE){
		int start;
		if (this->is_shared == SHARE_USING_LSB)
			start = 2;
		else if (this->is_shared == SHARE_USING_MID)
			start = 16;
		unsigned BIP = this->get_bits(pc, start, this->hist_size);
		hist_index = hist_index ^ BIP;
	}

	int state;
	if (this->is_global_machine){
		state = global_machines[hist_index];
	}
	else {
		state = entry.state_machine[hist_index];
	}

	if (state == SNT || state == WNT)
		return pc+4;
	else
		return entry.target;

}

void BranchPredictor::update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	this->num_branches++;
	int index_N_bits = log2(this->BTB_size);
	unsigned index = this->get_bits(pc, 2, index_N_bits);
	Entry entry = this->BTB[index];

	if (!entry.valid)
		entry.valid = true;

	//Getting tag
	unsigned tag = this->get_bits(pc, 2 + index_N_bits, this->tag_size);
	//Replacing existing branch inside BTB or new tag
	if (!entry.valid || tag != entry.tag) {
		entry.tag = tag;
		if (!this->is_global_history)
			entry.history = 0;
		if (!this->is_global_machine)
			entry.state_machine = vector<int>(pow(2, this->hist_size), this->def_fsm);
	}

	unsigned hist_index;
	if (this->is_global_history){

		hist_index = this->global_history;
	}
	else {
		hist_index = entry.history;
	}
	
	// LShare or GShare Handling
	if (this->is_shared != NO_SHARE){
		int start;
		if (this->is_shared == SHARE_USING_LSB)
			start = 2;
		else if (this->is_shared == SHARE_USING_MID)
			start = 16;
		unsigned BIP = this->get_bits(pc, start, this->hist_size);
		hist_index = hist_index ^ BIP;
	}

	entry.target = targetPc;

	if ((taken && (targetPc != pred_dst)) || (!taken && (pred_dst != pc+4)))
		this->num_flushes++;

	int direction = taken ? 1 : -1;

	if (this->is_global_machine){
		this->global_machines[hist_index] += direction;
		if (this->global_machines[hist_index] == 5 || this->global_machines[hist_index] == -1)
			this->global_machines[hist_index] -= direction;
	}
	else {
		entry.state_machine[hist_index] += direction;
		if (entry.state_machine[hist_index] == 5 || entry.state_machine[hist_index] == -1)
			entry.state_machine[hist_index] -= direction;
	}


	if (this->is_global_history) {
		this->global_history = this->global_history << 1;
		if (taken)
			this->global_history++;
		
		this->global_history = this->global_history & ((1 << this->hist_size) - 1);
	}
	else {
		entry.history = entry.history << 1;
		if (taken)
			entry.history++;
		
		entry.history = entry.history & ((1 << this->hist_size) - 1);
	}

	this->BTB[index] = entry;

}

int BranchPredictor::get_valid_entries() {
	int counter = 0;
	for (vector<Entry>::iterator it = this->BTB.begin(); it != this->BTB.end(); it++) {
		if (it->valid)
			counter++;
	}
	return counter;
}

SIM_stats BranchPredictor::get_stats() {
	SIM_stats stats;
	stats.br_num = this->num_branches;
	stats.flush_num = this->num_flushes;
	int N_BHR = this->get_valid_entries();
	if (this->is_global_history){
		if (this->is_global_machine){
			stats.size = this->BTB_size * (this->tag_size + 1 + TARGET_ADDR_SIZE) + this->hist_size + pow(2, this->hist_size + 1);
		}
		else{
			stats.size = this->BTB_size * (this->tag_size + 1 + TARGET_ADDR_SIZE + pow(2, this->hist_size+1)) + this->hist_size;
		}
	}
	else {
		if (this->is_global_machine){
			stats.size = this->BTB_size * (this->tag_size + 1 + TARGET_ADDR_SIZE + this->hist_size) + pow(2, this->hist_size + 1);
		}
		else {
			stats.size = this->BTB_size * (this->tag_size + 1 + TARGET_ADDR_SIZE + this->hist_size + pow(2, this->hist_size + 1));
		}
	}
	return stats;
}

/*
	START OF API IMPLEMENTATION
*/
BranchPredictor branch_predictor;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	
	try {
		branch_predictor = BranchPredictor(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
		return 0;
	}
	catch (std::bad_alloc const&) {
		return -23;
	}
}

bool BP_predict(uint32_t pc, uint32_t *dst){

	*dst = branch_predictor.predict(pc);
	// printf("%#x\n", *dst);
	return *dst != (pc+4);

}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	branch_predictor.update(pc, targetPc, taken, pred_dst);
}

void BP_GetStats(SIM_stats *curStats){
	*curStats = branch_predictor.get_stats();
}