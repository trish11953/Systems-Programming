////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_driver.c
//  Description    : This file contains the driver code to be developed by
//                   the students of the 311 class.  See assignment details
//                   for additional information.
//
//   Author        : Trisha Mandal
//   Last Modified : October 5th, 2020
//

// Include Files
#include <string.h>

// Project Includes 
#include <sg_driver.h>

// Defines

//
// Global Data

//
// Functions

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

SG_Packet_Status serialize_sg_packet( SG_Node_ID loc, SG_Node_ID rem, SG_Block_ID blk, 
        SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq, char *data, 
        char *packet, size_t *plen ) {
            // using to indicate data
            char dataindicator = '0';
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
                dataindicator = '1';
            }
            else
                dataindicator = '0';

            memcpy(&packet[36],&dataindicator, sizeof(dataindicator));
            

            // code to add data block or not
            if(dataindicator == '1')
            {   
                memcpy(&packet[37],&data[0],1024);
                memcpy(&packet[1061],&magic,4);
                
            }
            else
            { 
                memcpy(&packet[37],&magic,4);
               
            }

            //logMessage(SGDriverLevel, "S: magic in packet = %u", *(uint32_t *) &packet[0]);
            //logMessage(SGDriverLevel, "S: loc in packet = %u", *(uint64_t *) &packet[4]);
            //logMessage(SGDriverLevel, "S: rem in packet = %u", *(uint64_t *) &packet[12]);
            //logMessage(SGDriverLevel, "S: blk in packet = %u", *(uint64_t *) &packet[20]);
            //logMessage(SGDriverLevel, "S: op in packet = %u", *(uint32_t *) &packet[28]);
            //logMessage(SGDriverLevel, "S: sseq in packet = %u", *(uint16_t *) &packet[32]);
            //logMessage(SGDriverLevel, "S: rseq in packet = %u", *(uint16_t *) &packet[34]);
            //logMessage(SGDriverLevel, "S: indicator in packet = %c", *(char *) &packet[36]);
            //logMessage(SGDriverLevel, "S: packet = %s", &packet[0]);
            
            // return if the packet status is good
            return SG_PACKT_OK;

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

SG_Packet_Status deserialize_sg_packet( SG_Node_ID *loc, SG_Node_ID *rem, SG_Block_ID *blk, 
        SG_System_OP *op, SG_SeqNum *sseq, SG_SeqNum *rseq, char *data, 
        char *packet, size_t plen ) {

            char buffer[plen];

            memcpy(buffer, packet, plen);

            //logMessage(SGDriverLevel, "D: magic in packet = %u", *(uint32_t *) &packet[0]);
            //logMessage(SGDriverLevel, "D: loc in packet = %u", *(uint64_t *) &packet[4]);
            //logMessage(SGDriverLevel, "D: rem in packet = %u", *(uint64_t *) &packet[12]);
            //logMessage(SGDriverLevel, "D: blk in packet = %u", *(uint64_t *) &packet[20]);
            //logMessage(SGDriverLevel, "D: op in packet = %u", *(uint32_t *) &packet[28]);
            //logMessage(SGDriverLevel, "D: sseq in packet = %u", *(uint16_t *) &packet[32]);
            //logMessage(SGDriverLevel, "D: rseq in packet = %u", *(uint16_t *) &packet[34]);
            //logMessage(SGDriverLevel, "D: indicator in packet = %c", *(char *) &packet[36]);

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
            if(buffer[36]== '1')
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
    
}
        
        
