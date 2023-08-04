#pragma once

#define SIZE_OF_BUFFER 8

class SeedPacket {
   public:
    SeedPacket() {}
    ~SeedPacket() {}

    void Debug() {
        Serial.println("Packet: [" + 
            String(packet_[0]) + ", " + 
            String(packet_[1]) + ", " + 
            String(packet_[2]) + ", " + 
            String(packet_[3]) + ", " + 
            String(packet_[4]) + ", " + 
            String(packet_[5]) + ", " + 
            String(packet_[6]) + ", " + 
            String(packet_[7]) + "] "
            "\tr: " + String(read_index_) +
            "\tw: " + String(write_index_) +
            "\tb: " + String(buffer_length_)
            );
    }

    void NewRandomSeed() {
        SetSeed(GetRandom());
    }

    void SetSeed(uint16_t seed) {
        // Generate new seed and write to buffer.
        // packet_[write_index_] = static_cast<uint16_t>(seed);
        packet_[write_index_] = seed;
        Reseed();

        write_index_++;
        if (write_index_ == SIZE_OF_BUFFER) {
            write_index_ = 0;
        }

        buffer_length_ = constrain(buffer_length_ + 1, 0, SIZE_OF_BUFFER);
    };

    void NextSeed() {
        read_index_ = ++read_index_ % SIZE_OF_BUFFER;

        // If we are at the write index, add a new seed to the buffer.
        if (read_index_ == write_index_) {
            randomSeed(micros());  // Ensure new seeds are not deterministic
            NewRandomSeed();
        }
    }

    void PrevSeed() {
        // Don't wrap if we haven't filled up the buffer yet.
        if (read_index_ == 0 && buffer_length_ < SIZE_OF_BUFFER) {
            return;
        }

        // Check if we're at the last position. If so, return the current seed.
        if (read_index_ - 1 == write_index_ || (read_index_ == 0 && write_index_ == SIZE_OF_BUFFER - 1)) {
            return;
        }

        // Decrement the read index and wrap if necessary.
        if (read_index_ == 0) {
                read_index_ = buffer_length_ - 1;
        } else {
            read_index_--;
        }
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
    uint16_t packet_[SIZE_OF_BUFFER] = {GetRandom()};
    uint8_t read_index_ = 0;
    uint8_t write_index_ = 0;
    uint8_t buffer_length_ = 0;
};
