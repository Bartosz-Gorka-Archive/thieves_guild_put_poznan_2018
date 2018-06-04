/*
 * Thieves' Guild project
 *  Poznań, May 2018
 *
 * communication.h
 *  Communication header file with declarated functions
 *  used to communication between two monitors.
 *
 * Authors:
 *  Jarosław Skrzypczak index. 127265 (@jarkendar)
 *  Bartosz Górka index. 127228 (@bartoszgorka)
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

/*
 * Debug mode - if true, show extra details about sent and received data.
 */
extern bool debug_mode;

/*
 * Send completed message to second monitor.
 * @param int &clock - Lamport clock's reference to value (required to bump +1 value)
 * @param int message - Value to send
 * @param int extra_message - Value to send
 * @param int tag - Message tag
 * @param int receiverID - ID of receiver process
 * @param int senderID - ID of sender process
 */
void send(int &clock, int message, int extra_message, int tag, int receiverID, int senderID);

/*
 * Receive message from second monitor.
 * @param int &clock - Lamport clock's reference
 * @param int data[] - Array to insert received data
 * @param MPI_Status &status - Status to update
 * @param int tag - Message tag. Set -1 to receive all tags.
 * @param int receiverID - ID of receiver process
 * @param int senderID - ID of sender process
 */
void receive(int &clock, int data[], MPI_Status &status, int tag, int receiverID, int senderID);

/*
 * Broadcast message to all another monitors.
 * @param int &clock - Lamport clock's reference to value (required to bump +1 value)
 * @param int message - Value to send
 * @param int extra_message - Value to send
 * @param int tag - Message tag
 * @param int world_size - MPI_Comm_size value
 * @param int senderID - ID of sender process
 */
void broadcast(int &clock, int message, int extra_message, int tag, int world_size, int senderID);

#endif
