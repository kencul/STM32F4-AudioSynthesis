/*
 * Codec.h
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#ifndef SRC_CODEC_H_
#define SRC_CODEC_H_

#include <stdio.h>
#include <cstdint>

#define AUDIO_I2C_ADDR 0x94

class Codec {
public:
	Codec();
	virtual ~Codec();
	uint8_t init(int16_t * buffer, size_t bufferSize);
	uint8_t setVolume(float volume);
private:
	uint8_t write(uint8_t reg, uint8_t val);
};
#endif /* SRC_CODEC_H_ */
