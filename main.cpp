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
bool debug_mode = true;
// Clock - default, start value = 0.
int lamport_clock = 0;
// P - Office capacity
int P = 0;
// D - Number of houses to be robbed
int D = 0;
// Current process rank (process ID).
int rank;
// Total process' number.
int total_process;
// Run program - flag to enable correct program flow
bool run_program = true;
// Has partner to robbery - default false (changed in program running)
bool has_partner = false;
// Partner ID (to robbery) - set by myself process or first in queue
int partnerID = 0;
// Vector with Requests to find partner
std::vector<Request> partner_queue;
// Mutex to ensure correct update partner ID / has_partner variable or partner list
pthread_mutex_t partner_mutex = PTHREAD_MUTEX_INITIALIZER;
// Received friendship messages
int received_friendship_response = 0;
// Mutex to modify counter about received messages (for friendship)
pthread_mutex_t received_friendship_mutex = PTHREAD_MUTEX_INITIALIZER;

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
 * Increment received friendship responses'.
 * Incrementation in secure way - with use mutex `received_friendship_mutex`.
 */
void increment_received_friendship_response() {
  pthread_mutex_lock(&received_friendship_mutex);
  received_friendship_response++;
  pthread_mutex_unlock(&received_friendship_mutex);
}

/*
 * Add new friendship response to queue.
 * Function use mutex `partner_mutex` to secure update list.
 * @param int timer_value - Received time
 * @param int senderID - Sender process ID
 */
void add_friendship_response(int timer_value, int senderID) {
  pthread_mutex_lock(&partner_mutex);
  Request request = Request(timer_value, senderID);
  partner_queue.push_back(request);
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
 * Check process position on list of Request.
 * @param pthread_mutex_t &mutex - mutex to lock before and unlock after check
 * @param std::vector<Request> &list - vector with list of Request
 * @param int pid - Process ID (to check)
 * @return int position - Process position (default 2 to block access to critical section)
 */
int check_position(pthread_mutex_t &mutex, std::vector<Request> &list, int pid) {
  int position = 2;
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
 * Want partner
 */
void want_partner() {
  // Send request to all process (skip myself)
  broadcast(lamport_clock, rank, TAG_FIND_PARTNER, total_process, rank);
  // Append myself to list
  add_friendship_response(lamport_clock, rank);
  // Wait until correct number of response
  // TODO mutex?
  while(received_friendship_response + 1 < total_process) {
    printf("[%05d][%02d][WANT PARTNER] Received ACK %d / %d\n", lamport_clock, rank, received_friendship_response + 1, total_process);
    usleep(100);
  }
  printf("[%05d][%02d] Sort partners request list\n", lamport_clock, rank);

  // Sort list
  sort_requests(partner_queue);

  // Wait until first or second on list
  int position = 2;
  while((position = check_position(partner_mutex, partner_queue, rank)) > 1) {
    printf("[%05d][%02d] Partner - current position %d\n", lamport_clock, rank, position);
    usleep(100);
  }

  // First position
  if (position == 0) {
    printf("[%05d][%02d] First position on list\n", lamport_clock, rank);
    pthread_mutex_lock(&partner_mutex);
    if (partner_queue.size() > 1) {
      partnerID = partner_queue[1].pid;
      has_partner = true;
      broadcast(lamport_clock, partnerID, TAG_SELECTED_PARTNER, total_process, rank);
    }
    pthread_mutex_unlock(&partner_mutex);
  }

  printf("[%05d][%02d] Set variables: Partner ID %d - flag %d\n", lamport_clock, rank, partnerID, has_partner);
}

/*
 * Function to MPI thread - create monitor process to receive messages in loop and update process state.
 */
void *receive_loop(void *thread) {
  // Run in loop until `run_program` set as true
  while(run_program) {
    // Status & data to receive function
    MPI_Status status;
    int data[2];
    // Receive message
    receive(lamport_clock, data, status, MPI_ANY_TAG, rank, MPI_ANY_SOURCE);

    // Check status and do code
    switch (status.MPI_TAG) {
      case TAG_FIND_PARTNER:
        // Check partner status
        pthread_mutex_lock(&partner_mutex);
        if (has_partner) {
          pthread_mutex_unlock(&partner_mutex);
          send(lamport_clock, partnerID, TAG_ALREADY_PAIRED, status.MPI_SOURCE, rank);
        } else {
          pthread_mutex_unlock(&partner_mutex);
          send(lamport_clock, rank, TAG_ACCEPT_PARTNER, status.MPI_SOURCE, rank);
        }

        // End case TAG_FIND_PARTNER
        break;

      case TAG_ALREADY_PAIRED:
        // Add +1 to received response about friendship
        increment_received_friendship_response();

        // End case TAG_ALREADY_PAIRED
        break;

      case TAG_ACCEPT_PARTNER:
        // Add +1 to received response about friendship
        increment_received_friendship_response();
        add_friendship_response(data[0], status.MPI_SOURCE);

        // End case TAG_ACCEPT_PARTNER
        break;

      case TAG_SELECTED_PARTNER:
        // Delete sender and his partner from queue
        remove_from_friendship_queue(data[1]);
        remove_from_friendship_queue(status.MPI_SOURCE);

        // If my PID selected as data[1] (first process partner ID) - update variables
        if (data[1] == rank) {
          pthread_mutex_lock(&partner_mutex);
          printf("[%05d][%02d] Set partnerID to %d\n", lamport_clock, rank, status.MPI_SOURCE);
          partnerID = status.MPI_SOURCE;
          has_partner = true;
          pthread_mutex_unlock(&partner_mutex);
        }

        // End case TAG_SELECTED_PARTNER
        break;

      default:
        printf("[%05d][%02d][ERROR] Invalid tag '%d' from process %d.\n", lamport_clock, rank, status.MPI_TAG, status.MPI_SOURCE);
        exit(1);
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
    printf("[INFO] Parameters setup correct\n");

    // Check MPI threads
    enable_thread(&argc, &argv);

    // Create new thread - run for receive messages in loop (as monitor)
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, receive_loop, 0);

    // Get process ID and total process number
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &total_process);

    // Random seed depends on process rank
    srand(rank);

    // Barier to start calculations
    printf("[%05d][%02d][INFO] PROCESS %d READY\n", lamport_clock, rank, rank);
    MPI_Barrier(MPI_COMM_WORLD);

    // 1. Find partner
    if (rank == 0) {
      // Find partner
      want_partner();

      // Wait until set partner
      // TODO - mutex here?
      while(!has_partner) {
        printf("[%05d][%02d] Still no partner - loop\n", lamport_clock, rank);
        usleep(100);
      }

      // Has partner - can find house to robbery
      // TODO implementation
    }

    // Set end calculations
    run_program = false;

    // Sleep to ensure all threads refresh local reference to `run_program` variable
    printf("[%05d][%02d][INFO] Sleep 10 seconds to enable correct finish program\n", lamport_clock, rank);
    sleep(10);

    // Finalize MPI
    MPI_Finalize();

    // End without errors
    return 0;
  }
}
