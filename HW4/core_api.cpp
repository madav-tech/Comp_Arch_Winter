/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>

#include <stdio.h>


using std::vector;

class Thread {
  public:
	Instruction current_inst;
	uint32_t inst_address;
	int thread_id;
	tcontext reg_file;
	bool is_halted;
	int start_cycle;

	Thread(int id);

};
	Thread::Thread(int id): inst_address(0), thread_id(id), is_halted(false), start_cycle(0) {
		this->reg_file = tcontext();
		for (int i=0; i<REGS_COUNT; i++){
			this->reg_file.reg[i] = 0;
		}
	}

class Thread_Handler {
  public:
	int current_cycle;
	int N_instructions; // For CPI (/IPC) calculations
	bool is_blocked_MT; // Blocked or Finegrained
	vector<Thread> threads;
	int current_thread_id;
	Thread* current_thread;
	
	int load_lat;
	int store_lat;
	int switch_penalty;
	int N_threads;

	Thread_Handler(bool is_blocked_MT, int load_lat, int store_lat, int switch_pen, int N_threads);
	bool thread_free();
	void switch_thread();
	bool all_halted();
	void run_program();
	void run_program_blocked();
	void run_program_finegrained();
};

Thread_Handler::Thread_Handler(bool is_blocked_MT, int load_lat, int store_lat, int switch_pen, int N_threads): current_cycle(0), N_instructions(0), is_blocked_MT(is_blocked_MT),
				current_thread_id(0), load_lat(load_lat), store_lat(store_lat), switch_penalty(switch_pen), N_threads(N_threads){
	this->threads = vector<Thread>();
	for (int i=0; i<this->N_threads; i++){
		Thread th = Thread(i);
		this->threads.push_back(th);
	}
	this->current_thread = &(threads[0]);
}

bool Thread_Handler::thread_free(){
	Instruction* thread_inst = &(this->current_thread->current_inst);
	cmd_opcode opcode = thread_inst->opcode;
	int latency = 1;
	switch (opcode)
	{
	case CMD_LOAD:
		latency = this->load_lat;
		break;
	case CMD_STORE:
		latency = this->store_lat;
		break;
	}
	return (this->current_cycle - current_thread->start_cycle >= latency);
}
void Thread_Handler::switch_thread(){
	//Loop through threads until one is ready (if none are ready, stay in current thread and stall one cycle)
}

bool Thread_Handler::all_halted(){
	bool all_halted = true;
	for (vector<Thread>::iterator it=this->threads.begin(); it != this->threads.end(); it++)
		if (!it->is_halted)
			all_halted = false;

	return all_halted;
}

void Thread_Handler::run_program(){
	if (this->is_blocked_MT)
		this->run_program_blocked();
	else
		this->run_program_finegrained();
}
void Thread_Handler::run_program_finegrained(){
	while (!this->all_halted())
	{
		this->current_cycle++;
		this->current_thread->execute_inst(this->current_cycle);
		this->switch_thread();
		// Run program
		// Switch threads (run through threads until one is ready)
	}
	
}




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
