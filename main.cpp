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
// Maximum process inside saloon
int P = 0;
// My process ID - assigned by MPI
int myPID;
// Total process inside world - assigned by MPI
int total_process;
// Important parameter to enable correct finish threads - without extra variable can raise errors
bool run_program = true;
// Already in saloon
bool is_in_saloon = false;

// Vector with requests - requests to access to find partner critical section
std::vector<Request> partner_queue;
// Vector with requests to access saloon's critical section
std::vector<Request> saloon_queue;
// List of houseIDs to return
std::vector<int> houses_to_return_list;
// Array with lists of requests to all houses
std::vector<Request> *houses_vec;

// Mutex - clock
pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex - partner queue
pthread_mutex_t partner_mutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex - partner response number (for secure update variable because is it possible to send multi messages)
pthread_mutex_t partner_response_mutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex arrays for houses
std::vector<pthread_mutex_t> houses_mutex;
// Single mutex for access to table
pthread_mutex_t houses_array_mutex = PTHREAD_MUTEX_INITIALIZER;
// Saloon mutex
pthread_mutex_t saloon_mutex = PTHREAD_MUTEX_INITIALIZER;
// Houses to return mutex
pthread_mutex_t houses_to_return_mutex = PTHREAD_MUTEX_INITIALIZER;

