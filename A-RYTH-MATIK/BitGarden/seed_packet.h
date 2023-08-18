#pragma once

#define SIZE_OF_BUFFER 4

class SeedPacket {
   public:
    SeedPacket() {}
    ~SeedPacket() {}

    // Generate a new random seed and add it to the front of the buffer.
    void NewRandomSeed() {
        // Advance the write cursor toward the max position.
        if (write_index_ < SIZE_OF_BUFFER - 1) write_index_++;

        // If the buffer is full, shift elements
        if (buffer_length_ == SIZE_OF_BUFFER) {
            memcpy(&packet_[0], &packet_[1], sizeof(uint16_t) * SIZE_OF_BUFFER);
        }

        SetSeed(GetRandom());
        Reseed();
    };

    // Set the given seed at the front of the buffer.
    void SetSeed(uint16_t seed) {
        // Write new seed to buffer.
        packet_[write_index_] = seed;
        // Increment the buffer length count.
        if (buffer_length_ < SIZE_OF_BUFFER) buffer_length_++;
    }

    // Update the seed at the current read index.
    void UpdateSeed(uint16_t seed) {
        packet_[read_index_] = seed;
    }

    // Move the seek packet cursor ahead one. Add a new random seed if the buffer is full.
    void NextSeed() {
        // Increment read index allowing for exceeding buffer size to indicate
        // a new seed is needed.
        if (read_index_ < SIZE_OF_BUFFER) read_index_++;

        // If read index reaches the buffer length, add a new seed.
        if (read_index_ == buffer_length_) {
            randomSeed(micros());  // Ensure new seeds are not deterministic
            NewRandomSeed();
        }

        // Constrain the read index to the buffer size.
        if (read_index_ == SIZE_OF_BUFFER) read_index_--;
    }

    // Move the seek packet cursor back one.
    void PrevSeed() {
        if (read_index_ > 0) read_index_--;
    }

    uint16_t GetSeed() {
        return packet_[read_index_];
    }

    void Reseed() {
        randomSeed(GetSeed());
    };

    uint16_t GetRandom() {
        return random(UINT16_MAX);
    }

   private:
    uint16_t packet_[SIZE_OF_BUFFER] = {};
    uint8_t read_index_ = 0;
    uint8_t write_index_ = 0;
    uint8_t buffer_length_ = 0;
};
