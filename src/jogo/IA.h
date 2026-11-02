#pragma once
#include <optional>
#include <random>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Inimigos.h"
#include "jogo/Projeteis.h"
#include "mundo/Grade.h"

namespace jogo {

constexpr float ALCANCE_VISAO = 10.0f;
constexpr float RECALCULO_CAMINHO = 0.5f;  // segundos entre buscas em largura
constexpr float INICIO_GOLPE = 0.9f;       // distância em que o corpo a corpo começa a preparar
constexpr float ALCANCE_GOLPE = 1.1f;      // distância máxima para o golpe acertar no fim do preparo
constexpr float RAIO_INIMIGO = 0.25f;
constexpr float TEMPO_DESTROCOS = 10.0f;
constexpr float DISTANCIA_MIN_ATIRADOR = 3.0f, DISTANCIA_MAX_ATIRADOR = 6.0f;

struct MundoIA {
    mundo::Grade& grade;
    glm::vec2 jogador;
    const std::vector<int>& campo;  // distanciasNaGrade a partir da célula do jogador, sem atravessar tranca nem elevador
    std::mt19937& rng;
    float raioJogador = 0.25f;  // inimigos não entram no jogador
};

struct EventosIA {
    int dano = 0;  // bruto, antes da defesa do jogador
    std::optional<TipoInimigo> autor;
    std::vector<glm::ivec2> comportasParaAbrir;
    std::vector<Projetil> tiros;
};

// Parado → Perseguindo para quem estiver no alcance do barulho, mesmo sem linha de visão
void acordarPorBarulho(std::vector<Inimigo>& v, glm::vec2 origem, float alcance);

EventosIA atualizarInimigos(std::vector<Inimigo>& v, const MundoIA& m, float dt);

}
