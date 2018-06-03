/*
 * Thieves' Guild project
 *  Poznań, May - Juny 2018
 *
 * main.cpp
 *  Most important file - main functions inside Thieves' Guild project
 *
 * Authors:
 *  Jarosław Skrzypczak index. 127265 (@jarkendar)
 *  Bartosz Górka index. 127228 (@bartoszgorka)
 */

// System library
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include <limits.h>

// Own library
#include "tags.h"
#include "utils.h"
#include "communication.h"

// Debug mode - if set `true` - show all logs.
// On `false` show only most important logs
bool debug_mode = false;
// My clock value
int lamport_clock = 0;
// Number of houses
int D = 0;
// My process ID - assigned by MPI
int myPID;
// Total process inside world - assigned by MPI
int total_process;
// Important parameter to enable correct finish threads - without extra variable can raise errors
bool run_program = true;

// Vector with requests - requests to access to find partner critical section
std::vector<Request> partner_queue;

// Mutex - clock
pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex - partner queue
pthread_mutex_t partner_mutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex - partner response number (for secure update variable because is it possible to send multi messages)
pthread_mutex_t partner_response_mutex = PTHREAD_MUTEX_INITIALIZER;

// Number of received ACK to friend find critical section
// Default 1 - your own ACK ;)
int received_friendship_response = 1;
// Partner ID, default -1 to correct check this inside functions
int partnerID = -1;
// Start time - clock value in access to partner critical section
int start_find_partner_time = INT_MAX;
// Selected house ID
int houseID = -1;

/*
 * Debug function to show currect state of friend queue
 */
void show_friend_queue() {
  pthread_mutex_lock(&partner_mutex);
  for (size_t i = 0; i < partner_queue.size(); i++) {
    printf("\t [%d] %lu => %d\n", myPID, i+1, partner_queue[i].pid);
  }
  pthread_mutex_unlock(&partner_mutex);
}

/*
 * Set parameters inside project
 * @param int argc - Total number of parameters
 * @param char *argc[] - List with parameters
 * @return boolean - status, true when set done success, false if found errors with parameters
 */
bool set_parameters(int argc, char *argv[]) {
  // Required NAME D ...
  if (argc >= 2) {
    D = atoi(argv[1]);

    // Validate possitive value inside variable
    if (D > 0) {
      return true;
    }
  }

  // Any error - false (error found)
  return false;
}

/*
 * Enable threads inside project, check MPI_THREAD_MULTIPLE status.
 * When not supported - exit program.
 * @param int *argc - Reference to number of parameters
 * @param char **argv - Reference to parameters
 */
void enable_thread(int *argc, char ***argv) {
  // Check support
  int status = 0;
  MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &status);

  // Debug mode? - Show current MPI support level
  if (debug_mode) {
    switch (status) {
      case MPI_THREAD_SINGLE:
        printf("[INFO] Thread level supported: MPI_THREAD_SINGLE\n");
        break;
      case MPI_THREAD_FUNNELED:
        printf("[INFO] Thread level supported: MPI_THREAD_FUNNELED\n");
        break;
      case MPI_THREAD_SERIALIZED:
        printf("[INFO] Thread level supported: MPI_THREAD_SERIALIZED\n");
        break;
      case MPI_THREAD_MULTIPLE:
        printf("[INFO] Thread level supported: MPI_THREAD_MULTIPLE\n");
        break;
      default:
        printf("[INFO] Thread level supported: UNRECOGNIZED\n");
        exit(EXIT_FAILURE);
    }
  }

  // When thread not supported - exit
  if (status != MPI_THREAD_MULTIPLE) {
    fprintf(stderr, "[ERROR] There is not enough support for threads - I'm leaving!\n");
    MPI_Finalize();
    exit(EXIT_FAILURE);
  }
}

/*
 * Check process position on list of Request.
 * @param pthread_mutex_t &mutex - mutex to lock before and unlock after check
 * @param std::vector<Request> &list - vector with list of Request
 * @param int pid - Process ID (to check)
 * @return int position - Process position (default 2 to block access to critical section)
 */
int check_position(pthread_mutex_t &mutex, std::vector<Request> &list, int pid) {
  int position = INT_MAX;
  pthread_mutex_lock(&mutex);
  for (size_t i = 0; i < partner_queue.size(); i++) {
    if (partner_queue[i].pid == pid) {
      position = i;
      break;
    }
  }
  pthread_mutex_unlock(&mutex);
  return position;
}

/*
 * Remove request from friendship queue.
 * For secure update - with mutex `partner_mutex`.
 * [Warning] In hiden way update `partner_queue`.
 * @param int senderID - sender process ID
 */
void remove_from_friendship_queue(int senderID) {
  pthread_mutex_lock(&partner_mutex);
  for (size_t i = 0; i < partner_queue.size(); i++) {
    if (partner_queue[i].pid == senderID) {
      partner_queue.erase(partner_queue.begin() + i);
      break;
    }
  }
  pthread_mutex_unlock(&partner_mutex);
}

/*
 * Want partner to robbery.
 */
