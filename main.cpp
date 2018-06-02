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

// TODO function
void *receive_loop(void *thread) {
    while(1) {
      // TODO Receive
    }
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

    // std::vector<Request> vec;
    // vec.push_back(Request(3, 4));
    // vec.push_back(Request(3, 3));
    // vec.push_back(Request(3, 5));
    // vec.push_back(Request(1, 6));
    //
    // puts("Original vector");
    // for(int i = 0; i < vec.size(); ++i)
    //   printf("%d =>  [%d] [%d]\n", i, vec[i].time, vec[i].pid);
    //
    // sort_requests(vec);
    //
    // puts("After sort vector");
    // for(int i = 0; i < vec.size(); ++i)
    //   printf("%d =>  [%d] [%d]\n", i, vec[i].time, vec[i].pid);
    bool has_partner = false;
    if (rank == 0) {
      // Request friend
      bool decisions[size];
      for (int i = 0; i < size; i++) {
        decisions[i] = false;
      }
      decisions[rank] = true;

      broadcast(lamport_clock, 11, TAG_FIND_PARTNER, size, rank);

      int reveived_data[2];
      MPI_Status status;

      for (int i = 1; i < size; i++) {
        receive(lamport_clock, reveived_data, status, 12, rank, MPI_ANY_SOURCE);
        // printf("%d %d\n", status.MPI_SOURCE, reveived_data[0]);
        if (reveived_data[1] == 1) {
          decisions[status.MPI_SOURCE] = true;
        }
      }

      for (int i = 0; i < size; i++) {
        printf("%d %d\n", i, decisions[i]);
      }
    } else {
      // receive
      int reveived_data[2];
      MPI_Status status;
      receive(lamport_clock, reveived_data, status, TAG_FIND_PARTNER, rank, 0);
      send(lamport_clock, rank % 2, 12, 0, rank);
    }

    MPI_Finalize();
  }

  // if(rank == 0) {
  //   broadcast(lamport_clock, 12, TAG_FIND_PARTNER, size, 0);
  // }
  //
  // int reveived_data[2];
  // MPI_Status status;
  //
  // if(rank != 0) {
  //   receive(lamport_clock, reveived_data, status, TAG_FIND_PARTNER, rank, 0);
  // }
}
