/*
 * Thieves' Guild project
 *  Poznań, May 2018
 *
 * communication.cpp
 *  Communication implementation file with functions
 *  used to communication between two monitors.
 *
 * Authors:
 *  Jarosław Skrzypczak index. 127265 (@jarkendar)
 *  Bartosz Górka index. 127228 (@bartoszgorka)
 */

#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include "communication.h"

/*
 * Mutex assigned to Lamport clock.
 * Used from main file. Required to block unexpected modifications.
 */
extern pthread_mutex_t clock_mutex;

/*
 * Send completed message to second monitor.
 * @param int &clock - Lamport clock's reference to value (required to bump +1 value)
 * @param int message - Value to send
 * @param int tag - Message tag
 * @param int receiverID - ID of receiver process
 * @param int senderID - ID of sender process
 */
void send(int &clock, int message, int tag, int receiverID, int senderID) {
  // Lock mutex to enable modify Lamport clock value
  pthread_mutex_lock(&clock_mutex);

  // Prepare data to send
  int data[2];

  // Bump clock value
  ++clock;

  // Fill data
  data[0] = clock;
  data[1] = message;

  // Send data with MPI action
  MPI_Send(&data, 2, MPI_INT, receiverID, tag, MPI_COMM_WORLD);

  // When enabled debug mode - show extra details
  if (debug_mode) {
    printf("[TIMER: %05d][PID: %02d][TAG: %03d] Send '%d' to process %d.\n",
           data[0], senderID, tag, data[1], receiverID);
  }

  // Unlock mutex
  pthread_mutex_unlock(&clock_mutex);
}
