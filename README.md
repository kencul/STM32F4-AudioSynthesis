# STM32F407G DISC1 Synth Dev Notes

## Getting Started

After hours of no results, just cloning [this repo](https://github.com/ytneu/stm32f4-basic-synth) and adding some includes for std libraries finally got me sound.

Their [blog post](https://m-w-bochniewicz.medium.com/first-steps-in-music-synthesis-and-stm32-610bb674139) walks through the set-up in CubeMX to get everything working.

The configuring of pins and clock timings are overwhelming, so copying the blog step by step was the way to go to just get started. It is definitely significantly harder than Teensy, which I am used to.

The blog doesn't go into any detail about anything, just a recipe for sound to be blasted out of the headphone jack. I will use this project to slowly dissect what is happening to better understand what is happening.

## I2C

REF: [Youtube Video on I2C by GreatScott!](https://www.youtube.com/watch?v=_fgWQ3TIhyE)

I2C is similar to MIDI signals: it is a protocol to send address and data information.

It consists of two lines: SCL, the clock, and SDA, the data. Both sides, the master and the slave, use a **Open Drain** setup, which lets either side pull the line to 0. This technique is used to stop the clock when a device is busy by pinning it to 0, or to signal that it is ready by pulling down SDA.

### Start and End

The communication starts with both lines high, and SDA is pulled down while the clock is high. As a rule, SDA cannot change while the clock is high, except for when the communication starts. The opposite happens when the communication ends: SDA is set to high while the clock is high.

### Device Address

After communication starts, the master sends the address of the device it wants to communicate with. This address is found in the data sheet of the i2c chip as the I2C address. This address is 7 bits long.

After the 7 bits, the next bit is the read/write bit, 0 for write, 1 for read. This communicates if the master wants to send or receive data.

After the 9th clock pulse, SDA is set to high to wait for the ACK signal.

### ACK signal

If the slave with the specified address is ready for communication, it pulls SDA back down. This signals the master to prepare for sending or receiving data.

### Register

Complex I2C devices have lots of configuration. For intance, the CS43L22 Audio Codec chip on the STM32F4 Disc board has 30 something settings. Instead of sending 30 bytes of configuration everytime, the first byte of data specifies the register. This communicates to the slave what kind of data is being sent.

However, this depends on the chip. For instance, if the slave were a big memory chip, there could be hundreds of thousands of registers. More simple chips might expect no register and the same few number of bytes every communication.

This is all found in the data sheet

### Data

The actual data that is being transmitted.

Referencing the data sheet is crucial to ensure the correct information is being sent.

### End

Referenced in Start, SDA is set to high while the clock is high.

## Initializing the Codec Chip with I2C

Now that I understand the I2C protocol, the question is, "how do I do this on the STM32F4?"

`HAL_I2C_Mem_Write` sends an I2C message with register information.

By supplying a I2C structure defined in `i2c.h`, device address, register address and size, data and size, and a timeout duration, all the logic of the protocol is handled.

For communication with the CS43L22 chip, as it always takes a 1 byte register address and 1 byte of data, a function can be made to easily write to it:

```cpp
uint8_t Codec::write(uint8_t reg, uint8_t val) {
	HAL_StatusTypeDef status = HAL_OK;

	status = HAL_I2C_Mem_Write(&hi2c1, AUDIO_I2C_ADDR, reg, 1, &val, sizeof(val), HAL_MAX_DELAY);
	if (status != HAL_OK)
		return 1;
	return 0;
}
```

And so initializing the codec with I2C looks like this:

```cpp
// Address 4: Power Control 2, Data: 10101111 (Headphone on always, speakers off always)
status += write(0x04, 0xaf);
// Address 6: Interface Control 2, Data: 00000111 (Slave mode, normal polarity (doesn't matter), DSP mode off, I2S Format, 16bit data)
status += write(0x06, 0x07);
// Address 2: Power Control 1, Data: 10011110 (Powered up)
status += write(0x02, 0x9e);
```

## Powering on the Codec Chip

The Codec chip has a specific order of things needed to power on properly.

As the data sheet states:
>1. Hold RESET low until the power supplies are stable.
>2. Bring RESET high.
>3. The default state of the “Power Ctl. 1” register (0x02) is 0x01. Load the desired register settings while keeping the “Power Ctl 1” register set to 0x01.
>4. Load the required initialization settings listed in Section 4.11.
>5. Apply MCLK at the appropriate frequency, as discussed in Section 4.6. SCLK may be applied or set to master at any time; LRCK may only be applied or set to master while the PDN bit is set to 1.
>6. Set the “Power Ctl 1” register (0x02) to 0x9E.
>7. Bring RESET low if the analog or digital supplies drop below the recommended operating condition to prevent power glitch related issues.

This boils down to:

1. Use a Pin D4 as GPIO to output High to power the chip (steps 1, 2)
2. Send all configuration I2C messages before setting the message to set Power Control 1 (step 3)
3. Make sure the chip is getting a stable clock signal. MCLK: high speed clock from STM32, SCLK: bit-rate clock, LRCK: Sample rate clock. These must be going before starting the chip. (step 5)
4. Set Power Control 1 register to 0x9E to start the chip (step 6)

All the clocks are handled by I2S, a separate protocol that handles the audio data. These are set up in CubeMX and pretty much automatically handled by it.

In this program, this process is bundled into the [Codec class](Core/Src/Codec.cpp):

```cpp
// Codec.cpp 19-39
uint8_t Codec::init() {
	uint8_t status = 0;
	HAL_GPIO_WritePin(CODEC_RESET_GPIO_Port, CODEC_RESET_Pin, GPIO_PIN_SET);
	// Address 4: Power Control 2, Data: 10101111 (Headphone on always, speakers off always)
	status += write(0x04, 0xaf);
	// Address 6: Interface Control 2, Data: 00000111 (Slave mode, normal polarity (doesn't matter), DSP mode off, I2S Format, 16bit data)
	status += write(0x06, 0x07);
	// Address 2: Power Control 1, Data: 10011110 (Powered up)
	status += write(0x02, 0x9e);

	return status;
}

uint8_t Codec::write(uint8_t reg, uint8_t val) {
	HAL_StatusTypeDef status = HAL_OK;

	status = HAL_I2C_Mem_Write(&hi2c1, AUDIO_I2C_ADDR, reg, 1, &val, sizeof(val), HAL_MAX_DELAY);
	if (status != HAL_OK)
		return 1;
	return 0;
}
```

## Pin Configuration



## I2S

## Clock Configuration

## Board Support Packages (BSP)

Collection of drivers that encapsulate the capabilities of the board into user friendly functions. A wrapper designed specifically for each board.

Should (?) make getting started easier

[Guide on how to install BSP into a STM32CubeIDE project](https://community.st.com/t5/stm32-mcus/how-to-add-a-bsp-to-an-stm32cubeide-project/ta-p/49812)

[github for the components for the STM32F4.](https://github.com/STMicroelectronics/stm32f4discovery-bsp?tab=readme-ov-file) Don't download from here, but reference the bottom of the README to know which components you need to copy into your project
