#pragma once
#include <vector>

#include <glm/vec3.hpp>

#include "mundo/Luzes.h"

namespace render {

constexpr int MAXIMO_LUZES = 16;            // as que vão para o shader a cada quadro
constexpr float INTENSIDADE_LAMPIAO = 2.0f;  // a atenuação ao quadrado apaga rápido; isto clareia a sala em volta

// As até 16 luzes mais perto do ponto, da mais perto para a mais longe
std::vector<mundo::Luz> luzesMaisProximas(const std::vector<mundo::Luz>& luzes, glm::vec3 ponto);

// Brilho de chama: 0,9 … 1,1, com ritmo próprio pela fase
float tremulacao(float tempo, float fase);

}
