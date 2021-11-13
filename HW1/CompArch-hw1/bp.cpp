/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

// Libraries
#include "bp_api.h"
#include <vector>
#include <math.h>

// Typedefs
using std::vector;


#define SNT 0 // 0b00
#define WNT 1 // 0b01
#define WT 2  // 0b10
#define ST 3  // 0b11
typedef struct{
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
	bool is_shared;

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
};

BranchPredictor::BranchPredictor(){

}

BranchPredictor::BranchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int shared):
				BTB_size(btbSize), hist_size(historySize), tag_size(tagSize), def_fsm(fsmState), is_global_history(isGlobalHist), is_global_machine(isGlobalTable), is_shared(shared) {
		

	Entry def_entry;
	def_entry.tag = 0;
	def_entry.target = 0;
	def_entry.history = 0;

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
	unsigned n = number;
	n = n >> start;
	unsigned mask = 1 << N_bits;
	mask -= 1; // 0001000 => 0000111
	return n & mask;
}

uint32_t BranchPredictor::predict(uint32_t pc){
	int index_N_bits = log2(this->BTB_size);
	unsigned index = this->get_bits(pc, 2, index_N_bits);
	Entry entry = this->BTB[index];

	unsigned hist_index;
	if (this->is_global_history){
		hist_index = global_history;
	}
	else {
		hist_index = entry.history;
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

	//Getting tag
	unsigned tag = this->get_bits(pc, 2 + index_N_bits, this->tag_size);
	if (tag != entry.tag) {
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

	entry.target = targetPc;

	if (targetPc != pred_dst)
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

}

SIM_stats BranchPredictor::get_stats() {
	SIM_stats stats;
	stats.br_num = this->num_branches;
	stats.flush_num = this->num_flushes;
	stats.size = sizeof(this); //FIXME: Check if calculate theoretical or sizeof
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
	return *dst != (pc+4);

}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	branch_predictor.update(pc, targetPc, taken, pred_dst);
}

void BP_GetStats(SIM_stats *curStats){
	*curStats = branch_predictor.get_stats();
}