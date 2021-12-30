/* 046267 Computer Architecture - Winter 20/21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */



#include "dflow_calc.h"
#include "vector"
#include <math.h>

#define EXIT_START_SIZE 10
#define ENTRY_IDX -1
#define EXIT_IDX -2
#define NO_REG -10
#define NO_OP -11

using std::vector;

typedef struct InstNode
{
    unsigned int opcode;
    int inst_idx;
    int cycles;
    unsigned int write_reg;

    unsigned int read_reg1;
    unsigned int read_reg2;

    int inst_depth; // Not counting current instruction cycles
    
    InstNode* dep1;
    InstNode* dep2;

} InstNode;

typedef struct ExitNode
{
    int inst_idx;
    int num_deps;
    InstNode** deps;

    int max_depth;

} ExitNode;

typedef struct DepTree
{
    int num_insts;
    const unsigned int* ops_latency;
    InstNode entry;
    ExitNode exit;
} DepTree;

InstNode* find_node(InstNode* root, unsigned int read_idx) {
    if (root->inst_idx == ENTRY_IDX)
        return NULL;
    if (root->write_reg == read_idx)
        return root;
    InstNode* first_search = find_node(root->dep1, read_idx);
    InstNode* second_search = find_node(root->dep2, read_idx);
    if (first_search != NULL && second_search == NULL)
        return first_search;
    if (second_search != NULL && first_search == NULL)
        return second_search;
    if (second_search != NULL && first_search != NULL) {
        if (first_search->inst_idx > second_search->inst_idx)
            return first_search;
        else
            return second_search;
    }
    return NULL;
}

void add_inst(DepTree* tree, InstNode* inst) {
    ExitNode* exit = &(tree->exit);
    InstNode* dep1_recent = NULL;
    InstNode* dep2_recent = NULL;
    InstNode* dep1 = NULL;
    InstNode* dep2 = NULL;

    // Searching each path from Exit Node
    for (int i=0; i < exit->num_deps; i++) {
        if (exit->deps[i] == NULL)
            break;
        dep1 = find_node(exit->deps[i], inst->read_reg1);
        if ((dep1 != NULL) && (dep1_recent == NULL || dep1->inst_idx > dep1_recent->inst_idx)) // Only takes most recent matched instruction
            dep1_recent = dep1;
        
        dep2 = find_node(exit->deps[i], inst->read_reg2);
        if ((dep2 != NULL) && (dep2_recent == NULL || dep2->inst_idx > dep2_recent->inst_idx)) // Only takes most recent matched instruction
            dep2_recent = dep2;
    }

    //No Dependencies
    if (dep1_recent == NULL && dep2_recent == NULL){
        inst->dep1 = &(tree->entry);
        inst->dep2 = &(tree->entry);
        inst->inst_depth = 0;
    }
    else { //Dependencies exist
        if (dep1_recent != NULL && dep2_recent == NULL){
            inst->dep1 = dep1_recent;
            inst->dep2 = dep1_recent;
        }
        else if (dep2_recent != NULL && dep1_recent == NULL){
            inst->dep1 = dep2_recent;
            inst->dep2 = dep2_recent;
        }
        else{
            inst->dep1 = dep1_recent;
            inst->dep2 = dep2_recent;
        }
        inst->inst_depth = fmax(inst->dep1->inst_depth + inst->dep1->cycles, inst->dep2->inst_depth + inst->dep2->cycles);
    }

    //Maintaining maximum depth
    exit->max_depth = fmax(exit->max_depth, inst->inst_depth + inst->cycles);

    //Disconecting (exit -> dependencies)
    bool found_match = false;
    for (int i=0; i < exit->num_deps; i++) {
        if (exit->deps[i] == NULL)
            break;
        if (exit->deps[i]->inst_idx == inst->dep1->inst_idx) {
            exit->deps[i] = NULL;
            //Shifting exit dependencies for easier maintainence
            for (int j = i; j < exit->num_deps-1; j++){
                exit->deps[j] = exit->deps[j+1];
            }
            found_match = true;
        }
        if (exit->deps[i] == NULL)
            break;
        if (exit->deps[i]->inst_idx == inst->dep2->inst_idx) {
            exit->deps[i] = NULL;
            //Shifting exit dependencies for easier maintainence
            for (int j = i; j < exit->num_deps-1; j++){
                exit->deps[j] = exit->deps[j+1];
            }
            found_match = true;
        }
        if (found_match){
            i--;
            found_match = false;
        }
    }

    bool full = true;
    for (int i=0; i < exit->num_deps; i++) {
        if (exit->deps[i] == NULL){
            exit->deps[i] = inst;
            full = false;
            break;
        }
    }

    if (full){
        InstNode** new_deps = (InstNode**) malloc((2 * exit->num_deps) * sizeof(InstNode*));
        for (int i=0; i < exit->num_deps; i++) {
            new_deps[i] = exit->deps[i];
        }
        new_deps[exit->num_deps] = inst;
        for (int i=exit->num_deps + 1; i < 2 * exit->num_deps; i++) {
            new_deps[i] = NULL;
        }
        exit->num_deps = 2 * exit->num_deps;
        InstNode** prev_deps = exit->deps;
        exit->deps = new_deps;
        free(prev_deps);
    }
}

