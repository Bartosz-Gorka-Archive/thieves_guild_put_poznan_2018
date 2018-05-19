#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#define TAG_FIND_COLABORATOR 10
#define TAG_FIND_TABLE 20

MPI_Datatype MPI_PAKIET_T;

typedef struct message_s {
  int pid; // Process PID
  long time_value; // Lamport's Timer value
  int data; // Message value
} message;

int main(int argc,char **argv)
{
    int size,rank;
    MPI_Init(&argc, &argv);

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    const int nitems=3;
     int          blocklengths[3] = {1,2,1};
     MPI_Datatype types[3] = {MPI_INT, MPI_LONG, MPI_INT};
     MPI_Datatype message_s;
     MPI_Aint     offsets[3];

     offsets[0] = offsetof(message, pid);
     offsets[1] = offsetof(message, time_value);
     offsets[2] = offsetof(message, data);

     MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_PAKIET_T);
     MPI_Type_commit(&MPI_PAKIET_T);

     printf("Grubo siekane, wuff, wuff   \n");

    MPI_Finalize();
}
