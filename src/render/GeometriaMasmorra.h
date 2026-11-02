#pragma once
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "mundo/Grade.h"

namespace render {

// Camadas do array de texturas
enum CamadaTextura : int { ParedeEsgoto = 0, ChaoEsgoto = 1, TetoEsgoto = 2 };

struct Vertice {
    glm::vec3 posicao;
    glm::vec2 uv;
    float camada;
    glm::vec3 normal;
};

// Chão, teto e faces de parede visíveis de um andar (6 vértices por quadrado)
std::vector<Vertice> construirGeometria(const mundo::Grade& g);

}
