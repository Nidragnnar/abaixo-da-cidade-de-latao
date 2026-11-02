#pragma once
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Comandos.h"

namespace jogo {

class Partida;

// Piloto automático do --demo (seção 12 da spec): vai até a válvula-chave (se o andar tiver tranca e ela ainda
// estiver no chão) e depois ao elevador, abrindo comportas e batendo em quem estiver na frente
class PilotoDemo {
public:
    Comandos comandar(const Partida& p, float dt);

private:
    std::vector<int> campo_;                  // distâncias até o alvo
    glm::ivec2 alvoDoCampo_{-1, -1};
    int andarDoCampo_ = -1;
    bool trancaDoCampo_ = false;
    float desdeUsar_ = 1.0f;
    float relogioPreso_ = 0.0f;               // tempo desde a última conferência de movimento
    glm::vec2 posicaoConferida_{-1e9f, -1e9f};
    float desvio_ = 0.0f;                      // > 0 enquanto anda de lado para soltar
    float lado_ = 1.0f;
};

}
