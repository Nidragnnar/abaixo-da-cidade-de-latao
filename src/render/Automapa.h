#pragma once
#include <optional>
#include <vector>

#include <glm/vec2.hpp>

#include "mundo/Consultas.h"
#include "mundo/Grade.h"
#include "render/Primitivas2D.h"

namespace render {

constexpr int PIXELS_POR_CELULA = 4;

// Painel preto do tamanho de um andar (64 × 4 px) centralizado, células vistas coloridas,
// a válvula-chave (se a célula dela foi vista) e o jogador como triângulo branco, por último
std::vector<VerticeCor> construirAutomapa(const mundo::Grade& g, const mundo::Exploracao& e, glm::vec2 posJogador,
                                          float angulo, std::optional<glm::vec2> chave, int telaL, int telaA);

}
