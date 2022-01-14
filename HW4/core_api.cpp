/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>

#include <stdio.h>

#define NO_OPCODE -1

using std::vector;

class Thread {
  public:
	Instruction current_inst;
	uint32_t inst_address;
	int thread_id;
	tcontext reg_file;
	bool is_halted;
	int start_cycle; // The cycle in which the current thread STARTED running the current instruction

	Thread(int id);
	bool execute_inst(int start_cycle);
	void execute();

};
	Thread::Thread(int id): inst_address(0), thread_id(id), is_halted(false), start_cycle(0) {
		current_inst.opcode = CMD_NOP; // For access to latency before setting instruction
		this->reg_file = tcontext();
		// Initializing registers
		for (int i=0; i<REGS_COUNT; i++){
			this->reg_file.reg[i] = 0;
		}
	}

	bool Thread::execute_inst(int start_cycle) {
		this->start_cycle = start_cycle + 1; // Set the instruction start cycle
		SIM_MemInstRead(this->inst_address, &current_inst, this->thread_id); // Read next instruction from memory
		this->execute();

		this->inst_address++;
		bool event = (this->current_inst.opcode == CMD_LOAD) || (this->current_inst.opcode == CMD_STORE) || (this->current_inst.opcode == CMD_HALT); // To tell a block MT that it should try to switch threads
		return event;
	}

	void Thread::execute() {
		int dst = this->current_inst.dst_index;
		int src1 = this->current_inst.src1_index;
		int src2 = this->current_inst.src2_index_imm;

		if (this->current_inst.opcode == CMD_NOP){
			return;
		}
		
		else if (this->current_inst.opcode == CMD_ADD){
			this->reg_file.reg[dst] = this->reg_file.reg[src1] + this->reg_file.reg[src2];
			return;
		}
		
		else if (this->current_inst.opcode == CMD_SUB){
			this->reg_file.reg[dst] = this->reg_file.reg[src1] - this->reg_file.reg[src2];
			return;
		}

		else if (this->current_inst.opcode == CMD_ADDI){
			this->reg_file.reg[dst] = this->reg_file.reg[src1] + src2;
			return;
		}

		else if (this->current_inst.opcode == CMD_SUBI){
			this->reg_file.reg[dst] = this->reg_file.reg[src1] - src2;
			return;
		}

		else if (this->current_inst.opcode == CMD_LOAD){
			int offset = 0;
			if (this->current_inst.isSrc2Imm)
				offset = src2;
			else
				offset = this->reg_file.reg[src2];
			
			unsigned int address = this->reg_file.reg[src1] + offset;
			SIM_MemDataRead(address, &(this->reg_file.reg[dst]));
			return;
		}
		
		else if (this->current_inst.opcode == CMD_STORE){
			int offset = 0;
			if (this->current_inst.isSrc2Imm)
				offset = src2;
			else
				offset = this->reg_file.reg[src2];
			
			unsigned int address = this->reg_file.reg[dst] + offset;
			SIM_MemDataWrite(address, this->reg_file.reg[src1]);
			return;
		}
		
		else if (this->current_inst.opcode == CMD_HALT){
			this->is_halted = true;
			return;
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
	
	// Parameters
	int load_lat;
	int store_lat;
	int switch_penalty;
	int N_threads;

	Thread_Handler(bool is_blocked_MT, int load_lat, int store_lat, int switch_pen, int N_threads);
	Thread_Handler();
	bool thread_free();
	bool switch_thread();
	bool all_halted();
	void run_program();
	void run_program_blocked();
	void run_program_finegrained();
	int latency(int opcode);
};

Thread_Handler::Thread_Handler(bool is_blocked_MT, int load_lat, int store_lat, int switch_pen, int N_threads): current_cycle(0), N_instructions(0), is_blocked_MT(is_blocked_MT),
				current_thread_id(0), load_lat(load_lat), store_lat(store_lat), switch_penalty(switch_pen), N_threads(N_threads){
	this->threads = vector<Thread>();
	//Initializing threads
	for (int i=0; i<this->N_threads; i++){
		Thread th = Thread(i);
		this->threads.push_back(th);
	}
	this->current_thread = &(threads[0]);
}

Thread_Handler::Thread_Handler(): current_cycle(0), N_instructions(0), is_blocked_MT(false), current_thread_id(23), load_lat(0), store_lat(0), switch_penalty(0), N_threads(0) {}

bool Thread_Handler::thread_free(){
	Instruction* thread_inst = &(this->current_thread->current_inst);
	cmd_opcode opcode = thread_inst->opcode;
	int latency = 1;

	if (opcode == CMD_LOAD){
		latency = this->load_lat;
	}
	else if (opcode == CMD_STORE){
		latency = this->store_lat;
	}
	return (this->current_cycle - current_thread->start_cycle >= latency); // Wheather this thread is open for running its next instruction
}

int Thread_Handler::latency(int opcode){
	if (opcode == CMD_LOAD)
		return this->load_lat;
	else if (opcode == CMD_STORE)
		return this->store_lat;
	else
		return 0;
}

bool Thread_Handler::switch_thread(){
	// Prioritizing current thread execution in blocked MT
	if (this->is_blocked_MT && !this->current_thread->is_halted){
		if (this->current_cycle - this->current_thread->start_cycle >= this->latency(this->current_thread->current_inst.opcode)){
			return false;
		}
	}
	// Iterating through threads
	for (int i=1; i<=this->N_threads; i++){
		Thread* th = &(this->threads[(this->current_thread_id + i) % this->N_threads]); // Cyclic iteration
		if (th->is_halted)
			continue;
		if (this->current_cycle - th->start_cycle >= this->latency(th->current_inst.opcode)){ // Thread finished running its current instruction
			this->current_thread = th;
			this->current_thread_id = (this->current_thread_id + i) % this->N_threads;
			return false;
		}
	}
	return true;
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
	bool is_idle = false;
	while (!this->all_halted()) {
		if (!is_idle) { // if there's an open thread for running instruction
			this->current_thread->execute_inst(this->current_cycle);
			this->N_instructions++;
		}
		this->current_cycle++;
		is_idle = this->switch_thread(); // Context Switch and check if there's an open thread
	}
}

void Thread_Handler::run_program_blocked(){
	bool is_idle = false;
	bool event = false;
	int prev_id = 0;
	while (!this->all_halted()) {
		if (!is_idle) { // if there's an open thread for running instruction
			event = this->current_thread->execute_inst(this->current_cycle);
			this->N_instructions++;
		}
		this->current_cycle++;
		if (event || is_idle){ // If the current thread is running a memory access instruction (or halted or in idle mode)
			prev_id = this->current_thread_id;
			is_idle = this->switch_thread();
			if (!is_idle && (prev_id != this->current_thread_id)) // Only invoke penalty if switched to a DIFFERENT thread
				this->current_cycle += this->switch_penalty;
		}
	}
}

Thread_Handler blocked_MT;
Thread_Handler finegrained_MT;

void CORE_BlockedMT() {
	blocked_MT = Thread_Handler(true, SIM_GetLoadLat(), SIM_GetStoreLat(), SIM_GetSwitchCycles(), SIM_GetThreadsNum());
	blocked_MT.run_program();
}

void CORE_FinegrainedMT() {
	finegrained_MT = Thread_Handler(false, SIM_GetLoadLat(), SIM_GetStoreLat(), SIM_GetSwitchCycles(), SIM_GetThreadsNum());
	finegrained_MT.run_program();
}

double CORE_BlockedMT_CPI(){
	return double(blocked_MT.current_cycle) / blocked_MT.N_instructions;
}

double CORE_FinegrainedMT_CPI(){
	return double(finegrained_MT.current_cycle) / finegrained_MT.N_instructions;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for (int i=0; i<blocked_MT.N_threads; i++){
		tcontext reg_file = blocked_MT.threads[threadid].reg_file;
		for (int j=0; j<REGS_COUNT; j++){
			context[i].reg[j] = reg_file.reg[j];
		}
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for (int i=0; i<finegrained_MT.N_threads; i++){
		tcontext reg_file = finegrained_MT.threads[threadid].reg_file;
		for (int j=0; j<REGS_COUNT; j++){
			context[i].reg[j] = reg_file.reg[j];
		}
	}
}
