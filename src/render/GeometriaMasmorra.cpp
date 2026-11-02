#include "render/GeometriaMasmorra.h"

#include <glm/geometric.hpp>

namespace render {

int camadaParede(mundo::Zona z) { return 3 * static_cast<int>(z); }
int camadaChao(mundo::Zona z) { return 3 * static_cast<int>(z) + 1; }
int camadaTeto(mundo::Zona z) { return 3 * static_cast<int>(z) + 2; }

void adicionarQuadrado(std::vector<Vertice>& v, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float camada,
                       glm::vec3 normal) {
    const Vertice va{a, {0, 1}, camada, normal}, vb{b, {1, 1}, camada, normal};
    const Vertice vc{c, {1, 0}, camada, normal}, vd{d, {0, 0}, camada, normal};
    v.insert(v.end(), {va, vb, vc, va, vc, vd});
}

namespace {

constexpr glm::vec3 CIMA{0.0f, 1.0f, 0.0f};

// Parede vista da célula aberta: p0 embaixo à esquerda, p1 embaixo à direita
void parede(std::vector<Vertice>& v, glm::vec3 p0, glm::vec3 p1, float camada) {
    const glm::vec3 normal = glm::cross(p1 - p0, CIMA);
    adicionarQuadrado(v, p0, p1, p1 + CIMA, p0 + CIMA, camada, normal);
}

}

std::vector<Vertice> construirGeometria(const mundo::Grade& g, mundo::Zona z) {
    const float cParede = static_cast<float>(camadaParede(z));
    const float cChao = static_cast<float>(camadaChao(z));
    const float cTeto = static_cast<float>(camadaTeto(z));
    // Comportas e elevador têm chão e teto; só paredes de verdade bloqueiam a malha
    auto ehParede = [&](int x, int y) { return g.em(x, y) == mundo::Celula::Parede; };

    std::vector<Vertice> v;
    for (int y = 0; y < g.altura(); ++y) {
        for (int x = 0; x < g.largura(); ++x) {
            if (ehParede(x, y)) continue;
            const float fx = static_cast<float>(x), fz = static_cast<float>(y);

            // Chão (visto de cima) e teto (visto de baixo)
            adicionarQuadrado(v, {fx, 0, fz}, {fx, 0, fz + 1}, {fx + 1, 0, fz + 1}, {fx + 1, 0, fz}, cChao, CIMA);
            adicionarQuadrado(v, {fx, 1, fz + 1}, {fx, 1, fz}, {fx + 1, 1, fz}, {fx + 1, 1, fz + 1}, cTeto, -CIMA);

            // Uma face para cada vizinha que é parede, virada para esta célula
            if (ehParede(x - 1, y)) parede(v, {fx, 0, fz + 1}, {fx, 0, fz}, cParede);
            if (ehParede(x + 1, y)) parede(v, {fx + 1, 0, fz}, {fx + 1, 0, fz + 1}, cParede);
            if (ehParede(x, y - 1)) parede(v, {fx, 0, fz}, {fx + 1, 0, fz}, cParede);
            if (ehParede(x, y + 1)) parede(v, {fx + 1, 0, fz + 1}, {fx, 0, fz + 1}, cParede);
        }
    }
    return v;
}

}
