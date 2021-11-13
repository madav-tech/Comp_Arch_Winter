#ifndef BP_HPP
#define BP_HPP

// Libraries
#include <vector>

// Typedefs
using std::vector;


#define SNT 0 // 0b00
#define WNT 1 // 0b01
#define WT 2  // 0b10
#define ST 3  // 0b11

typedef struct{
	int tag;
	int target;
	int history;
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

	//Booleans
	bool global_machines;
	bool global_history;
	bool share;

    //Defaults
    unsigned def_fsm;

public:
	bp(/* unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int shared */);
	~bp();
};

bp::bp(/* args */)
{
}

bp::~bp()
{
}

#endif

	/*
 * BP_init - initialize the predictor
 * all input parameters are set (by the main) as declared in the trace file
 * return 0 on success, otherwise (init failure) return <0
 */
	int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
				bool isGlobalHist, bool isGlobalTable, int Shared);