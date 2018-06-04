/*
 * Thieves' Guild project
 *  Poznań, May 2018
 *
 * utils.cpp
 *  Utils implementation file with functions used inside software code.
 *
 * Authors:
 *  Jarosław Skrzypczak index. 127265 (@jarkendar)
 *  Bartosz Górka index. 127228 (@bartoszgorka)
 */

#include <algorithm>
#include <vector>
#include "utils.h"

/*
 * Sort requests to critical section.
 * @param std::vector &list - List with Requests to sort, required reference to sort original data
 */
void sort_requests(std::vector<Request> &list) {
  std::sort(list.begin(), list.end());
}

/*
 * Sort list of int.
 * @param std::vector &list
 */
void sort_list(std::vector<int> &list) {
  std::sort(list.begin(), list.end());
}
