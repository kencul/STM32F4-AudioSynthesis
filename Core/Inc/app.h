/*
 * app.h
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#ifndef SRC_APP_H_
#define SRC_APP_H_

#include <stdio.h>

#define NUM_FRAMES 256
#define BUFFER_SIZE (NUM_FRAMES * 4)

#ifdef __cplusplus
extern "C" {
#endif

// This is the bridge function.
// It is defined in app.cpp and called in main.c
void cpp_main(void);

#ifdef __cplusplus
}
#endif


#endif /* SRC_APP_H_ */
