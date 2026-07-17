#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <stdexcept>

class BinaryWriter {
    std::vector<uint8_t> buf_;
public:
    void writeU8(uint8_t v) { buf_.push_back(v); }
    void writeU16(uint16_t v) {
        buf_.push_back((uint8_t)(v & 0xFF));
        buf_.push_back((uint8_t)((v >> 8) & 0xFF));
    }
    void writeFloat(float v) {
        auto* p = reinterpret_cast<const uint8_t*>(&v);
        for (size_t i = 0; i < sizeof(v); ++i) buf_.push_back(p[i]);
    }
    void writeString(const std::string& s) {
        if (s.size() > 255) throw std::runtime_error("String too long for binary");
        writeU8((uint8_t)s.size());
        for (auto c : s) buf_.push_back((uint8_t)c);
    }
    void writeBool(bool v) { writeU8(v ? 1 : 0); }

    const std::vector<uint8_t>& data() const { return buf_; }
    size_t size() const { return buf_.size(); }
    void clear() { buf_.clear(); }
};

class BinaryReader {
    const uint8_t* ptr_;
    const uint8_t* end_;
public:
    BinaryReader(const std::vector<uint8_t>& data)
        : ptr_(data.data()), end_(data.data() + data.size()) {}

    uint8_t readU8() {
        if (ptr_ + 1 > end_) throw std::runtime_error("BinaryReader: out of data (u8)");
        return *ptr_++;
    }
    uint16_t readU16() {
        if (ptr_ + 2 > end_) throw std::runtime_error("BinaryReader: out of data (u16)");
        uint16_t v = ptr_[0] | ((uint16_t)ptr_[1] << 8);
        ptr_ += 2; return v;
    }
    float readFloat() {
        if (ptr_ + 4 > end_) throw std::runtime_error("BinaryReader: out of data (float)");
        float v;
        std::memcpy(&v, ptr_, sizeof(v));
        ptr_ += 4; return v;
    }
    std::string readString() {
        uint8_t len = readU8();
        if (ptr_ + len > end_) throw std::runtime_error("BinaryReader: out of data (string)");
        std::string s(reinterpret_cast<const char*>(ptr_), len);
        ptr_ += len; return s;
    }
    bool readBool() { return readU8() != 0; }
    bool done() const { return ptr_ >= end_; }
};

struct BinaryEntityState {
    static void write(BinaryWriter& w, int id, float x, float y,
                      float rot, uint8_t imageId, float health,
                      bool isReady, int playerId, uint8_t magabomb,
                      const std::string& name) {
        w.writeU16((uint16_t)id);
        w.writeFloat(x); w.writeFloat(y);
        w.writeU16((uint16_t)(rot * 10));  // 0-3600
        w.writeU8(imageId);
        uint8_t flags = 0;
        if (playerId >= 0) flags |= 1;
        if (isReady) flags |= 2;
        w.writeU8(flags);
        if (playerId >= 0) {
            w.writeU8((uint8_t)std::min(100.0f, health));
            w.writeU8(magabomb);
            w.writeString(name);
        }
    }
};
