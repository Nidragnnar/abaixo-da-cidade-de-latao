#pragma once
#include <glm/vec2.hpp>

#include "jogo/Comandos.h"
#include "mundo/Grade.h"

namespace jogo {

constexpr float VELOCIDADE_ANDAR = 3.0f;  // células por segundo
constexpr float RAIO_JOGADOR = 0.25f;
constexpr float INCLINACAO_MAXIMA = 1.0471976f;  // 60° para cima ou para baixo
constexpr float ALTURA_OLHOS = 0.5f;             // de onde sai o olhar e o tiro

struct Jogador {
    glm::vec2 posicao;     // (X, Z) do mundo = (x, y) da grade
    float angulo = 0;      // 0 olha para +X; cresce girando para a direita
    float inclinacao = 0;  // olhar vertical; positivo = para cima
};

void atualizarJogador(Jogador& j, const Comandos& c, const mundo::Grade& g, float dt);

}
