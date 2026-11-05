#pragma once
#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "jogo/Inimigos.h"
#include "mundo/Grade.h"

namespace jogo {

constexpr float VELOCIDADE_PROJETIL = 10.0f;  // células por segundo (visível, dá para desviar)
constexpr float RAIO_PROJETIL = 0.1f;

// Bomba do dirigível: nasce parada, cai e explode em área
constexpr float GRAVIDADE_BOMBA = 9.0f;   // blocos/s²
constexpr float RAIO_BOMBA = 1.0f;        // dano cheio no centro, nada a partir daqui
constexpr float INTERVALO_BOMBA = 2.5f;   // segundos entre bombas (a recuperação do dirigível)
constexpr float ALCANCE_BOMBA = 2.0f;     // distância horizontal em que o dirigível solta
constexpr float ALTURA_MINIMA_BOMBA = 1.0f;  // acima dos pés do jogador: mais baixo a bomba não dá tempo de fugir

struct Projetil {
    glm::vec3 posicao;    // (x, altura, z)
    glm::vec3 velocidade;
    int dano;  // bruto; a defesa do jogador é aplicada pela Partida
    TipoInimigo autor;
    float gravidade = 0;     // 0 nos tiros (voam retos)
    float raioExplosao = 0;  // 0 nos tiros; > 0: explode ao bater e fere pela distância
};

struct ResultadoProjeteis {
    std::vector<Golpe> golpes;        // um por projétil que acertou
    std::vector<glm::vec3> explosoes;  // onde as bombas explodiram (som e clarão)
};

// Move em subpassos de no máximo 0,1; some na parede, abaixo do chão, acima do teto ou ao acertar o jogador
// (corpo de 0,7 de altura em volta de centroJogador). A bomba explode nesses mesmos casos: dano × (1 − d / raio),
// com d a distância do ponto da explosão ao centro do jogador, atravessando paredes
ResultadoProjeteis atualizarProjeteis(std::vector<Projetil>& p, const mundo::Grade& g, glm::vec3 centroJogador,
                                      float raioJogador, float dt);

}