/* Add inst:
    if no dependencies:
        connect (US -> Entry)
        US.depth = 0
    else:
        for DEP in our dependencies:
            connect (US -> DEP)
            if (exit -> DEP):
                remove (exit -> entry)
        US.depth = max(dep1.depth + dep1.cycles, dep2.depth + dep2.cycles)
    exit.depth = max(exit.depth, US.depth + inst.cycles)
    connect (exit -> US)
*/

/*
typedef struct {
    unsigned int opcode;
             int dstIdx;
    unsigned int src1Idx;
    unsigned int src2Idx;
} InstInfo;
*/

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    
    DepTree* new_tree = (DepTree*) malloc(sizeof(DepTree)); //Allocating
    new_tree->entry.inst_idx = ENTRY_IDX; // For detection when on entry node
    new_tree->entry.dep1 = NULL;
    new_tree->entry.dep2 = NULL;
    new_tree->entry.read_reg1 = NO_REG;
    new_tree->entry.read_reg2 = NO_REG;
    new_tree->entry.write_reg = NO_REG;
    new_tree->entry.opcode = NO_OP;
    new_tree->entry.cycles = 0;

    new_tree->exit.inst_idx = EXIT_IDX;
    new_tree->exit.num_deps = EXIT_START_SIZE;
    new_tree->exit.max_depth = 0;
    new_tree->exit.deps = (InstNode**) malloc(EXIT_START_SIZE * sizeof(InstNode*)); // Allocating Exit ->
    new_tree->exit.deps[0] = &(new_tree->entry); // Connecting (Exit -> Entry)
    for (int i=1; i < new_tree->exit.num_deps; i++)
        new_tree->exit.deps[i] = NULL;

    new_tree->num_insts = 0;

    new_tree->ops_latency = opsLatency;

    for (unsigned int i=0; i < numOfInsts; i++) {
        InstNode* new_inst = (InstNode*) malloc(sizeof(InstNode));
        new_inst->opcode = progTrace[i].opcode;
        new_inst->cycles = opsLatency[new_inst->opcode];
        new_inst->inst_idx = new_tree->num_insts;
        new_inst->inst_depth = 0;
        new_tree->num_insts++;

        new_inst->read_reg1 = progTrace[i].src1Idx;
        new_inst->read_reg2 = progTrace[i].src2Idx;
        new_inst->write_reg = progTrace[i].dstIdx;

        add_inst(new_tree, new_inst);
    }

    return new_tree;
}

