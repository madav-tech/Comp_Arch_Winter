#ifndef BP_HPP
#define BP_HPP

// Libraries
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

class bp
{
private:
	//Sizes
	unsigned BTB_size;
	unsigned hist_size;
	unsigned tag_size;

	//BTB
	vector<Entry> BTB;
	vector<int> global_fsm;
	unsigned global_history_reg;

	//Booleans
	bool global_machines;
	bool global_history;
	bool shared;

    //Defaults
    unsigned def_fsm;

public:
	bp(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int shared);
};

bp::bp(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int shared):
		BTB_size(btbSize), hist_size(historySize), tag_size(tagSize), def_fsm(fsmState), global_history(isGlobalHist), global_machines(isGlobalTable), shared(shared) {
		

	Entry def_entry;
	def_entry.tag = -1;
	def_entry.target = -1;
	def_entry.history = -1;

	int state_size = pow(2, this->hist_size);

	if (this->global_machines) {
		def_entry.state_machine = vector<int>();
		this->global_fsm = vector<int>(state_size, this->def_fsm);
	}
	else {
		def_entry.state_machine = vector<int>(state_size, this->def_fsm);
	}

	if (this->global_history) {
		this->global_history_reg = 0;
	}


	this->BTB = vector<Entry>(this->BTB_size, def_entry);
	
}


#endif

	/*
 * BP_init - initialize the predictor
 * all input parameters are set (by the main) as declared in the trace file
 * return 0 on success, otherwise (init failure) return <0
 */
	int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
				bool isGlobalHist, bool isGlobalTable, int Shared);