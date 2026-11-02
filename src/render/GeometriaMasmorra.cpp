#include "render/GeometriaMasmorra.h"

#include <glm/geometric.hpp>

namespace render {

namespace {

constexpr glm::vec3 CIMA{0.0f, 1.0f, 0.0f};

// Quadrado a, b, c, d em sentido anti-horário visto de quem olha a face da frente
void quadrado(std::vector<Vertice>& v, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float camada,
              glm::vec3 normal) {
    const Vertice va{a, {0, 1}, camada, normal}, vb{b, {1, 1}, camada, normal};
    const Vertice vc{c, {1, 0}, camada, normal}, vd{d, {0, 0}, camada, normal};
    v.insert(v.end(), {va, vb, vc, va, vc, vd});
}

// Parede vista da célula aberta: p0 embaixo à esquerda, p1 embaixo à direita
void parede(std::vector<Vertice>& v, glm::vec3 p0, glm::vec3 p1) {
    const glm::vec3 normal = glm::cross(p1 - p0, CIMA);
    quadrado(v, p0, p1, p1 + CIMA, p0 + CIMA, ParedeEsgoto, normal);
}

}

std::vector<Vertice> construirGeometria(const mundo::Grade& g) {
    std::vector<Vertice> v;
    for (int y = 0; y < g.altura(); ++y) {
        for (int x = 0; x < g.largura(); ++x) {
            if (g.solida(x, y)) continue;
            const float fx = static_cast<float>(x), fz = static_cast<float>(y);

            // Chão (visto de cima) e teto (visto de baixo)
            quadrado(v, {fx, 0, fz}, {fx, 0, fz + 1}, {fx + 1, 0, fz + 1}, {fx + 1, 0, fz}, ChaoEsgoto, CIMA);
            quadrado(v, {fx, 1, fz + 1}, {fx, 1, fz}, {fx + 1, 1, fz}, {fx + 1, 1, fz + 1}, TetoEsgoto, -CIMA);

            // Uma face para cada vizinha sólida, virada para esta célula
            if (g.solida(x - 1, y)) parede(v, {fx, 0, fz + 1}, {fx, 0, fz});
            if (g.solida(x + 1, y)) parede(v, {fx + 1, 0, fz}, {fx + 1, 0, fz + 1});
            if (g.solida(x, y - 1)) parede(v, {fx, 0, fz}, {fx + 1, 0, fz});
            if (g.solida(x, y + 1)) parede(v, {fx + 1, 0, fz + 1}, {fx, 0, fz + 1});
        }
    }
    return v;
}

}
