////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_driver.c
//  Description    : This file contains the driver code to be developed by
//                   the students of the 311 class.  See assignment details
//                   for additional information.
//
//   Author        : 
//   Last Modified : 
//

// Include Files

// Project Includes
#include <sg_driver.h>
#include <sg_service.h>

// Defines

//
// Global Data

// Driver file entry

// Global data
int sgDriverInitialized = 0; // The flag indicating the driver initialized
SG_Block_ID sgLocalNodeId;   // The local node identifier
SG_SeqNum sgLocalSeqno;      // The local sequence number

// Driver support functions
int sgInitEndpoint( void ); // Initialize the endpoint

typedef struct {
    int nid;
    int bid;
}Block;

typedef struct{
    char path[1000]; // File path
    SgFHandle handle; // File handle
    uint32_t pos; // Cursor position (in bytes)
    uint64_t size; // File size
    int open; // Boolean to determine if the file is open
    Block blocks[SG_MAX_BLOCKS_PER_FILE]; 
    struct SGFile *next;
} SGFile;


SGFile *liststart, *listend;

SGFile* search(const char *path)
{
    SGFile *index = liststart;
    while(index)
    {
        if (strcmp(index->path, path) == 0)
            return index;
        index = index->next;
    }
    return index;
}

void append(SGFile *element)
{   
    element->next = NULL;
    if(!liststart)
    {
        liststart = element;
        listend = element;
    }

    listend->next = element;
    listend = element;
}

SGFile* searchfh(SgFHandle fh)
{
    SGFile *index = liststart;
    while(index)
    {
        if (index->handle == fh)
            return index;
        index = index->next;
    }
    return index;
}

