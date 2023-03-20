/*
 * Project 1
 * EECS 370 LC-2K Instruction-level simulator
 *
 * Make sure *not* to modify printState or any of the associated functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// #pragma warning(disable:4996) // fixes errors in debugger

// Machine Definitions
#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */

// File
#define MAXLINELENGTH 1000 /* MAXLINELENGTH is the max number of characters we read */

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

void printState(stateType *);

int convertNum(int);

void initialize(stateType*);

// *********************NEWLY ADDED CODE FOR CACHE (P4)**************************
extern void cache_init(int blockSize, int numSets, int blocksPerSet);
extern int cache_access(int addr, int write_flag, int write_data);
extern void printStats();
static stateType state;
static int num_mem_accesses = 0;
int mem_access(int addr, int write_flag, int write_data) {
    ++num_mem_accesses;
    if (write_flag) {
        state.mem[addr] = write_data;
    }
    return state.mem[addr];
}
int get_num_mem_accesses() {
    return num_mem_accesses;
}

// *********************NEWLY ADDED CODE FOR CACHE (P4)**************************





int main(int argc, char *argv[])
{
    char line[MAXLINELENGTH];
    // stateType state;
    FILE *filePtr;

    if (argc != 5) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    // CACHE_INIT
    cache_init(*argv[2], *argv[3], *argv[4]);
    // CACHE_INIT

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    /* read the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++) {
        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }

    //Your code starts here
    stateType* statePtr = &state;
    initialize(statePtr);
    // printState(statePtr); // First print, just the initial state

    // Main loop to process entire file. Goes until it hits a halt command.
    int halt = 1;
    int counter = 1;
    while (halt) {
        // int current = state.mem[state.pc]; <--- old
        int current = cache_access(state.pc, 0, 0); // fetch instr. from cache

        // Get opcode
        int temp = 0b111 << 22;
        temp = current & temp;
        int opcode = temp >> 22;
        // Get arg0
        temp = 0b111 << 19;
        temp = current & temp;
        int arg0 = temp >> 19;
        // Get arg1
        temp = 0b111 << 16;
        temp = current & temp;
        int arg1 = temp >> 16;


        if (opcode == 0) { // add
            // Get arg2 (destination)
            temp = 0b111;
            int arg2 = current & temp;

            state.reg[arg2] = state.reg[arg0] + state.reg[arg1];
        }
        else if (opcode == 1) { // nor
            // Get arg2 (destination)
            temp = 0b111;
            int arg2 = current & temp;

            state.reg[arg2] = ~(state.reg[arg0] | state.reg[arg1]);
        }
        else if (opcode == 2) { // lw
            // Grab offsetfield, then convert it to proper int 
            temp = 0b1111111111111111;
            int offsetField = current & temp;
            offsetField = convertNum(offsetField);
            
            // state.reg[arg1] = state.mem[offsetField + state.reg[arg0]]; <--- old

            state.reg[arg1] = cache_access(offsetField + state.reg[arg0], 0, 0);
        }
        else if (opcode == 3) { // sw
            // Grab offsetfield, then convert it to proper int 
            temp = 0b1111111111111111;
            int offsetField = current & temp;
            offsetField = convertNum(offsetField);

            if (state.numMemory <= offsetField + state.reg[arg0]) //if we are trying to store something on the stack
                state.numMemory = offsetField + state.reg[arg0] + 1; //expand our 'memorysize' so that printstate shows the stack

            // state.mem[offsetField + state.reg[arg0]] = state.reg[arg1]; <--- old

            cache_access(offsetField + state.reg[arg0], 1, state.reg[arg1]);
        }
        else if (opcode == 4) { // beq
            // Grab offsetfield, then convert it to proper int 
            temp = 0b1111111111111111;
            int offsetField = current & temp;
            offsetField = convertNum(offsetField);

            if (state.reg[arg0] == state.reg[arg1]) {
                state.pc = state.pc + offsetField;
            }
        }
        else if (opcode == 5) { // jalr
            state.reg[arg1] = state.pc + 1;
            state.pc = state.reg[arg0];
            state.pc--;
        }
        else if (opcode == 6) { // halt
            halt = 0;
            printf("machine halted\n");
            printf("total of %d instructions executed\n", counter);
            printf("final state of machine:\n");

            printState(statePtr);
            printStats();
            printf("%d", get_num_mem_accesses()); // Might add to printStats
        }
        
        counter++;
        // Always increase pc by 1 and printState at end
        state.pc++;
        // printState(statePtr); maybe add back in idk
    }
    return(0);
}

void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
              printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
              printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}

// Sets pc and all registers to 0
void initialize(stateType* statePtr) {
    statePtr->pc = 0;
    for (int i = 0; i < NUMREGS; i++) {
        statePtr->reg[i] = 0;
    }
}
