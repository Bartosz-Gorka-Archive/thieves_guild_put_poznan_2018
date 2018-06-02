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
  clock += 1;

  // Fill data
  data[0] = clock;
  data[1] = message;

  // Send data with MPI action
  MPI_Send(&data, 2, MPI_INT, receiverID, tag, MPI_COMM_WORLD);

  // When enabled debug mode - show extra details
  if (debug_mode) {
    printf("[%05d][%02d][TAG: %03d] Send '%d' to process %d.\n",
           data[0], senderID, tag, data[1], receiverID);
  }

  // Unlock mutex
  pthread_mutex_unlock(&clock_mutex);
}

/*
 * Receive message from second monitor.
 * @param int &clock - Lamport clock's reference
 * @param int data[] - Array to insert received data
 * @param MPI_Status &status - Status to update
 * @param int tag - Message tag. Set -1 to receive all tags.
 * @param int receiverID - ID of receiver process
 * @param int senderID - ID of sender process
 */
void receive(int &clock, int data[], MPI_Status &status, int tag, int receiverID, int senderID) {
  // Receive data
  if(tag != -1) {
    MPI_Recv(data, 2, MPI_INT, senderID, tag, MPI_COMM_WORLD, &status);
  } else {
    MPI_Recv(data, 2, MPI_INT, senderID, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  }

  // Lock mutex to enable modify Lamport clock value
  pthread_mutex_lock(&clock_mutex);

  // Check timer value
  if(clock < data[0]) {
    clock = data[0] + 1;
  } else {
    clock += 1;
  }

  // When enabled debug mode - show extra details
  if (debug_mode) {
    printf("[%05d][%02d][TAG: %03d] Receive '%d' from process %d.\n",
           clock, receiverID, status.MPI_TAG, data[1], status.MPI_SOURCE);
  }

  // Unlock mutex
  pthread_mutex_unlock(&clock_mutex);
}

/*
 * Broadcast message to all another monitors.
 * @param int &clock - Lamport clock's reference to value (required to bump +1 value)
 * @param int message - Value to send
 * @param int tag - Message tag
 * @param int world_size - MPI_Comm_size value
 * @param int senderID - ID of sender process
 */
void broadcast(int &clock, int message, int tag, int world_size, int senderID) {
  // Loop to send message to another monitors
  for(int i = 0; i < world_size; i++) {
    // Skip send to self project
    if(i != senderID) {
      send(clock, message, tag, i, senderID);
    }
  }
}
