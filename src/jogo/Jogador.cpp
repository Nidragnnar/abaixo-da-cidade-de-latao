#include "jogo/Jogador.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

#include "mundo/Colisao.h"

namespace jogo {

void atualizarJogador(Jogador& j, const Comandos& c, const mundo::Grade& g, float dt) {
    j.angulo += c.giro;
    j.inclinacao = std::clamp(j.inclinacao + c.inclinar, -INCLINACAO_MAXIMA, INCLINACAO_MAXIMA);

    glm::vec2 intencao{c.frente, c.lado};
    if (glm::length(intencao) > 1.0f) intencao = glm::normalize(intencao);  // diagonal não é mais rápida

    const glm::vec2 frente{std::cos(j.angulo), std::sin(j.angulo)};
    const glm::vec2 direita{-std::sin(j.angulo), std::cos(j.angulo)};
    const glm::vec2 deslocamento = (frente * intencao.x + direita * intencao.y) * VELOCIDADE_ANDAR * dt;
    j.posicao = mundo::moverComColisao(g, j.posicao, deslocamento, RAIO_JOGADOR);
}

}
