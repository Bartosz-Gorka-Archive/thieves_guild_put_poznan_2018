#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>

#include "tags.h"
#include "utils.h"
#include "communication.h"

pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
bool debug_mode = true;
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

int main(int argc,char **argv) {
  // stdout - disable bufforing
  setbuf(stdout, NULL);

  // Set program parameters
  if (!set_parameters(argc, argv)) {
    puts("[ERROR] You should start with NAME P D parameters (P and D greater than zero)");
  } else {
    puts("[INFO] Parameters setup correct");
  }

  int size,rank;
  MPI_Init(&argc, &argv);
  // MPI_Comm_size( MPI_COMM_WORLD, &size );
  // MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  //
  // std::vector<Request> vec;
  // vec.push_back(Request(3, 4));
  // vec.push_back(Request(3, 3));
  // vec.push_back(Request(3, 5));
  // vec.push_back(Request(1, 6));
  //
  // for(int i = 0; i < vec.size(); ++i)
  //   printf("%d =>  [%d] [%d]\n", i, vec[i].time, vec[i].pid);
  //
  // sort_requests(vec);
  //
  // for(int i = 0; i < vec.size(); ++i)
  //   printf("%d =>  [%d] [%d]\n", i, vec[i].time, vec[i].pid);

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

  MPI_Finalize();
}
