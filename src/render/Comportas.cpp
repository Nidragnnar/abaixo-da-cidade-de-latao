#include "render/Comportas.h"

#include <glm/geometric.hpp>

namespace render {

namespace {

// Face erguida em `base`: p0 embaixo à esquerda, p1 embaixo à direita (vista de quem olha a frente)
void face(std::vector<Vertice>& v, glm::vec3 p0, glm::vec3 p1, float camada) {
    const glm::vec3 cima{0.0f, 1.0f, 0.0f};
    adicionarQuadrado(v, p0, p1, p1 + cima, p0 + cima, camada, glm::cross(p1 - p0, cima));
}

}

std::vector<Vertice> construirComportas(const mundo::Grade& g, const std::function<float(int, int)>& abertura) {
    std::vector<Vertice> v;
    for (int y = 0; y < g.altura(); ++y)
        for (int x = 0; x < g.largura(); ++x) {
            const mundo::Celula c = g.em(x, y);
            if (c != mundo::Celula::Comporta && c != mundo::Celula::ComportaTrancada) continue;
            const float camada = static_cast<float>(c == mundo::Celula::Comporta ? CamadaComporta : CamadaComportaTrancada);
            const float base = abertura(x, y);
            const float fx = static_cast<float>(x), fz = static_cast<float>(y);
            // Paredes em cima e embaixo na grade → passagem em X → placa no plano X = x + 0,5
            const bool passagemEmX = g.em(x, y - 1) == mundo::Celula::Parede && g.em(x, y + 1) == mundo::Celula::Parede;
            if (passagemEmX) {
                face(v, {fx + 0.5f, base, fz}, {fx + 0.5f, base, fz + 1}, camada);  // vista de -X
                face(v, {fx + 0.5f, base, fz + 1}, {fx + 0.5f, base, fz}, camada);  // vista de +X
            } else {
                face(v, {fx + 1, base, fz + 0.5f}, {fx, base, fz + 0.5f}, camada);  // vista de -Z
                face(v, {fx, base, fz + 0.5f}, {fx + 1, base, fz + 0.5f}, camada);  // vista de +Z
            }
        }
    return v;
}

}
