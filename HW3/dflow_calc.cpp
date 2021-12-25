/* 046267 Computer Architecture - Winter 20/21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */



#include "dflow_calc.h"
#include "vector"

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
    int write_reg;

    unsigned int read_reg1;
    unsigned int read_reg2;
    
    InstNode* dep1;
    InstNode* dep2;

} InstNode;

typedef struct ExitNode
{
    int inst_idx;
    int num_deps;
    InstNode** deps;

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
    for (int i=0; i < exit->num_deps; i++) {
        if (exit->deps[i] == NULL)
            break;
        dep1 = find_node(exit->deps[i], inst->read_reg1);
        if (dep1 != NULL && dep1->inst_idx > dep1_recent->inst_idx)
            dep1_recent = dep1;
        
        dep2 = find_node(exit->deps[i], inst->read_reg2);
        if (dep2 != NULL && dep2->inst_idx > dep2_recent->inst_idx)
            dep2_recent = dep2;
    }

    if (dep1_recent == NULL && dep2_recent == NULL){
        inst->dep1 = &(tree->entry);
        inst->dep2 = &(tree->entry);
    }
    else if (dep1_recent != NULL && dep2_recent == NULL){
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

    for (int i=0; i < exit->num_deps; i++) {
        if (exit->deps[i] == NULL)
            break;
        if (exit->deps[i]->inst_idx == inst->dep1->inst_idx) {
            exit->deps[i] = NULL;
            for (int j = i; j < exit->num_deps-1; j++){
                exit->deps[j] = exit->deps[j+1];
            }
            i--;
        }
        if (exit->deps[i]->inst_idx == inst->dep2->inst_idx) {
            exit->deps[i] = NULL;
            for (int j = i; j < exit->num_deps-1; j++){
                exit->deps[j] = exit->deps[j+1];
            }
            i--;
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
    else:
        for DEP in our dependencies:
            connect (US -> DEP)
            if (exit -> DEP):
                remove (exit -> entry)
            
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
    new_tree->exit.deps = (InstNode**) malloc(EXIT_START_SIZE * sizeof(InstNode*)); // Allocating Exit ->
    new_tree->exit.deps[0] = &(new_tree->entry); // Connecting (Exit -> Entry)
    for (int i=1; i < new_tree->exit.num_deps; i++)
        new_tree->exit.deps[i] = NULL;

    new_tree->num_insts = 0;

    new_tree->ops_latency = opsLatency;

    for (int i=0; i < numOfInsts; i++) {
        InstNode* new_inst = (InstNode*) malloc(sizeof(InstNode));
        new_inst->opcode = progTrace[i].opcode;
        new_inst->cycles = opsLatency[new_inst->opcode];
        new_inst->inst_idx = new_tree->num_insts;
        new_tree->num_insts++;

        new_inst->read_reg1 = progTrace[i].src1Idx;
        new_inst->read_reg2 = progTrace[i].src2Idx;
        new_inst->write_reg = progTrace[i].dstIdx;

        add_inst(new_tree, new_inst);
    }

    return new_tree;
}

void freeProgCtx(ProgCtx ctx) {
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    return -1;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    return -1;
}

int getProgDepth(ProgCtx ctx) {
    return 0;
}


