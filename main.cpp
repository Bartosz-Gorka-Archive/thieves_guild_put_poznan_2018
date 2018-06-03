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

// Clock mutex to ensure correct modifications
pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
/*
 * If set `true` - show extra details inside program.
 * On production env should be set to `false`.
 */
bool debug_mode = false;
// Clock - default, start value = 0.
int lamport_clock = 0;
// P - Office capacity
int P = 0;
// D - Number of houses to be robbed
int D = 0;
// Current process myPID (process ID).
int myPID;
// Total process' number.
int total_process;
// Run program - flag to enable correct program flow
bool run_program = true;
// Has partner to robbery - default false (changed in program running)
bool has_partner = false;
// Vector with Requests to find partner
std::vector<Request> partner_queue;
// Mutex to ensure correct update partner ID / has_partner variable or partner list
pthread_mutex_t partner_mutex = PTHREAD_MUTEX_INITIALIZER;
// Received partnership responses
int received_friendship_response = 1; // One from myself
// Partner ID
int partnerID = -1;

/*
 * Set parameters inside project
 * @param int argc - Total number of parameters
 * @param char *argc[] - List with parameters
 * @return boolean - status, true when set done success, false if found errors with parameters
 */
bool set_parameters(int argc, char *argv[]) {
  // Required NAME P D ...
  if (argc >= 3) {
    P = atoi(argv[1]);
    D = atoi(argv[2]);

    // Validate possitive values inside variables
    if (D > 0 && P > 0) {
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
int check_single_position(pthread_mutex_t &mutex, std::vector<Request> &list, int pid) {
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

void check_both_positions(int positions[], pthread_mutex_t &mutex, std::vector<Request> &list, int PID, int secondPID) {
  positions[0] = INT_MAX;
  positions[1] = INT_MAX;

  pthread_mutex_lock(&mutex);
  for (size_t i = 0; i < partner_queue.size(); i++) {
    if (partner_queue[i].pid == PID) {
      positions[0] = i;
    }
    if (partner_queue[i].pid == secondPID) {
      positions[1] = i;
    }
  }
  pthread_mutex_unlock(&mutex);
}

/*
 * Want partner to robbery.
 */
void want_partner() {
  Request temp = Request(lamport_clock, myPID);
  // Lock, append request, sort, unlock
  pthread_mutex_lock(&partner_mutex);
  partner_queue.push_back(temp);
  sort_requests(partner_queue);
  pthread_mutex_unlock(&partner_mutex);
  // Broadcast find partner request
  broadcast(lamport_clock, temp.time, temp.time, TAG_FIND_PARTNER, total_process, myPID);
  // Wait until receive all confirmations
  while(received_friendship_response < total_process) {
    usleep(1000);
  }

  if (debug_mode) {
    printf("[%05d][%02d] Received all messages\n", lamport_clock, myPID);
  }

  // You received all confirmations but total process number is odd - ignore you (bye!)
  while(partnerID == -1) {
    // usleep(1000);
    sleep(1);
    printf("\tStill %d\n", myPID);
  }

  // if (debug_mode) {
    // Selected partner - go to robbery
    printf("[%05d][%02d] I have partner! Selected process %02d\n", lamport_clock, myPID, partnerID);
  // }
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
 * Function to MPI thread - create monitor process to receive messages in loop and update process state.
 */
void *receive_loop(void *thread) {
  // Run in loop until `run_program` set as true
  while(run_program) {
    // Status & data to receive function
    MPI_Status status;
    int data[3];
    int positions[2];
    // Receive message
    receive(lamport_clock, data, status, MPI_ANY_TAG, myPID, MPI_ANY_SOURCE);

    // Check status and do code
    switch (status.MPI_TAG) {
      case TAG_FIND_PARTNER: {
          insert_partner_request(data[2], status.MPI_SOURCE);
          check_both_positions(positions, partner_mutex, partner_queue, myPID, status.MPI_SOURCE);

          if (positions[0] > positions[1]) {
            send(lamport_clock, -1, -1, TAG_ACCEPT_PARTNER, status.MPI_SOURCE, myPID);
          } else {
            if (debug_mode) {
              printf("[%05d][%02d] Ignore TAG_FIND_PARTNER from %d (positions %d and %d)\n", lamport_clock, myPID, status.MPI_SOURCE, positions[0], positions[1]);
            }
          }

        // End case TAG_FIND_PARTNER
        break;
      }

      case TAG_ACCEPT_PARTNER: {
        // Mark response as received
        received_friendship_response++;
        // if (debug_mode) {
          printf("[%05d][%02d] AP - Received %d / %d\n", lamport_clock, myPID, received_friendship_response, total_process);
        // }

        if (received_friendship_response == total_process) {
          if (debug_mode) {
            printf("[%05d][%02d] Last friend response\n", lamport_clock, myPID);
          }

          // Remove me from list
          remove_from_friendship_queue(myPID);

          // Check list size
          pthread_mutex_lock(&partner_mutex);
          // If more process - select second and set as partner
          if (partner_queue.size() >= 1) {
            partnerID = partner_queue[0].pid;
          }
          pthread_mutex_unlock(&partner_mutex);

          // Send broadcast message
          broadcast(lamport_clock, partnerID, partnerID, TAG_SELECTED_PARTNER, total_process, myPID);
        }

        // End case TAG_ACCEPT_PARTNER
        break;
      }

      case TAG_SELECTED_PARTNER: {
        remove_from_friendship_queue(status.MPI_SOURCE);
        received_friendship_response++;
        // if (debug_mode) {
          printf("[%05d][%02d] SP - Received %d / %d\n", lamport_clock, myPID, received_friendship_response, total_process);
        // }

        if (received_friendship_response == total_process) {
          // Remove me from list
          remove_from_friendship_queue(myPID);

          // Someone take my process to robbery
          if (data[2] == myPID) {
            pthread_mutex_lock(&partner_mutex);
            partnerID = status.MPI_SOURCE;
            pthread_mutex_unlock(&partner_mutex);
          } else {
            // I must choose someone

            pthread_mutex_lock(&partner_mutex);
            if (partner_queue.size() >= 1) {
              partnerID = partner_queue[0].pid;
            }
            pthread_mutex_unlock(&partner_mutex);
          }
          // Send broadcast message
          broadcast(lamport_clock, partnerID, partnerID, TAG_SELECTED_PARTNER, total_process, myPID);
        }

        // End case TAG_SELECTED_PARTNER
        break;
      }

      default: {
        printf("[%05d][%02d][ERROR] Invalid tag '%d' from process %d.\n", lamport_clock, myPID, status.MPI_TAG, status.MPI_SOURCE);
        exit(1);
      }
    }
  }

  return 0;
}

int main(int argc,char **argv) {
  // stdout - disable bufforing
  setbuf(stdout, NULL);

  // Set program parameters
  if (!set_parameters(argc, argv)) {
    puts("[ERROR] You should start with NAME P D parameters (P and D greater than zero)");
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

    // 1. Find partner
    want_partner();

    // Has partner - can find house to robbery
    // TODO implementation

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
