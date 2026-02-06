#pragma once
#include <array>
#include <atomic>
#include <cstdint>

// USB MIDI packets are always 4 bytes [cite: 504, 532]
struct MidiPacket {
    uint8_t data[4];
};

class MidiBuffer {
private:
    static constexpr int SIZE = 128; 
    std::array<MidiPacket, SIZE> buffer;
    std::atomic<int> head{0};
    std::atomic<int> tail{0};

public:
    // Called by USB Interrupt
    void push(const uint8_t* raw) {
        int next = (head.load() + 1) % SIZE;
        if (next != tail.load()) {
            for (int i = 0; i < 4; ++i) {
                buffer[head].data[i] = raw[i];
            }
            head.store(next);
        }
    }

    // Called by Main Loop
    bool pop(MidiPacket& out) {
        if (head.load() == tail.load()) return false;
        out = buffer[tail];
        tail.store((tail.load() + 1) % SIZE);
        return true;
    }
};
