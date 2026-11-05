#pragma once
#include <glm/vec2.hpp>

#include "mundo/Grade.h"

namespace mundo {

// Move uma caixa de meia-largura `raio` pela grade, deslizando nas células sólidas e nas que o corpo não alcança:
// chão acima de pés + subida (subida 0 no ar) ou teto abaixo de pés + ALTURA_CORPO.
// Divide o deslocamento em subpassos para não atravessar paredes.
glm::vec2 moverComColisao(const Grade& grade, glm::vec2 posicao, glm::vec2 deslocamento, float raio, float pes = 0.0f,
                          float subida = 0.25f);

}
