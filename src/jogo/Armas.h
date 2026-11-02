#pragma once
#include <random>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Comandos.h"
#include "jogo/Inimigos.h"
#include "mundo/Grade.h"

namespace jogo {

enum class ArmaAtiva { ChaveInglesa, Revolver };

constexpr int CARGA_REVOLVER = 6;
constexpr int RESERVA_MAXIMA_BALAS = 48;
constexpr float DURACAO_ANIMACAO_ARMA = 0.2f;

struct Armas {
    ArmaAtiva ativa = ArmaAtiva::ChaveInglesa;
    int carga = CARGA_REVOLVER;
    int reserva = 12;
    float intervalo = 0;  // espera até o próximo ataque
    float recarga = 0;    // > 0 enquanto recarrega
    float animacao = 0;   // > 0 enquanto o golpe/tiro aparece na mão
};

struct ResultadoAtaque {
    int acertos = 0;
    int destruidos = 0;
    float barulho = 0;  // alcance do barulho (acorda inimigos parados)
    bool disparou = false;
    std::string mensagem;
};

// Troca, recarga e ataque da arma ativa (seção 4 da spec); a inclinação do olhar mira o revólver na vertical
ResultadoAtaque atualizarArmas(Armas& a, const Comandos& c, glm::vec2 posicao, float angulo, const mundo::Grade& g,
                               std::vector<Inimigo>& inimigos, std::mt19937& rng, float dt, float inclinacao = 0.0f);

}
