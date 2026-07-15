#pragma once

#include <cstdint>
#include <random>
#include <stdexcept>

namespace ddg {

// Thin wrapper around std::mt19937_64 giving every generator in this project
// a single, seedable source of randomness. Seeding a DataGenerator with the
// same value always reproduces the same dummy dataset, which is useful for
// tests and for reproducing a bug report.
class RandomEngine {
public:
    explicit RandomEngine(uint64_t seed) : engine_(seed) {}

    RandomEngine() : engine_(std::random_device{}()) {}

    // Returns an integer in the inclusive range [min, max].
    int64_t NextInt(int64_t min, int64_t max) {
        if (min > max) {
            throw std::invalid_argument("RandomEngine::NextInt: min must be <= max");
        }
        std::uniform_int_distribution<int64_t> dist(min, max);
        return dist(engine_);
    }

    // Returns a double in the inclusive range [min, max].
    double NextDouble(double min, double max) {
        if (min > max) {
            throw std::invalid_argument("RandomEngine::NextDouble: min must be <= max");
        }
        if (min == max) return min;
        std::uniform_real_distribution<double> dist(min, max);
        return dist(engine_);
    }

    // Returns true with the given probability (clamped to [0, 1]).
    bool NextBool(double trueProbability = 0.5) {
        if (trueProbability <= 0.0) return false;
        if (trueProbability >= 1.0) return true;
        std::bernoulli_distribution dist(trueProbability);
        return dist(engine_);
    }

    // Returns an index in [0, size).
    size_t NextIndex(size_t size) {
        if (size == 0) {
            throw std::invalid_argument("RandomEngine::NextIndex: size must be > 0");
        }
        std::uniform_int_distribution<size_t> dist(0, size - 1);
        return dist(engine_);
    }

private:
    std::mt19937_64 engine_;
};

} // namespace ddg