int nfh = 0;
SgFHandle newhandle()
{
    nfh++;
    return nfh;
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : allocateandwrite
// Description  : creating a file and allocating memory
//
// Inputs       : file - new file to be created and allocated memory to
//                buf - pointer that contains data 
//                blknum - the block number
// Outputs      : return 0 if successful or, -1 if failed
int allocateandwrite(SGFile* file, int blknum, char *buf)
{
    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE + SG_BLOCK_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    size_t pktlen , rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Local and do some initial setup
    logMessage( LOG_INFO_LEVEL, "Initializing local endpoint ..." );
    sgLocalSeqno = SG_INITIAL_SEQNO;

    // Setup the packet
    pktlen = SG_BASE_PACKET_SIZE + SG_BLOCK_SIZE; // will contain data 
    if ( (ret = serialize_sg_packet( sgLocalNodeId , // Local ID
                                    SG_NODE_UNKNOWN,   // Remote ID
                                    SG_BLOCK_UNKNOWN,  // Block ID
                                    SG_CREATE_BLOCK,  // Operation to create and allocate file
                                    SG_SEQNO_UNKNOWN,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    buf, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "allocateandwrite: failed creating block [%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE;
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "allocateandwrite: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "allocateandwrite: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "allocateandwrite: bad local ID returned [%ul]", loc );
        return( -1 );
    }

    file->blocks[file->pos/SG_BLOCK_SIZE].nid = rem;
    file->blocks[file->pos/SG_BLOCK_SIZE].bid = blkid;
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : update
// Description  : updating the contents of a file
//
// Inputs       : nid - the local node id
//                bid - The local block id
//                blknum - the block number
// Outputs      : return 0 if successful or, -1 if failed
int update(SG_Node_ID nid, SG_Block_ID bid, char *buf)
{
    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE + SG_BLOCK_SIZE], recvPacket[SG_BASE_PACKET_SIZE ];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Local and do some initial setup
    logMessage( LOG_INFO_LEVEL, "Initializing local endpoint ..." );
    sgLocalSeqno = SG_INITIAL_SEQNO;

    // Setup the packet
    pktlen = SG_BASE_PACKET_SIZE + SG_BLOCK_SIZE;
    if ( (ret = serialize_sg_packet( sgLocalNodeId , // Local ID
                                    nid,   // Remote ID
                                    bid,  // Block ID
                                    SG_UPDATE_BLOCK,  // Operation to update
                                    SG_SEQNO_UNKNOWN,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    buf, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "update: failed updating [%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE + SG_BLOCK_SIZE; // will contain data
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "update: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "update: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "update: bad local ID returned [%ul]", loc );
        return( -1 );
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : delete
// Description  : delete the data in a file 
//
// Inputs       : nid - local node id
//                bid - local block id
// Outputs      : return 0 if successful or, -1 if failed

int delete( SG_Node_ID nid, SG_Block_ID bid)
{
    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Local and do some initial setup
    logMessage( LOG_INFO_LEVEL, "Initializing local endpoint ..." );
    sgLocalSeqno = SG_INITIAL_SEQNO;

    // Setup the packet
    pktlen = SG_BASE_PACKET_SIZE;
    if ( (ret = serialize_sg_packet( sgLocalNodeId , // Local ID
                                    nid,   // Remote ID
                                    bid,  // Block ID
                                    SG_DELETE_BLOCK,  // Operation to delete file
                                    SG_SEQNO_UNKNOWN,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "delete: failed deleting [%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE;
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "delete: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "delete: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "delete: bad local ID returned [%ul]", loc);
        return( -1 );
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : retrieve
// Description  : retrieving the contents of a file
//
// Inputs       : file - file from which data will be retrieved
//                buf - pointer that contains the data to be retrieved
// Outputs      : return 0 if successful or, -1 if failed

int retrieve(SGFile* file, char *buf)
{
    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE + SG_BLOCK_SIZE];
    size_t pktlen, rpktlen;

    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Local and do some initial setup
    logMessage( LOG_INFO_LEVEL, "Initializing local endpoint ..." );
    sgLocalSeqno = SG_INITIAL_SEQNO;

    // Setup the packet
    pktlen = SG_BASE_PACKET_SIZE;
    if ( (ret = serialize_sg_packet( sgLocalNodeId , // Local ID
                                    file->blocks[file->pos/SG_BLOCK_SIZE].nid,   // Remote ID
                                    file->blocks[file->pos/SG_BLOCK_SIZE].bid,  // Block ID
                                    SG_OBTAIN_BLOCK,  // Operation to retrieve file
                                    SG_SEQNO_UNKNOWN,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "Retrieve: failed retrieving[%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE + SG_BLOCK_SIZE; 
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "Retrieve: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, buf, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "Retriive: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "Retrieve: bad local ID returned [%ul]", loc);
        return( -1 );
    }

    return 0;
}


//
// Functions

//
// File system interface implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

SgFHandle sgopen(const char *path) {
    logMessage(SGDriverLevel, "packet size %d", SG_BASE_PACKET_SIZE);
    // First check to see if we have been initialized
    if (!sgDriverInitialized)
    {
            // Call the endpoint initialization 
        if ( sgInitEndpoint() ) 
        {
            logMessage( LOG_ERROR_LEVEL, "sgopen: Scatter/Gather endpoint initialization failed." );
            return( -1 );
        }
        sgDriverInitialized = 1;
    }
        // Set to initialized
    
    SGFile *file;
    file = search(path);
    // if file has not been created
    if(!file)
    {
        // I am making a new file and setting its paramenters to what it is supposed to be initially 
        file = (SGFile*) (malloc(sizeof(SGFile)));
        strcpy(file->path, path);
        file->handle = newhandle();
        file->size = 0;
        file->pos = 0;
        for( int i = 0; i < SG_MAX_BLOCKS_PER_FILE; i++)
        {
            file->blocks[i].nid = -1;
            file->blocks[i].bid = -1;
        }
        file->next = NULL;
        file->open = 1;
        append(file);
        return file->handle;
    }
    // if file is created but not opened
    if(!file->open)
    {
        file->open = 1;
        file->handle = newhandle();
        file->pos = 0;
        return file->handle;
    }

    return -1;
}



////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgread
// Description  : Read data from the file
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure

int sgread(SgFHandle fh, char *buf, size_t len) 

{
    // getting the file with that file handle
    SGFile *file = searchfh(fh);
    // if there is no file
    if (!file)
        return -1;
    
    if (file->pos < file->size)
    {   // file edge case
        if (file->pos % SG_BLOCK_SIZE == 0)
        {
            // reading the data and then setting pos to what its supposed to be and then return the block size
            retrieve(file, buf);
            file->pos += SG_BLOCK_SIZE;
            return SG_BLOCK_SIZE;
        }
        
    }      

    // Return the bytes processed
    return( len );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgwrite
// Description  : write data to the file
//
// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure




int sgwrite(SgFHandle fh, char *buf, size_t len) {
    
    // getting the file with that file handle
    SGFile *file = searchfh(fh);

    //if there is no file 
    if (!file)
        return -1;

    if (file->pos == file->size)
    {   // edge case
        if (file->pos % SG_BLOCK_SIZE == 0)
        {   // creating and allocating data, setting the size and pos to whats its supposed to be and then returning block size
            allocateandwrite(file, file->pos/SG_BLOCK_SIZE, buf);
            file->size += SG_BLOCK_SIZE;
            file->pos += SG_BLOCK_SIZE;
            return SG_BLOCK_SIZE;
        
        }
    }      

    // Log the write, return bytes written
    return( len );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : new position if successful, -1 if failure

int sgseek(SgFHandle fh, size_t off) {
    // getting the file with that file handle
    SGFile *file = searchfh(fh);
    // if there is no file

    if (!file)
        return -1;

    // if offset is greater than file size
    if(off > file->size)
        file->pos = file->size;
    else
    {
        file->pos = off;
    }
    // Return new position
     return( off );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure

int sgclose(SgFHandle fh) {
    // invalid file handle 
    if( fh == -1)
        return -1;

    // getting the file with that file handle
    SGFile *file = searchfh(fh);

    //if no file
    if(!file)
        return -1;
    
    //if file is not open 
    if (!(file->open))
        return -1;

    //closing the file
    file->handle = -1;
    file->open = 0;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int sgshutdown(void) {

    // shutting down the system 
    char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    SGFile *index = liststart;
    
    while(index)
    {
        sgclose(index->handle);
        index = index->next;
        if(index == listend->next)
            break;
    }

    // Setup the packet   
    pktlen = SG_BASE_PACKET_SIZE;
    if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                                    SG_NODE_UNKNOWN,   // Remote ID
                                    SG_BLOCK_UNKNOWN,  // Block ID                                                                      
                                    SG_STOP_ENDPOINT,  // Operation to close file
                                    SG_SEQNO_UNKNOWN,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "ShutDown: failed shutting down the system [%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE;
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "ShutDown: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "ShutDown: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "ShutDown: bad local ID returned [%ul]", loc );
        return( -1 );
    }

    // Log, return successfully
    logMessage( LOG_INFO_LEVEL, "Shut down Scatter/Gather driver." );
    return( 0 );

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : serialize_sg_packet
// Description  : Serialize a ScatterGather packet (create packet)
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                data - the data block (of size SG_BLOCK_SIZE) or NULL
//                packet - the buffer to place the data
//                plen - the packet length (int bytes)
// Outputs      : 0 if successfully created, -1 if failure

SG_Packet_Status serialize_sg_packet(SG_Node_ID loc, SG_Node_ID rem, SG_Block_ID blk,
                                     SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq, char *data,
                                     char *packet, size_t *plen) {
            // using to indicate data
            char dataindicator = 0;
            // Checking packet status
            if(packet == NULL)
                return SG_PACKT_PDATA_BAD;

            //logMessage(SGDriverLevel, "loc = %u, rem = %u, blk = %u, op = %d ", loc, rem, blk, op);
            //logMessage(SGDriverLevel, "sseq = %d, rseq = %d", sseq, rseq);
            //logMessage(SGDriverLevel, "data = %s", data);
            //logMessage(SGDriverLevel, "plen = %d", *plen);

            //initializing magic
            uint32_t magic;
            magic = SG_MAGIC_VALUE;

            //Checking if the inputs are valid
            if(loc == 0)
                return SG_PACKT_LOCID_BAD;
            if(rem == 0)
                return SG_PACKT_REMID_BAD;
            if(blk == 0)
                return SG_PACKT_BLKID_BAD;
            if(sseq == 0)
                return SG_PACKT_SNDSQ_BAD;
            if(rseq == 0)
                return SG_PACKT_RCVSQ_BAD;
            if(op<0 || op>6)
                return SG_PACKT_OPERN_BAD;
            
            // copying values of the given local variables into the buffer -> packet

            
            memcpy(&packet[0],&magic, sizeof(magic));
            memcpy(&packet[4],&loc, sizeof(loc));
            memcpy(&packet[12],&rem,sizeof(rem));
            memcpy(&packet[20],&blk,sizeof(blk));
            memcpy(&packet[28],&op,sizeof(op));
            memcpy(&packet[32],&sseq,sizeof(sseq));
            memcpy(&packet[34],&rseq,sizeof(rseq));
            
            //assigning value for data indicator
            if(data != NULL)
            {
                dataindicator = 1;
            }
            else
                dataindicator = 0;

            memcpy(&packet[36],&dataindicator, sizeof(dataindicator));
            

            // code to add data block or not
            if(dataindicator == 1)
            {   
                memcpy(&packet[37],&data[0],1024);
                memcpy(&packet[1061],&magic,4);
                
            }
            else
            { 
                memcpy(&packet[37],&magic,4);
               
            }
            
            // return if the packet status is good
            return SG_PACKT_OK;


    // Return the system function return value
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : deserialize_sg_packet
// Description  : De-serialize a ScatterGather packet (unpack packet)
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                data - the data block (of size SG_BLOCK_SIZE) or NULL
//                packet - the buffer to place the data
//                plen - the packet length (int bytes)
// Outputs      : 0 if successfully created, -1 if failure

SG_Packet_Status deserialize_sg_packet(SG_Node_ID *loc, SG_Node_ID *rem, SG_Block_ID *blk,
                                       SG_System_OP *op, SG_SeqNum *sseq, SG_SeqNum *rseq, char *data,
                                       char *packet, size_t plen) {
            char buffer[plen];

            memcpy(buffer, packet, plen);

            // copying all the values from the buffer to the variables given to us
            memcpy(loc, &buffer[4],8);
            memcpy(rem,&buffer[12],8);
            memcpy(blk, &buffer[20],8);
            memcpy(op, &buffer[28],4);
            memcpy(sseq,&buffer[32],2);
            memcpy(rseq,&buffer[34],2);
            //logMessage(SGDriverLevel, "loc = %u, rem = %u, blk = %u, op = %d ", *loc, *rem, *blk, *op);
            //logMessage(SGDriverLevel, "sseq = %d, rseq = %d", *sseq, *rseq);

            // To see if there is a data block or not
            if(buffer[36]== 1)
            {
                memcpy(data,&buffer[37],1024);
            }
            
            // Checking if the inputs are valid
            if(*loc == 0)
                return SG_PACKT_LOCID_BAD;
            if(*rem == 0)
                return SG_PACKT_REMID_BAD;
            if(*blk == 0)
                return SG_PACKT_BLKID_BAD;
            if(*sseq == 0)
                return SG_PACKT_SNDSQ_BAD;
            if(*rseq == 0)
                return SG_PACKT_RCVSQ_BAD;
            if(*op < 0 || *op>6)
                return SG_PACKT_OPERN_BAD;
            
            //deserialize worked
            return SG_PACKT_OK;


    // Return the system function return value
    
}

//
// Driver support functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgInitEndpoint
// Description  : Initialize the endpoint
//
// Inputs       : none
// Outputs      : 0 if successfull, -1 if failure

int sgInitEndpoint( void ) {

    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Local and do some initial setup
    logMessage( LOG_INFO_LEVEL, "Initializing local endpoint ..." );
    sgLocalSeqno = SG_INITIAL_SEQNO;

    // Setup the packet
    pktlen = SG_BASE_PACKET_SIZE;
    if ( (ret = serialize_sg_packet( SG_NODE_UNKNOWN, // Local ID
                                    SG_NODE_UNKNOWN,   // Remote ID
                                    SG_BLOCK_UNKNOWN,  // Block ID
                                    SG_INIT_ENDPOINT,  // Operation
                                    sgLocalSeqno++,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE;
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: bad local ID returned [%ul]", loc );
        return( -1 );
    }

    // Set the local node ID, log and return successfully
    sgLocalNodeId = loc;
    logMessage( LOG_INFO_LEVEL, "Completed initialization of node (local node ID %lu", sgLocalNodeId );
    return( 0 );
}
