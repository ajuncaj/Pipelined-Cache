/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>
#include <math.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

extern int mem_access(int addr, int write_flag, int write_data);
extern int get_num_mem_accesses();
int get_block_offset(int addr);
int get_set_index(int addr);

enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int valid; // new
    int dirty;
    int lruLabel;
    int set;
    int tag;
    int startAddr; // new
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE][MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache();

/*
 * Set up the cache with given command line parameters. This is 
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet){
    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;
    
    // Set all tags to -1 so we don't get hits at the start
    for (int i = 0; i < numSets; i++) {
        for (int j = 0; j < blocksPerSet; j++) {
            cache.blocks[i][j].tag = -1;
            cache.blocks[i][j].lruLabel = -100000;
        }
    }

    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data) {
    
    // Extract what you need
    int block_offset = get_block_offset(addr);
    int set_index = get_set_index(addr);
    int tag = addr >> (int)((log2(cache.numSets) + log2(cache.blockSize)));
    

    // Look through cache to see if current addr is already in there
    for (int block = 0; block < cache.blocksPerSet; block++) {
        if (cache.blocks[set_index][block].tag == tag && cache.blocks[set_index][block].valid == 1) { // Hit!
            // Update LRU
            // Store oldlru, then increment all that are less than that. Set new = 0
            int oldlru = cache.blocks[set_index][block].lruLabel;
            for (int lruBlock = 0; lruBlock < cache.blocksPerSet; lruBlock++) {
                if (cache.blocks[set_index][lruBlock].lruLabel < oldlru) {
                    cache.blocks[set_index][lruBlock].lruLabel++;
                }
            }
            cache.blocks[set_index][block].lruLabel = 0;

            if (write_flag) { // Stores
                printAction(addr, 1, processorToCache);
                cache.blocks[set_index][block].data[block_offset] = write_data;
                cache.blocks[set_index][block].dirty = 1;
                return -1;
            }
            else { // Loads
                printAction(addr, 1, cacheToProcessor);
                return cache.blocks[set_index][block].data[block_offset];
            }
        }
    }
    // Miss!
    // Look for empty block
    int block = 0;
    int isFull = 0;
    for (block = 0; block < cache.blocksPerSet; block++) {
        if (cache.blocks[set_index][block].valid == 0) {
            break;
        }
        else if (block  == cache.blocksPerSet - 1) { // If everything is full
            isFull = 1;
        }
    }

    // If it's full, need to find + evict LRU
    if (isFull) {
        // Loop through block to find the highest lru
        int highest = -1;
        for (int i = 0; i < cache.blocksPerSet; i++) {
            if (cache.blocks[set_index][i].lruLabel > highest) {
                highest = cache.blocks[set_index][i].lruLabel;
                block = i;
            }
        }
        
        if (cache.blocks[set_index][block].dirty) { // If dirty, send to mem
            printAction(cache.blocks[set_index][block].startAddr, cache.blockSize, cacheToMemory);
            cache.blocks[set_index][block].dirty = 0;
            for (int word = 0; word < cache.blockSize; word++) {
                mem_access(cache.blocks[set_index][block].startAddr + word, 1, cache.blocks[set_index][block].data[block_offset]);
            }
        }
        else { // Not dirty, no need to send to mem
            printAction(cache.blocks[set_index][block].startAddr, cache.blockSize, cacheToNowhere);
        }
    }

    // Bring in the block from mem
    printAction(addr - block_offset, cache.blockSize, memoryToCache);
    cache.blocks[set_index][block].valid = 1;
    cache.blocks[set_index][block].tag = tag;
        
    // Fill block with proper data
    for (int word = 0; word < cache.blockSize; word++) {
        int data = mem_access(addr - block_offset + word, 0, 0);
        cache.blocks[set_index][block].data[word] = data;
    }

    // Store address for easy eviction
    cache.blocks[set_index][block].startAddr = addr - block_offset;

    // Update LRU
    for (int lruBlock = 0; lruBlock < cache.blocksPerSet; lruBlock++) {
        cache.blocks[set_index][lruBlock].lruLabel++;
    }
    cache.blocks[set_index][block].lruLabel = 0;
   
    if (write_flag) { // Stores
        // Write to cache.
        printAction(addr, 1, processorToCache);
        cache.blocks[set_index][block].data[block_offset] = write_data;
        cache.blocks[set_index][block].dirty = 1;
        return -1;
    }
    else { // Loads
        // Send over new data to processor by accessing the word we just put in there
        printAction(addr, 1, cacheToProcessor);
        return cache.blocks[set_index][block].data[block_offset];
    }
}


int get_block_offset(int addr) {
    return addr % cache.blockSize;
}

int get_set_index(int addr) {
    unsigned  mask;
    mask = ((1 << (int)log2(cache.numSets) + (int)log2(cache.blockSize)) - 1);
    mask = addr & mask;
    return mask >> (int)log2(cache.blockSize);
}


/*
 * print end of run statistics like in the spec. This is not required,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(){
    return;
}

/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
}
/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
 /*
 void printCache()
{
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index) {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}
 */