InstNode* find_by_idx_node(InstNode* inst, unsigned int idx) {
    
    if (inst->inst_idx == ENTRY_IDX)
        return NULL;
    if (inst->inst_idx == idx)
        return inst;
    
    InstNode* inst1 = find_by_idx_node(inst->dep1, idx);
    InstNode* inst2 = NULL;
    if (inst->dep1->inst_idx != inst->dep2->inst_idx)
        inst2 = find_by_idx_node(inst->dep2, idx);

    if (inst1 != NULL)
        return inst1;
    if (inst2 != NULL)
        return inst2;

    return NULL;
}

InstNode* find_by_idx(DepTree* tree, unsigned int idx) {
    ExitNode* exit = &(tree->exit);
    for (int i=0; i < exit->num_deps; i++) {
        if (exit->deps[i] == NULL)
            break;
        InstNode* inst = find_by_idx_node(exit->deps[i], idx);
        if (inst != NULL)
            return inst;
    }
    return NULL;
}

bool exists_in_set(InstNode** node_set, int set_size, unsigned int idx) {
    for (int i=0; i < set_size; i++){
        if (node_set[i] == NULL)
            return false;
        if (node_set[i]->inst_idx == idx)
            return true;
    }
    return false;
}

void add_to_node_set(InstNode** node_set, int set_size, InstNode* inst, int* j) {
    if (inst->inst_idx == ENTRY_IDX)
        return;
    
    if (!exists_in_set(node_set, set_size, inst->inst_idx)){
        node_set[*j] = inst;
        (*j)++;
    }
    add_to_node_set(node_set, set_size, inst->dep1, j);
    if (inst->dep1->inst_idx != inst->dep2->inst_idx)
        add_to_node_set(node_set, set_size, inst->dep2, j);
}

void freeProgCtx(ProgCtx ctx) {
    DepTree* tree = (DepTree*) ctx;
    ExitNode* exit = &(tree->exit);
    InstNode** node_set = (InstNode**) malloc(tree->num_insts * sizeof(InstNode*)); //Set for holding nodes

    for (int i = 0; i < tree->num_insts; i++)
        node_set[i] = NULL;

    //Adding Nodes to set prior to freeing
    int j = 0;
    for (int i=0; i < exit->num_deps; i++) {
        if (exit->deps[i] == NULL)
            break;
        add_to_node_set(node_set, tree->num_insts, exit->deps[i], &j);
    }

    for (int i=0; i < tree->num_insts; i++){
        free(node_set[i]);
    }
    free(node_set);
    free(tree->exit.deps);
    free(tree);

    return;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    DepTree* tree = (DepTree*) ctx;
    InstNode* inst = find_by_idx(tree, theInst);
    if (inst != NULL)
        return inst->inst_depth;
    return -1;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    DepTree* tree = (DepTree*) ctx;
    InstNode* inst = find_by_idx(tree, theInst);
    if (inst != NULL) {
        //Different Dependencies
        if (inst->dep1->inst_idx != inst->dep2->inst_idx) {
            *src1DepInst = inst->dep1->inst_idx;
            *src2DepInst = inst->dep2->inst_idx;
        }
        // Same Dependencies
        else {
            //dep1 is the true dependency
            if (inst->dep1->write_reg == inst->read_reg1) {
                *src1DepInst = inst->dep1->inst_idx;
                *src2DepInst = ENTRY_IDX;
            }
            //dep2 is the true dependency
            else if (inst->dep2->write_reg == inst->read_reg2){
                *src1DepInst = ENTRY_IDX;
                *src2DepInst = inst->dep2->inst_idx;
            }
            //No Dependency
            else{
                *src1DepInst = ENTRY_IDX;
                *src2DepInst = ENTRY_IDX;
            }
        }
        return 0;
    }
    return -1;
}

int getProgDepth(ProgCtx ctx) {
    DepTree* tree = (DepTree*) ctx;
    return tree->exit.max_depth;
}


