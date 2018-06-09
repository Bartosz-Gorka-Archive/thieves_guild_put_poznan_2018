/*
 * Thieves' Guild project
 *  Poznań, May - Juny 2018
 *
 * tags.h
 *  MPI Tags header file, all tags prepared in single file to enable easier
 *  modify this values and eliminate duplicate code.
 *
 * Authors:
 *  Jarosław Skrzypczak index. 127265 (@jarkendar)
 *  Bartosz Górka index. 127228 (@bartoszgorka)
 */

#ifndef TAG_FIND_PARTNER
/*
 * TAG for enter to critical section and find partner to robbery
 */
#define TAG_FIND_PARTNER 110
#endif

#ifndef TAG_RESPONSE_PARTNER
/*
 * TAG for send response about someone question about critical section (partner)
 */
#define TAG_RESPONSE_PARTNER 120
#endif

#ifndef TAG_SELECTED_PARTNER
/*
 * TAG for note all process about selected partner to robbery
 */
#define TAG_SELECTED_PARTNER 130
#endif

#ifndef TAG_FINISH_PARTNERSHIP
/*
 * TAG for note partner about exit from partnership
 */
#define TAG_FINISH_PARTNERSHIP 150
#endif

#ifndef TAG_HOUSE_REQUEST
/*
 * TAG for houses request (bulk)
 */
#define TAG_HOUSE_REQUEST 200
#endif

#ifndef TAG_RESPONSE_HOUSE
/*
 * TAG for send response about houses request
 */
#define TAG_RESPONSE_HOUSE 210
#endif

#ifndef TAG_SELECT_HOUSE
/*
 * TAG for receive details about selected house by external process
 */
#define TAG_SELECT_HOUSE 220
#endif

#ifndef TAG_HOUSE_EXIT
/*
 * TAG for exit from last selected house
 */
#define TAG_HOUSE_EXIT 250
#endif

#ifndef TAG_ENTER_SALOON
/*
 * TAG for notify all process about enter to saloon's critical section
 */
#define TAG_ENTER_SALOON 300
#endif

#ifndef TAG_CONFIRM_SALOON
/*
 * TAG for confirm sender TAG_ENTER_SALOON
 */
#define TAG_CONFIRM_SALOON 310
#endif

#ifndef TAG_RELEASE_SALOON
/*
 * TAG for notify process about exit from saloon's critical section
 */
#define TAG_RELEASE_SALOON 320
#endif

#ifndef TAG_SYNCHRONIZE_PARTNER
/*
 * TAG for synchronize partners
 */
#define TAG_SYNCHRONIZE_PARTNER 500
#endif