// Number of received ACK to friend find critical section
// Default 1 - your own ACK ;)
int received_friendship_response = 1;
int received_saloon_ack = 1;
// Partner ID, default -1 to correct check this inside functions
int partnerID = -1;
// Selected house ID
int houseID = -1;
// Array with responses (counters) about houses
int *houses_responses_array;
// Master / slave flag
bool master = false;
// Iteration counter
int iteration = 0;

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
  if (argc >= 3) {
    D = atoi(argv[1]);
    P = atoi(argv[2]);

    // Validate possitive value inside variable
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
int check_position(pthread_mutex_t &mutex, std::vector<Request> &list, int pid) {
  int position = INT_MAX;
  pthread_mutex_lock(&mutex);
  for (size_t i = 0; i < list.size(); i++) {
    if (list[i].pid == pid) {
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
 * Remove request from saloon queue.
 * For secure update - with mutex `saloon_mutex`.
 * [Warning] In hiden way update `saloon_queue`.
 * @param int senderID - sender process ID
 */
void remove_from_saloon_queue(int senderID) {
  pthread_mutex_lock(&saloon_mutex);
  for (size_t i = 0; i < saloon_queue.size(); i++) {
    if (saloon_queue[i].pid == senderID) {
      saloon_queue.erase(saloon_queue.begin() + i);
      break;
    }
  }
  pthread_mutex_unlock(&saloon_mutex);
}

/*
 * Want partner to robbery.
 */
void want_partner() {
  Request temp = Request(lamport_clock, myPID);
  // Lock, append request, sort, unlock
  pthread_mutex_lock(&partner_mutex);
  partner_queue.push_back(temp);
  received_friendship_response = 1;
  pthread_mutex_unlock(&partner_mutex);
  // Broadcast find partner request
  broadcast(lamport_clock, iteration, temp.time, TAG_FIND_PARTNER, total_process, myPID);
  // Wait until receive all confirmations
  while(received_friendship_response < total_process) {
    usleep(1000);
  }

  printf("[%05d][%02d] Received all messages\n", lamport_clock, myPID);

  // Sort requests
  pthread_mutex_lock(&partner_mutex);
  sort_requests(partner_queue);
  pthread_mutex_unlock(&partner_mutex);

  // You received all confirmations but total process number is odd - ignore you (bye!)
  while(partnerID == -1) {
    if (check_position(partner_mutex, partner_queue, myPID) == 1) {
      pthread_mutex_lock(&partner_mutex);
      // On list my process and someone else
      if (partner_queue.size() >= 1) {
        // Set master status and select partner
        master = true;
        partnerID = partner_queue[0].pid;
        pthread_mutex_unlock(&partner_mutex);

        remove_from_friendship_queue(myPID);
        broadcast(lamport_clock, iteration, partnerID, TAG_SELECTED_PARTNER, total_process, myPID);
      } else {
        pthread_mutex_unlock(&partner_mutex);
      }
    }

    sleep(1);
  }

  // Selected partner - go to robbery
  printf("[%05d][PID: %02d][IT: %02d] I have partner! Selected process %02d\n", lamport_clock, myPID, iteration, partnerID);
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
 * Insert new request to saloon vector.
 * For reduce security issues - also sort requests list
 * @param int time - Time
 * @param int pid - Process ID
 */
void insert_saloon_request(int time, int pid) {
  pthread_mutex_lock(&saloon_mutex);
  saloon_queue.push_back(Request(time, pid));
  sort_requests(saloon_queue);
  pthread_mutex_unlock(&saloon_mutex);
}

/*
 * Insert new request to house vector.
 * For reduce security issues - also sort requests list
 * @param int ID - selected House
 * @param int time - Time
 * @param int pid - Process ID
 */
void insert_house_request(int ID, int time, int pid) {
  pthread_mutex_lock(&houses_mutex[ID]);
  houses_vec[ID].push_back(Request(time, pid));
  sort_requests(houses_vec[ID]);
  pthread_mutex_unlock(&houses_mutex[ID]);
}

/*
 * Debug function to show current status of all houses queues
 */
void show_house_queues() {
  for (int i = 0; i < D; i++) {
    pthread_mutex_lock(&houses_mutex[i]);
    for (size_t k = 0; k < houses_vec[i].size(); k++) {
      printf("\t [%d] List %d | %lu => %d\n", myPID, i, k+1, houses_vec[i][k].pid);
    }
    pthread_mutex_unlock(&houses_mutex[i]);
  }
}

/*
 * Security (with lock & unclock mutex) update houses_responses_array.
 * @param int ID - selected House ID
 */
void increment_houses_counter(int ID) {
  pthread_mutex_lock(&houses_array_mutex);
  houses_responses_array[ID] += 1;
  pthread_mutex_unlock(&houses_array_mutex);
}

/*
 * Remove request from single house queue
 * @param int selectedHouseID - House ID (queue ID)
 * @param int senderID - Sender process ID
 */
void remove_from_single_house_queues(int selectedHouseID, int senderID) {
  pthread_mutex_lock(&houses_mutex[selectedHouseID]);
  for (size_t j = 0; j < houses_vec[selectedHouseID].size(); j++) {
    if (houses_vec[selectedHouseID][j].pid == senderID) {
      houses_vec[selectedHouseID].erase(houses_vec[selectedHouseID].begin() + j);
      break;
    }
  }
  pthread_mutex_unlock(&houses_mutex[selectedHouseID]);
}

void *release_assigned_houses(void *thread) {
  // Run in loop until `run_program` set as true
  while(run_program) {
    int sleep_time = (rand() % 100000) + 30000;

    pthread_mutex_lock(&houses_to_return_mutex);
    if (!houses_to_return_list.empty()) {
      int selectedID = houses_to_return_list[0];
      printf("[%05d][%02d] Release house %02d\n", lamport_clock, myPID, selectedID);
      remove_from_single_house_queues(selectedID, myPID);
      houses_to_return_list.erase(houses_to_return_list.begin());
      broadcast(lamport_clock, selectedID, selectedID, TAG_HOUSE_EXIT, total_process, myPID);
    }
    pthread_mutex_unlock(&houses_to_return_mutex);

    usleep(sleep_time);
  }
  return 0;
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
        send(lamport_clock, iteration, myPID, TAG_RESPONSE_PARTNER, status.MPI_SOURCE, myPID);

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
        if (data[2] == myPID) {
          // My partner - sender
          master = false;
          partnerID = status.MPI_SOURCE;

          // Remove his request and my request from queue
          remove_from_friendship_queue(myPID);
          remove_from_friendship_queue(status.MPI_SOURCE);
          // Broadcast to all process - exit from section
          broadcast(lamport_clock, iteration, myPID, TAG_SELECTED_PARTNER, total_process, myPID);
        } else {
          // Just remove sender's request from queue
          remove_from_friendship_queue(status.MPI_SOURCE);
        }

        // End case TAG_SELECTED_PARTNER
        break;
      }

      case TAG_FINISH_PARTNERSHIP: {
        houseID = -1;

        // End case TAG_FINISH_PARTNERSHIP
        break;
      }

      case TAG_HOUSE_REQUEST: {
        // Append request with orignal time to queue
        insert_house_request(data[1], data[2], status.MPI_SOURCE);
        // Send response - note sender about receive message
        send(lamport_clock, data[1], data[1], TAG_RESPONSE_HOUSE, status.MPI_SOURCE, myPID);

        // End case TAG_HOUSE_REQUEST
        break;
      }

      case TAG_RESPONSE_HOUSE: {
        increment_houses_counter(data[1]);
        // End case TAG_RESPONSE_HOUSE
        break;
      }

      case TAG_SELECT_HOUSE: {
        // Message from my partner (master) - save details about selected house
        if (status.MPI_SOURCE == partnerID) {
          houseID = data[1];
        }

        // End case TAG_SELECT_HOUSE
        break;
      }

      case TAG_HOUSE_EXIT: {
        // Remove request
        remove_from_single_house_queues(data[1], status.MPI_SOURCE);

        // End case TAG_HOUSE_EXIT
        break;
      }

      case TAG_ENTER_SALOON: {
        insert_saloon_request(data[2], status.MPI_SOURCE);
        send(lamport_clock, iteration, myPID, TAG_CONFIRM_SALOON, status.MPI_SOURCE, myPID);

        // End case TAG_ENTER_SALOON
        break;
      }

      case TAG_CONFIRM_SALOON: {
        pthread_mutex_lock(&saloon_mutex);
        received_saloon_ack++;
        pthread_mutex_unlock(&saloon_mutex);

        // End case TAG_CONFIRM_SALOON
        break;
      }

      case TAG_RELEASE_SALOON: {
        remove_from_saloon_queue(status.MPI_SOURCE);

        if (status.MPI_SOURCE == partnerID) {
          is_in_saloon = false;
        }

        // End case TAG_RELEASE_SALOON
        break;
      }

      case TAG_IN_SALOON: {
        if (status.MPI_SOURCE == partnerID) {
          is_in_saloon = true;
        }

        // End case TAG_IN_SALOON
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

bool checkAlreadyHasHouse(int ID) {
  for (size_t i = 0; i < houses_to_return_list.size(); i++) {
    if (houses_to_return_list[i] == ID) {
      return true;
    }
  }
  return false;
}

/*
 * Want access to house
 */
void want_house() {
  // Master can access to critical sections
  if (master) {
    pthread_mutex_lock(&houses_to_return_mutex);
    for (int i = 0; i < D; i++) {
      if(!checkAlreadyHasHouse(i)) {
        Request temp = Request(lamport_clock, myPID);
        // Lock, append request, sort, unlock
        pthread_mutex_lock(&houses_mutex[i]);
        houses_vec[i].push_back(temp);
        sort_requests(houses_vec[i]);
        pthread_mutex_unlock(&houses_mutex[i]);

        // Broadcast find available house
        broadcast(lamport_clock, i, temp.time, TAG_HOUSE_REQUEST, total_process, myPID);
      }
    }
    pthread_mutex_unlock(&houses_to_return_mutex);

    // Wait until receive all confirmations in minimum one critical section
    bool wait = true;
    do {
      usleep(1000);
      pthread_mutex_lock(&houses_array_mutex);
      for (int i = 0; i < D; i++) {
        if(houses_responses_array[i] == total_process) {
          wait = false;
          break;
        }
      }
      pthread_mutex_unlock(&houses_array_mutex);
    } while(wait);

    if (debug_mode) {
      printf("[%05d][%02d] Received all messages in one of house queue\n", lamport_clock, myPID);
    }

    // For security also again sort vectors
    for (int i = 0; i < D; i++) {
      pthread_mutex_lock(&houses_mutex[i]);
      sort_requests(houses_vec[i]);
      pthread_mutex_unlock(&houses_mutex[i]);
    }

    // Check can select one house
    bool notSelected = true;
    do {
      pthread_mutex_lock(&houses_array_mutex);
      for (int i = 0; i < D; i++) {
        if(houses_responses_array[i] == total_process && check_position(houses_mutex[i], houses_vec[i], myPID) == 0) {
          if (debug_mode) {
            printf("[%05d][%02d] Can full access to %02d house\n", lamport_clock, myPID, i);
          }
          houseID = i;
          notSelected = false;
          send(lamport_clock, houseID, houseID, TAG_SELECT_HOUSE, partnerID, myPID);
          break;
        }
      }
      pthread_mutex_unlock(&houses_array_mutex);

      if(notSelected) {
        sleep(1);
        printf("[%05d][%02d] Master can't access house\n", lamport_clock, myPID);
      }
    } while(houseID == -1);

    // Selected house to robbery
    printf("[%05d][PID: %02d][IT: %02d] I have house! Selected to robbery %02d\n", lamport_clock, myPID, iteration, houseID);
  } else {
    printf("[%05d][PID: %02d][IT: %02d] Skip requests, %02d should try to access\n", lamport_clock, myPID, iteration, partnerID);

    // Wait until master send message about house
    while(houseID == -1) {
      sleep(1);
      printf("[%05d][%02d] Slave sleep - wait house assign\n", lamport_clock, myPID);
    }
  }
}

/*
 * Message + sleep - visit house
 */
void robbery() {
  // Show message
  printf("[%05d][%02d] With %02d visit house %d\n", lamport_clock, myPID, partnerID, houseID);
  // Sleep random time
  int sleep_time = (rand() % 4) + 1;
  printf("[%05d][%02d] -- ROBBERY -- [%02d seconds]\n", lamport_clock, myPID, sleep_time);
  sleep(sleep_time);
}

/*
 * Release assigned resource (house) + in hidden also partner
 */
void release_resources() {
  // Master can access to critical sections
  if (master) {
    // We must send to slave message and exit critical section for houseID
    send(lamport_clock, houseID, partnerID, TAG_FINISH_PARTNERSHIP, partnerID, myPID);

    // Set house as required to return
    pthread_mutex_lock(&houses_to_return_mutex);
    houses_to_return_list.push_back(houseID);

    pthread_mutex_lock(&houses_array_mutex);
    houses_responses_array[houseID] = 1;
    pthread_mutex_unlock(&houses_array_mutex);

    pthread_mutex_unlock(&houses_to_return_mutex);
    printf("[%05d][%02d] Exit from partnership with %02d\n", lamport_clock, myPID, partnerID);
  } else {
    // We wait until master send release house
    while(houseID != -1) {
      sleep(1);
      printf("[%05d][%02d] Wait until Master %02d send me goodbye\n", lamport_clock, myPID, partnerID);
    }
  }
}

/*
 * Set variables to enable run in while loop
 */
void init_variables() {
  // PartnerID + ACK
  pthread_mutex_lock(&partner_mutex);
  partnerID = -1;
  pthread_mutex_unlock(&partner_mutex);

  // Release current house
  houseID = -1;
}

/*
 * Access to saloon with partner
 */
void want_saloon() {
  // Only master send request to access saloon
  if (master) {
    Request temp = Request(lamport_clock, myPID);
    // Lock, append request, sort, unlock
    pthread_mutex_lock(&saloon_mutex);
    saloon_queue.push_back(temp);
    received_saloon_ack = 1;
    pthread_mutex_unlock(&saloon_mutex);
    // Broadcast access to saloon request
    broadcast(lamport_clock, iteration, temp.time, TAG_ENTER_SALOON, total_process, myPID);
    // Wait until receive all confirmations
    while(received_saloon_ack < total_process) {
      usleep(1000);
    }

    if (debug_mode) {
      printf("[%05d][%02d] Received all messages (saloon access)\n", lamport_clock, myPID);
    }

    // Sort requests
    pthread_mutex_lock(&saloon_mutex);
    sort_requests(saloon_queue);
    pthread_mutex_unlock(&saloon_mutex);
    int current_position = INT_MAX;

    while(!is_in_saloon) {
      // Calculate total process inside saloon and check can access
      current_position = 2 * (check_position(saloon_mutex, saloon_queue, myPID) + 1);
      if (current_position <= P) {
        // Note partner about enter to saloon
        is_in_saloon = true;
        send(lamport_clock, iteration, partnerID, TAG_IN_SALOON, partnerID, myPID);
      } else {
        sleep(1);
      }
    }

    // Can access to saloon
    printf("[%05d][PID: %02d][IT: %02d] I can access to saloon with %02d\n", lamport_clock, myPID, iteration, partnerID);
  } else {
    // As slave - we ignore send and wait until receive note from master
    printf("[%05d][PID: %02d][IT: %02d] Skip saloon request, %02d should try to access\n", lamport_clock, myPID, iteration, partnerID);

    // Wait until master send message about saloon
    while(!is_in_saloon) {
      sleep(1);
      printf("[%05d][%02d] Slave sleep - wait saloon access granted\n", lamport_clock, myPID);
    }
  }
}

/*
 * Fill papers about access to house.
 * After sleep random time, master send release message in broadcast.
 */
void fill_papers() {
  int sleep_time = (rand() % 5) + 2;
  printf("[%05d][%02d] Fill papers [%02d seconds]\n", lamport_clock, myPID, sleep_time);
  sleep(sleep_time);
  printf("[%05d][%02d] Papers ready\n", lamport_clock, myPID);

  // If master - send release from saloon critical section
  if (master) {
    remove_from_saloon_queue(myPID);
    broadcast(lamport_clock, iteration, partnerID, TAG_RELEASE_SALOON, total_process, myPID);
  }
}

/*
 * Main function to run Thieves' Guild code
 * @param int argc - Total number of parameters
 * @param char *argc[] - List with parameters
 * @return int status - action status, 0 => success, else error
 */
int main(int argc, char **argv) {
  // stdout - disable bufforing
  setbuf(stdout, NULL);

  // Set program parameters
  if (!set_parameters(argc, argv)) {
    puts("[ERROR] You should start with NAME D P parameters (D and P greater than zero)");
  } else {
    // Parameters setup with success
    if (debug_mode) {
      printf("[INFO] Parameters setup correct\n");
    }

    // Check MPI threads
    enable_thread(&argc, &argv);

    // Create new thread - run for receive messages in loop (as monitor)
    pthread_t monitor_thread_receiver;
    pthread_create(&monitor_thread_receiver, NULL, receive_loop, 0);
    pthread_t monitor_thread_sender;
    pthread_create(&monitor_thread_sender, NULL, release_assigned_houses, 0);

    // Get process ID and total process number
    MPI_Comm_rank(MPI_COMM_WORLD, &myPID);
    MPI_Comm_size(MPI_COMM_WORLD, &total_process);

    // Random seed depends on process myPID
    srand(myPID);

    // Initialize variables
    houses_responses_array = new int[D];
    houses_vec = new std::vector<Request>[D];
    for (int i = 0; i < D; i++) {
      houses_responses_array[i] = 1;
      pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
      houses_mutex.push_back(mutex);
    }

    // Barier to start calculations
    if (debug_mode) {
      printf("[%05d][%02d][INFO] PROCESS %d READY\n", lamport_clock, myPID, myPID);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    while(1) {
      iteration++;
      printf("[%05d][%02d] -- CODE RUN -- ITERATION %02d --\n", lamport_clock, myPID, iteration);

      // 1. Init variables
      init_variables();

      // 2. Find partner
      want_partner();

      // 3. Find house to robbery
      want_house();

      // 4. Try access to saloon
      want_saloon();

      // 5. Fill papers and release saloon
      fill_papers();

      // 6. Robbery
      robbery();

      // 7. Release all resource
      release_resources();
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
