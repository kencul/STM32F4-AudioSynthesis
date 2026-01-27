/*
 * Codec.h
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#ifndef SRC_CODEC_H_
#define SRC_CODEC_H_

#include "main.h"

class Codec {
public:
	Codec();
	virtual ~Codec();
	uint8_t init();
private:
	uint8_t write();
};

#endif /* SRC_CODEC_H_ */
