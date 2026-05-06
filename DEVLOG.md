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

They can also be pulled back up to 3.3V by a pull-up resistor. This is a disadvantage over SPI protocol, as the pull-up resistor is slower than transistors pushing voltage.

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
2. Send all configuration I2C messages before setting the message to set Power Control 1 (step 3), and send manufuacturer provided initiazlizatin data to reserved registers (step 4).
3. Make sure the chip is getting a stable clock signal. MCLK: high speed clock from STM32, SCLK: bit-rate clock, LRCK: Sample rate clock. These must be going before starting the chip. This means starting I2S to send the MCLK, so run `HAL_I2S_TRANSMIT_DMA` to start i2s communication (step 5).
4. Set Power Control 1 register to 0x9E to start the chip (step 6)

All the clocks are handled by I2S, a separate protocol that handles the audio data. These are set up in CubeMX and pretty much automatically handled by it.

In this program, this process is bundled into the [Codec class](Core/Src/Codec.cpp):

```cpp
uint8_t Codec::init(int16_t * buffer, size_t bufferSize) {
	uint8_t status = 0;
	HAL_GPIO_WritePin(CODEC_RESET_GPIO_Port, CODEC_RESET_Pin, GPIO_PIN_SET);
	// Address 4: Power Control 2, Data: 10101111 (Headphone on always, speakers off always)
	status += write(0x04, 0xaf);
	// Address 6: Interface Control 2, Data: 00000111 (Slave mode, normal polarity (doesn't matter), DSP mode off, I2S Format, 16bit data)
	status += write(0x06, 0x07);
	
	// Manufacturer provided initialization
	status += write(0x00, 0x99);
    status += write(0x47, 0x80);
    status += write(0x32, 0xBB);
    status += write(0x32, 0x3B);
    status += write(0x00, 0x00);

	HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *)buffer, bufferSize);
	
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

In CubeMX, most of the pins on the STM32 can be configured to do different things.

By refering to the following pin out diagram, you can see which pins can be set to what.

![pin out diagram](/Docs/STM32F407DISC_PinOut.avif)

The red circles represent pins that aren't recommended to use, as they are wired to on board hardware. Using these for other purposes than what they are wired for will stop the hardware from being usable.

Simply searching for the desired pin setting in CubeMX will show which pins can be set to the mode.

Details on which pins are connected to which board function is found in Table 7 on page 26 of the [STM32F4 Disc Manual](Docs/STM32F4DISC_manual.pdf).

### Audio Codec Reset Pin

The Audio Codec has a reset pin that needs to be set to high for the chip to turn on. Refering to the [manual](Docs/STM32F4DISC_manual.pdf), the reset pin for the codec chip is connected to PD4.

In CubeMX, set PD4 to GPIO_Output. Rename it to CODEC_RESET, so that we can use that name as a macro in code.

### Setting Up I2C pins in CubeMX

When enabling I2C1 in CubeMX on the STM32F4, it defaults to PB7-I2C1_SDA and PB6-I2C1_SCL. These pins would work work I2C with an external I2C device, but to the on-board codec chip is wired to PB6 and **PB9**. So, in CubeMX, after enabling I2C1 for I2C, you need to click and drag PB6 over to PB9. This makes the I2C1 engine route the output through PB9 instead so that the signal actually reaches the codec chip.

## I2S Protocol

A protocol made for audio to connnect two digital audio devices. Communicates PCM (Pulse Code Modulation) data, a digital form of audio data used in wav files.

On the STM32F4, the Codec chip is connected to the STM32 through I2S to communicate audio data.

Uses 4 wires:

* Serial Clock (SCK): The metronome provided by the STM 32. A bit is communicated every pulse
* Word Select (WS): Communicates which channel the data is for (L/R). Low means left, high means right.
* Serial Data (SD): Audio information clumped into words of 16 or 24 bits.
* (Optional) Master Clock (MCLK): Clock to run the chip. Provided by the STM32, usually 256 times the sample rate.

There is no handshaking that goes on in I2S. The data is streamed out constantly without confirmation from the slave.

The process of sending one sample is as follows:

1. WS drops to low to prepare to send the left channel
2. SCK pulses 16 times to send one sample
3. WS flips high to prepare to send the right channel
4. SCK pulses 16 times to send a sample
5. The codec has two 16bit numbers that is processes in its DAC to drive the headphone jack or speaker

### I2S Setup in CubeMX

Go to multimedia and select I2S3. Select Half-Duplex master mode. The half-duplex means it will either transmit or receive, but not both. We need it to be a master transmit.

The sample rate is also set here. The real frequency is based on the provided clock source. If it is off, refer to the [#Clock Configuration](#clock-configuration) section on how to calculate and set the PLLI2S to the proper value for the sample rate.

## DMA

The CPU, DMA and the I2S device both live in the microcontroller (MCU). The I2S hardware sends one sample at a time. Instead of the CPU sending one sample at a time, the DMA accesses a buffer of audio and sends over a sample whenever the I2S requests it. The sample is placed in SPI3_TX, a hardware signal line within the STM32 chip connecting I2S to DMA. The DMA places the sample data in this register, and the I2S sends high voltage down when it needs another sample. As the CPU doesn't deal with any of this copying, the CPU has more time for complex math.

The DMA communicates to the CPU when it has used half of the buffer through an interrupt. This interrupt makes the CPU stop what its doing and run a callback function that generates audio for the half of the buffer that was used. This is what a circular buffer, or a sliding window, is.

### DMA Setup in CubeMX

Under the I2S3 menu, the DMA settings section allows you to add a DMA request by SPI3_TX. Setting this DMA request to a circular mode with half-word data width sets up 16 bit audio buffer DMA perfectly.

## SPI Interface

A higher rate of data transfer compared to I2C at the cost of needing a dedicated line for each slave.

## Clock Configuration

There are so many abbrevaitions and boxes to configure when looking at the clock configuration tab. This section will go through each box relavent to audio DSP.

### RCC

The Reset and Clock Control. This takes the raw vibrations of crystals and distribute them to every part of the chip. The pulse from the RCC, or the clock, drives the chip. This is the clock configuration tab in CubeMX is configuring.

The STM32F4 disc board provides two crystals

- HSI (High-Speed Internal): The built in RC oscillator inside the silicon. It is always there when the chip boots up, starting up near instantly with low power. Ensures the chip boots in a timely manner, and provides a inaccurate but low power clock.

- HSE (High-Speed External): A physical metal crystal soldered onto the board. Takes a few milliseconds to stabilitze, but is accurate to a few parts per million. Nessessary for driving the I2S bus at an accurate clock speed to match the desired sample rate.

### Clock Configuration Tab in CubeMX

The clock config tab is simply the user interface of the RCC. I will go through all the steps the blog post goes through on this page.

- *First, we will set our external crystal. To do that click on System Core, next choose RCC. Set High Speed Clock (HSE) as Crystal.*

	Before entering the clock config, the RCC must be told to use the HSE. Setting the HSE mode for the RCC to Crystal tells the RCC that there is a crystal connected that it needs to power.

- *Change input frequency from whatever value it is to 8.*

	Setting the input freq to 8MHz tells the RCC there is a 8MHz clock signal coming in. This is a set constant for the discovery board, as the on-board crystal is a 8MHz crystal. If you were to solder on a different crystal, you would change this value to whatever speed crystal you use.

- *In PLL Source: Change from HSI to HSE.*

	Tells the PLL (Phase Locked Loop) to use the external crystal over the internal one.

- *In System Clock: Change from whatever it is to PLLCLK.*

	Neither the HSI nor the HSE is fast enough to run the chip at its best clock speed. The STM32F407VG6 chip can run up to 168MHz, so the system clock should be at 168MHz to get maximum performance out of the chip. The only way to get a clock speed this high from HSI or HSE is by using PLL to scale the clock speed up.

- *NOW! Let’s do some magic: write down 168 in HCLK and hit ENTER. See how it magically adjusts all the parameters!*

	HCLK is the clock sent to the chip. We want this at 168MHz for max performance. By pressing enter, the paramters for the PLL are calculated to take the HSE clock input up to 168MHz.

	The values past HCLK are important, but don't need to be touched. By default, they are scaled from HCLK to their proper values. They are important, however, as they are clock speeds for the I2C (APB1 peripheral clock), I2S (APB1), and DMA.

- *And change PLLI2S to 123*

	This change comes after the I2S communication is set to 48kHz. The audio codec chip needs a clock speed of 256 times the sample rate, or 12.288 MHz. By getting close to the desired clock speed, the I2S hardware on the STM32 (I2S2 and I2S3) can extrapolate down to a clock speed fit for 48kHz sample rate. This extrapolation results in the 0.09% error. The PLLI2S provides a way to separate the ideal clock for the CPU from the ideal clock for audio.

	As there are two I2S engines on board, one can handle mic input, while the other controls headphone output, for instance.

## Board Support Packages (BSP)

Collection of drivers that encapsulate the capabilities of the board into user friendly functions. A wrapper designed specifically for each board.

Should (?) make getting started easier

[Guide on how to install BSP into a STM32CubeIDE project](https://community.st.com/t5/stm32-mcus/how-to-add-a-bsp-to-an-stm32cubeide-project/ta-p/49812)

[github for the components for the STM32F4.](https://github.com/STMicroelectronics/stm32f4discovery-bsp?tab=readme-ov-file) Don't download from here, but reference the bottom of the README to know which components you need to copy into your project

# Developing the Synth

With the complex set-up of the board configured, and with basic knowledge of the communication protocols, I moved beyond the blog post to start developing a polyphonic synth.

## USB MIDI Device Conversion

While the initial setup focused on I2S and I2C for the on-board codec, I have since implemented a communication layer for external control via the OTG port.

* **OTG Port Transformation**: I modified the USB stack to act as a MIDI device rather than a standard audio device.
* **MIDI Parser**: In `app.cpp`, the `handleMidi()` function pops packets from a `gMidiBuffer` and decodes status bytes for Note On (0x90), Note Off (0x80), Control Change (0xB0), and Pitch Bend (0xE0).
* **Parameter Mapping**: MIDI CC 1 is mapped to the Mod Wheel, and CC 123 serves as a "Panic" command to silence all voices.

## Polyphonic Voice Management

The `VoiceManager` class handles the logic for playing multiple notes simultaneously, moving beyond a single "recipe for sound".

* **Voice Allocation**: The engine manages 8 independent voices. When a MIDI Note On is received, it searches for the "best" voice by prioritizing idle voices, then released voices, and finally the oldest active voice if it needs to "steal" one.
* **Mixing Engine**: It iterates through all active voices, calculates their audio blocks, and sums them into a `mixBus`.
* **Global LFO**: A single global Low-Frequency Oscillator is updated once per audio block to provide synchronized modulation across all voices.

## Wavetable Synthesis & Morphing

I expanded the oscillator to handle complex timbres rather than just simple sine waves.

* **Dual-Slot Morphing**: The engine loads two wavetables (Slot A and Slot B). A "Morph" parameter allows for real-time cross-fading between these two shapes.
* **Wavetable Library**: I implemented a library of waveforms (Sine, Square, etc.) that can be cycled through using hardware buttons.
* **Phase Accumulation**: It uses a `uint32_t` phase accumulator to "scrub" through wavetables at a speed determined by the MIDI note frequency.
* **Pitch Bend**: Support for MIDI pitch bend messages uses a power-of-two formula to scale frequency accurately across semitones.

## Digital Signal Processing (DSP)

This section covers the implementation of the core synthesis components.

### Zero-Delay State Variable Filter (SVF)

The filter implementation in `svf.cpp` uses an advanced topology for better analog-style response.

* **Algebraic Loop Solver**: To achieve "Zero-Delay" characteristics, the code solves the algebraic loop at each sample. It calculates a denominator (`den = 1.0f / (1.0f + g * (g + k))`) to determine coefficients `a1`, `a2`, and `a3` without relying on unit delays in the feedback path.
* **Self-Oscillation Stability**: The damping coefficient `k` is clamped to a minimum of 0.01f to ensure the filter remains stable even at high resonance settings.

### Moog Ladder Filter

An alternative resonant filter based on the classic Moog design is also included.

This version was initially what I wanted, as it has a interesting color, but the calculations required to use it was too lengthy for the board I am using. I had to reduce the quality of the filter a lot in order for it to be within the MCU's capability.

### ADSR & "Soft-Kill" Envelope

Each voice has a dedicated ADSR (Attack, Decay, Sustain, Release) state machine to prevent notes from clicking or stopping abruptly.

* **EnvState::KILL**: When `Osc::noteOn()` detects a voice is already active, it triggers `_adsr.kill()` instead of a hard reset.
* **Anti-Click Ramp**: The `kill()` function calculates a `_killStep` to quickly ramp the volume to zero over approximately 5ms.
* **Pending Note Logic**: The oscillator stores new note data in a `_pending` struct, waits for the kill ramp to finish, and then executes the new `noteOn`.

### Fixed-Point Conversion

While internal synthesis uses floating-point math, the final output is clamped and converted to 16-bit signed integers for the I2S hardware.

## Hardware Control & Sensing

A robust system was added to interface the internal engine with the physical board.

* **Potentiometer Bank (PotBank)**: Using the STM32's ADC and a multiplexer, the system reads 8 different knobs.
* **Exponential Scaling**: Parameters like Filter Cutoff and ADSR times use exponential scaling so that knob turns feel natural to the human ear.
* **DMA & Interrupts**: ADC scanning is triggered by timer `TIM4` and handled via interrupts to ensure the CPU isn't wasted waiting for sensor readings.
* **Button Debouncing**: Software logic detects clean "falling edge" presses for changing waveforms, preventing "ghost" triggers.

## Visuals & Feedback

The UI implementation transforms the project into a "standalone instrument".

* **OLED Graphics**: An SSD1306 display shows real-time information.
* **Dynamic Views**: The screen automatically switches views (Wavetable, Filter, or ADSR) based on which parameter is being adjusted, timing out back to the wavetable view after 1.2 seconds.
* **Wavetable Preview**: The OLED draws the actual morphed shape of the current waveform using a preview buffer.
* **LED Voice Indicators**: An external I2C LED controller (PCA9685) displays the volume level of each of the 8 voices in real-time.
* **Startup Sequence**: A custom animation with rotating hexagons and breathing LEDs plays upon boot.

## System & Performance

* **CCMRAM Optimization**: The `VoiceManager` is placed in "Core Coupled Memory" (CCMRAM) to speed up execution by avoiding bus contention.
* **CPU Load Debugging**: I added code using the `DWT->CYCCNT` register to measure exactly how many microseconds each audio block takes to process.
* **Circular Buffer**: Audio is processed in two halves using Half-Transfer and Transfer-Complete DMA callbacks, ensuring the codec always has data while the CPU generates the next block.
