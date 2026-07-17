#pragma once
#include <random>

inline std::mt19937& globalRNG() {
    static std::mt19937 rng{42};  // deterministic seed for reproducibility
    return rng;
}

// Seed once (deterministic, avoids std::random_device blocking on Windows)
inline void seedRNG() {
    globalRNG().seed(42);
}
