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
 * Tag - start find partner process
 */
#define TAG_FIND_PARTNER 110
#endif

#ifndef TAG_ALREADY_PAIRED
/*
 * Tag - already in pair
 */
#define TAG_ALREADY_PAIRED 111
#endif

#ifndef TAG_ACCEPT_PARTNER
/*
 * Tag - acceptance of the proposal
 */
#define TAG_ACCEPT_PARTNER 112
#endif

#ifndef TAG_SELECTED_PARTNER
/*
 * Tag - selected partner
 */
#define TAG_SELECTED_PARTNER 113
#endif
