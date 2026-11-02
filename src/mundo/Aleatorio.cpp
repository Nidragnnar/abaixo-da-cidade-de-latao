#include "mundo/Aleatorio.h"

namespace mundo {

int sortear(std::mt19937& rng, int min, int max) { return min + static_cast<int>(rng() % (max - min + 1)); }

float sortear01(std::mt19937& rng) { return static_cast<float>(rng() % 1000000) / 1000000.0f; }

float sortearReal(std::mt19937& rng, float min, float max) {
    return min + (max - min) * static_cast<float>(rng() % 1000001) / 1000000.0f;
}

}
