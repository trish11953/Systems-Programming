////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_driver.c
//  Description    : This file contains the driver code to be developed by
//                   the students of the 311 class.  See assignment details
//                   for additional information.
//
//   Author        : YOUR NAME
//   Last Modified : 
//

// Include Files
#include <stdlib.h>
#include <cmpsc311_log.h>

// Project Includes
#include <sg_cache.h>
#include <stdio.h>
#include <limits.h>

// Defines

// Functional Prototypes


struct cacheBlock{
    uint64_t nid;
    uint64_t bid;
    char data[SG_BLOCK_SIZE];
    int timestamp;
};

struct cacheBlock *cache;

//global variables
int hits = 0;
int tries = 0;
uint16_t maxCacheSize;
int globaltime = 0;
char transferbuf[SG_BLOCK_SIZE];

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : initSGCache
// Description  : Initialize the cache of block elements
//
// Inputs       : maxElements - maximum number of elements allowed
// Outputs      : 0 if successful, -1 if failure

int initSGCache( uint16_t maxElements ) {
   //initializing cache with basic numbers
    cache = (struct cacheBlock*)malloc(maxElements*sizeof(struct cacheBlock));
    maxCacheSize = maxElements;
    for(int i = 0; i < maxCacheSize; i++)
    { 
        logMessage( LOG_ERROR_LEVEL,"Initialised cache line %d", i);
        cache[i].nid = -1;
        cache[i].bid = -1;
        
    }
 
    // Return successfully
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : closeSGCache
// Description  : Close the cache of block elements, clean up remaining data
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int closeSGCache( void ) {
    //freeing cache memory
    free(cache);
    // calculating hits ratio
    double hitsratio = ((double)hits/(double)tries)*100;
    logMessage( SGDriverLevel, "Closing cache: %d queries, %d hits (%.2f %% hit rate).", tries, hits, hitsratio);
    // Return successfully
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : getSGDataBlock
// Description  : Get the data block from the block cache
//
// Inputs       : nde - node ID to find
//                blk - block ID to find
// Outputs      : pointer to block or NULL if not found

char * getSGDataBlock( SG_Node_ID nde, SG_Block_ID blk ) {
 
 //getting cache block
    
    for(int i = 0; i < maxCacheSize; i++)
    {  //Looking at cache if nodeid and blockid is same
        if(cache[i].nid == nde && cache[i].bid == blk)
        {   
            cache[i].timestamp = globaltime;
            globaltime++;
            //logMessage( SGDriverLevel, "HIT: nid = %d, blk = %d", nde, blk);
            hits++;
            memcpy(&transferbuf, &cache[i].data, SG_BLOCK_SIZE);
            return &transferbuf;
        }
    }

    tries++;

    // Return successfully
    return( NULL );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : putSGDataBlock
// Description  : Put the data block from the block cache
//
// Inputs       : nde - node ID to find
//                blk - block ID to find
//                block - block to insert into cache
// Outputs      : 0 if successful, -1 if failure


int putSGDataBlock( SG_Node_ID nde, SG_Block_ID blk, char *block ) {
    //putting data cache block
    int LRUtime = INT_MAX;
    int index = -1;
    tries++;
    for(int i = 0; i < maxCacheSize; i++)
    {   
        //logMessage( LOG_ERROR_LEVEL,"Inside put function");
        if(cache[i].nid == nde && cache[i].bid == blk)
        {   
            //logMessage( LOG_ERROR_LEVEL,"already in cache");
            memcpy(&cache[i].data, block, SG_BLOCK_SIZE);
            cache[i].timestamp = globaltime;
            globaltime++;
            hits++;
            //"Already in cache"
            return 0;
        }
        
        if (cache[i].nid == -1 && cache[i].bid == -1)
        {   
            //logMessage( LOG_ERROR_LEVEL,"not in cache");
            memcpy(&cache[i].data, block, SG_BLOCK_SIZE);
            cache[i].nid = nde;
            cache[i].bid = blk;
            cache[i].timestamp = globaltime;
            globaltime++;
            hits++;
            //"Inserting, no replacment."
            return 0;
            
        }
     
        if(cache[i].timestamp < LRUtime)
        {   
            LRUtime = cache[i].timestamp;
            index = i;
        }
        
    }


    //logMessage( LOG_ERROR_LEVEL,"Replacing");
    //"Replacing cache[%d,%d] with [%d,%d]", cache[index].nid , cache[index].bid, nde, blk
    memcpy(&cache[index].data, block, SG_BLOCK_SIZE);
    cache[index].nid = nde;
    cache[index].bid = blk;
    cache[index].timestamp = globaltime;
    globaltime++;
    
   
    // Return successfully
    return( 0 );
}
