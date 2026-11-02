#pragma once
#include <glm/vec2.hpp>

#include "jogo/Inimigos.h"
#include "render/Sprites.h"

namespace render {

// Quadros de cada inimigo no array de sprites: parado, andar 1, andar 2, atacar 1, atacar 2, morrer, destroços
constexpr int QUADROS_POR_INIMIGO = 7;

// Andar alterna a cada 0,25 s; Preparando = atacar 1; Recuperando = atacar 2;
// Destruído: "morrer" nos primeiros 0,3 s e depois os destroços
int camadaDoInimigo(jogo::TipoInimigo t, jogo::EstadoInimigo e, float tempoNoEstado);

// Largura e altura do sprite em células
glm::vec2 tamanhoDoInimigo(jogo::TipoInimigo t);

}