void want_partner() {
  Request temp = Request(lamport_clock, myPID);
  // Lock, append request, sort, unlock
  pthread_mutex_lock(&partner_mutex);
  partner_queue.push_back(temp);
  start_find_partner_time = temp.time;
  pthread_mutex_unlock(&partner_mutex);
  // Broadcast find partner request
  broadcast(lamport_clock, temp.time, temp.time, TAG_FIND_PARTNER, total_process, myPID);
  // Wait until receive all confirmations
  while(received_friendship_response < total_process) {
    usleep(1000);
  }

  printf("[%05d][%02d] Received all messages\n", lamport_clock, myPID);

  // Sort requests
  sort_requests(partner_queue);

  // You received all confirmations but total process number is odd - ignore you (bye!)
  while(partnerID == -1) {
    // usleep(1000);
    if (check_position(partner_mutex, partner_queue, myPID) == 0) {
      pthread_mutex_lock(&partner_mutex);
      // On list my process and someone else
      if (partner_queue.size() >= 1) {
        partnerID = partner_queue[1].pid;
        pthread_mutex_unlock(&partner_mutex);

        remove_from_friendship_queue(myPID);
        broadcast(lamport_clock, partnerID, partnerID, TAG_SELECTED_PARTNER, total_process, myPID);
      } else {
        pthread_mutex_unlock(&partner_mutex);
      }
    }

    sleep(1);
    // printf("\tNo partner for %d\n", myPID);
  }

  // Selected partner - go to robbery
  printf("[%05d][%02d] I have partner! Selected process %02d\n", lamport_clock, myPID, partnerID);
}

/*
 * Insert new request to partner vector.
 * For reduce security issues - also sort requests list
 * @param int time - Time
 * @param int pid - Process ID
 */
void insert_partner_request(int time, int pid) {
  pthread_mutex_lock(&partner_mutex);
  partner_queue.push_back(Request(time, pid));
  sort_requests(partner_queue);
  pthread_mutex_unlock(&partner_mutex);
}

/*
 * Function to MPI thread - create monitor process to receive messages in loop and update process state.
 */
void *receive_loop(void *thread) {
  // Run in loop until `run_program` set as true
  while(run_program) {
    // Status & data to receive function
    MPI_Status status;
    int data[3];
    // Receive message
    receive(lamport_clock, data, status, MPI_ANY_TAG, myPID, MPI_ANY_SOURCE);

    // Check status and do code
    switch (status.MPI_TAG) {
      case TAG_FIND_PARTNER: {
        // Append request with orignal time to queue
        insert_partner_request(data[2], status.MPI_SOURCE);
        // Send response - note sender about receive message
        send(lamport_clock, start_find_partner_time, start_find_partner_time, TAG_RESPONSE_PARTNER, status.MPI_SOURCE, myPID);

        // End case TAG_FIND_PARTNER
        break;
      }

      case TAG_RESPONSE_PARTNER: {
        // Increment received response counter - for security with mutex
        pthread_mutex_lock(&partner_response_mutex);
        received_friendship_response++;
        pthread_mutex_unlock(&partner_response_mutex);

        // End case TAG_RESPONSE_PARTNER
        break;
      }

      case TAG_SELECTED_PARTNER: {
        // I was chosen!
        if (data[1] == myPID) {
          // My partner - sender
          partnerID = status.MPI_SOURCE;

          // Remove his request and my request from queue
          remove_from_friendship_queue(myPID);
          remove_from_friendship_queue(status.MPI_SOURCE);
          // Broadcast to all process - exit from section
          broadcast(lamport_clock, myPID, myPID, TAG_SELECTED_PARTNER, total_process, myPID);
        } else {
          // Just remove sender's request from queue
          remove_from_friendship_queue(status.MPI_SOURCE);
        }

        // End case TAG_SELECTED_PARTNER
        break;
      }

      default: {
        // Default - raise error because received not supported TAG inside message
        printf("[%05d][%02d][ERROR] Invalid tag '%d' from process %d.\n", lamport_clock, myPID, status.MPI_TAG, status.MPI_SOURCE);
        exit(1);
      }
    }
  }

  return 0;
}

void want_house() {
  // TODO
}

void robbery() {
  // Show message
  printf("[%05d][%02d] With %02d visit house %d\n", lamport_clock, myPID, partnerID, houseID);
  // Sleep random time
  int sleep_time = (rand() % 4) + 1;
  printf("[%05d][%02d] Sleep %d\n", lamport_clock, myPID, sleep_time);
  sleep(sleep_time);
}

int main(int argc,char **argv) {
  // stdout - disable bufforing
  setbuf(stdout, NULL);

  // Set program parameters
  if (!set_parameters(argc, argv)) {
    puts("[ERROR] You should start with NAME D parameters (D greater than zero)");
  } else {
    // Parameters setup with success
    if (debug_mode) {
      printf("[INFO] Parameters setup correct\n");
    }

    // Check MPI threads
    enable_thread(&argc, &argv);

    // Create new thread - run for receive messages in loop (as monitor)
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, receive_loop, 0);

    // Get process ID and total process number
    MPI_Comm_rank(MPI_COMM_WORLD, &myPID);
    MPI_Comm_size(MPI_COMM_WORLD, &total_process);

    // Random seed depends on process myPID
    srand(myPID);

    // Barier to start calculations
    if (debug_mode) {
      printf("[%05d][%02d][INFO] PROCESS %d READY\n", lamport_clock, myPID, myPID);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // while(1) {
      // init_variables();
      // 1. Find partner
      want_partner();

      // 2. Find house to robbery
      want_house();

      // 3. Robbery
      robbery();
    // }

    // Set end calculations
    run_program = false;

    // Sleep to ensure all threads refresh local reference to `run_program` variable
    if (debug_mode) {
      printf("[%05d][%02d][INFO] Sleep 10 seconds to enable correct finish program\n", lamport_clock, myPID);
    }
    sleep(10);

    // Finalize MPI
    MPI_Finalize();

    // End without errors
    return 0;
  }
}
