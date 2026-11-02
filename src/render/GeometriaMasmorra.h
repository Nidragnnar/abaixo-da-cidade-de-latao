#pragma once
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "mundo/Grade.h"
#include "mundo/Zona.h"

namespace render {

// Camadas do array de texturas: parede, chão e teto de cada zona, depois as comportas
enum CamadaTextura : int {
    ParedeEsgoto, ChaoEsgoto, TetoEsgoto,
    ParedeFabrica, ChaoFabrica, TetoFabrica,
    ParedeMinas, ChaoMinas, TetoMinas,
    CamadaComporta, CamadaComportaTrancada,
    TOTAL_CAMADAS
};

int camadaParede(mundo::Zona z);
int camadaChao(mundo::Zona z);
int camadaTeto(mundo::Zona z);

struct Vertice {
    glm::vec3 posicao;
    glm::vec2 uv;
    float camada;
    glm::vec3 normal;
};

// Quadrado a, b, c, d em sentido anti-horário visto de quem olha a face da frente (6 vértices)
void adicionarQuadrado(std::vector<Vertice>& v, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float camada,
                       glm::vec3 normal);

// Chão e teto de toda célula que não é parede; faces só contra paredes (6 vértices por quadrado)
std::vector<Vertice> construirGeometria(const mundo::Grade& g, mundo::Zona z);

}
