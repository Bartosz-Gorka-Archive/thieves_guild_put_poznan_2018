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
