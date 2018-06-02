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

/*
 * Clock mutex to ensure correct modifications
 */
pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
/*
 * If set `true` - show extra details inside program.
 * On production env should be set to `false`.
 */
bool debug_mode = true;
/*
 * Clock - default, start value = 0.
 */
int lamport_clock = 0;
/*
 * P - Office capacity
 */
int P = 0;
/*
 * D - Number of houses to be robbed
 */
int D = 0;
/*
 * Current process rank (process ID).
 */
int rank;
/*
 * Total process' number.
 */
int size;

bool run_program = true;
bool has_partner = false;
int partnerID = 0;
std::vector<Request> partner_queue;
pthread_mutex_t partner_mutex = PTHREAD_MUTEX_INITIALIZER;

int received_friendship_response = 0;
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

void increment_received_friendship_response() {
  pthread_mutex_lock(&received_friendship_mutex);
  received_friendship_response++;
  pthread_mutex_unlock(&received_friendship_mutex);
}

void add_friendship_response(int timer_value, int senderID) {
  pthread_mutex_lock(&partner_mutex);
  Request request = Request(timer_value, senderID);
  partner_queue.push_back(request);
  pthread_mutex_unlock(&partner_mutex);
}

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

// TODO function
void *receive_loop(void *thread) {
  while(run_program) {
    MPI_Status status;
    int data[2];
    receive(lamport_clock, data, status, MPI_ANY_TAG, rank, MPI_ANY_SOURCE);

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

        // End case TAG_SELECTED_PARTNER
        break;

      default:
        printf("[%d][%d][ERROR] Invalid tag '%d' from process %d.\n", lamport_clock, rank, status.MPI_TAG, status.MPI_SOURCE);
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
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Random seed depends on process rank
    srand(rank);

    // Barier to start calculations
    printf("[INFO] PROCESS %d READY\n", rank);
    MPI_Barrier(MPI_COMM_WORLD);

    // 1. Find partner
    if (rank == 0) {
      broadcast(lamport_clock, rank, TAG_FIND_PARTNER, size, rank);
      add_friendship_response(lamport_clock, rank);

      sleep(3);
      sort_requests(partner_queue);
      for (int i = 0; i < partner_queue.size(); i++) {
        printf("%d %d\n", partner_queue[i].time, partner_queue[i].pid);
      }

      send(lamport_clock, 1, TAG_SELECTED_PARTNER, 0, 0);
      sleep(3);
      sort_requests(partner_queue);
      for (int i = 0; i < partner_queue.size(); i++) {
        printf("%d %d\n", partner_queue[i].time, partner_queue[i].pid);
      }
    }

    // Set end calculations
    run_program = false;

    // Sleep to ensure all threads refresh local reference to `run_program` variable
    sleep(10);

    // Finalize MPI
    MPI_Finalize();

    // End without errors
    return 0;
  }
}
