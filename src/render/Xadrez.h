#pragma once
#include <cstdint>
#include <vector>

namespace render {

// Textura de reserva RGBA (lado × lado): quadrados de 8 px rosa #FF00FF e preto, rosa em (0, 0)
std::vector<uint8_t> gerarXadrez(int lado);

}
