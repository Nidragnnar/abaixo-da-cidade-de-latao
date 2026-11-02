#pragma once
#include <optional>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Inimigos.h"
#include "mundo/Grade.h"

namespace jogo {

constexpr float VELOCIDADE_PROJETIL = 10.0f;  // células por segundo (visível, dá para desviar)
constexpr float RAIO_PROJETIL = 0.1f;

struct Projetil {
    glm::vec2 posicao;
    glm::vec2 velocidade;
    int dano;  // bruto; a defesa do jogador é aplicada pela Partida
    TipoInimigo autor;
};

struct ResultadoProjeteis {
    std::vector<Golpe> golpes;  // um por projétil que acertou
};

// Move em subpassos de no máximo 0,1; some na parede ou ao acertar o jogador
ResultadoProjeteis atualizarProjeteis(std::vector<Projetil>& p, const mundo::Grade& g, glm::vec2 jogador,
                                      float raioJogador, float dt);

}
