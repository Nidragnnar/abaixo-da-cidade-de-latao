#pragma once
#include <glm/vec2.hpp>

#include "mundo/Grade.h"

namespace mundo {

// Move uma caixa de meia-largura `raio` pela grade, deslizando nas células sólidas.
// Divide o deslocamento em subpassos para não atravessar paredes.
glm::vec2 moverComColisao(const Grade& grade, glm::vec2 posicao, glm::vec2 deslocamento, float raio);

}
