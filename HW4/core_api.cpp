/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>


/*
Handler:
	int cycle (Current Cycle)
	int N_instructions (Number of executed instructions)
	bool is_blocked_MT
	vector<Thread>
	int current_thread_id
	Thread* current_thread 

	(From sim_api)
	int load_lat; 
	int store_lat;
	int switch_penalty;
	int N_threads;

	switch_threads (function) (Advances Thread pointer to next thread)

Thread:
	Instruction* current_inst
	int inst_address (each instruction calls SIM_MemInstRead)
	thread_ID (int)
	tcontext (Register File)
	bool is_halted (is finished)
	int start_cycle (Time since instruction start = Handler.cycle - this.start_cycle)

	perform_next_instruction (has to return opcode)

*/

void CORE_BlockedMT() {
	
}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
