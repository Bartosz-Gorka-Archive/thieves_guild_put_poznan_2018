#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "tags.h"
#include "communication.h"

pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
bool debug_mode = true;
int lamport_clock = 0;

int main(int argc,char **argv)
{
    int size,rank;
    MPI_Init(&argc, &argv);

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    send(lamport_clock, 12, (232 * rank)+1, 0, rank);

    MPI_Finalize();
}
