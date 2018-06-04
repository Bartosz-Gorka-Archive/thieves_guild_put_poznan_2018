/*
 * Thieves' Guild project
 *  Poznań, May 2018
 *
 * utils.h
 *  Utils header file with declarated functions
 *  used inside software code.
 *
 * Authors:
 *  Jarosław Skrzypczak index. 127265 (@jarkendar)
 *  Bartosz Górka index. 127228 (@bartoszgorka)
 */

#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <vector>

/*
 * Structure Request to ensure correct manage data about process & time inside single Request to critical sesion.
 */
struct Request {
  /*
   * Lamport clock value (time)
   */
  int time;
  /*
   * Process ID
   */
  int pid;

  /*
   * Request structu constructor
   * @param t - Time
   * @param p - PID
   */
  Request(int t, int p) {
    time = t;
    pid = p;
  }

  /*
   * Override operator <
   * @param str - Request structure to compare
   */
  bool operator < (const Request& str) const {
    if (time < str.time) {
      return true;
    } else if (time == str.time && pid < str.pid) {
      return true;
    }

    return false;
  }
};

/*
 * Sort requests to critical section.
 * @param std::vector &list - List with Requests to sort, required reference to sort original data
 */
void sort_requests(std::vector<Request> &list);

/*
 * Sort list of int.
 * @param std::vector &list
 */
void sort_list(std::vector<int> &list);

#endif
