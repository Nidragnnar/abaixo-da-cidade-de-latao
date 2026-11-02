#pragma once
#include <random>

namespace mundo {

// Sorteios próprios (a distribuição da biblioteca padrão pode mudar entre compiladores)
int sortear(std::mt19937& rng, int min, int max);           // inteiro em [min, max]
float sortear01(std::mt19937& rng);                         // real em [0, 1)
float sortearReal(std::mt19937& rng, float min, float max);  // real em [min, max]

}
