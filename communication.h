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
 * @param int tag - Message tag
 * @param int receiverID - ID of receiver process
 * @param int senderID - ID of sender process
 */
void send(int &clock, int message, int tag, int receiverID, int senderID);

#endif
