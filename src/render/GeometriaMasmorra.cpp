#include "render/GeometriaMasmorra.h"

#include <algorithm>
#include <array>

#include <glm/geometric.hpp>

#include "mundo/Terreno.h"

namespace render {

int camadaParede(mundo::Zona z) { return 3 * static_cast<int>(z); }
int camadaChao(mundo::Zona z) { return 3 * static_cast<int>(z) + 1; }
int camadaTeto(mundo::Zona z) { return 3 * static_cast<int>(z) + 2; }

void adicionarQuadradoUV(std::vector<Vertice>& v, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec2 ua,
                         glm::vec2 ub, glm::vec2 uc, glm::vec2 ud, float camada, glm::vec3 normal) {
    const Vertice va{a, ua, camada, normal}, vb{b, ub, camada, normal};
    const Vertice vc{c, uc, camada, normal}, vd{d, ud, camada, normal};
    v.insert(v.end(), {va, vb, vc, va, vc, vd});
}

void adicionarQuadrado(std::vector<Vertice>& v, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float camada,
                       glm::vec3 normal) {
    adicionarQuadradoUV(v, a, b, c, d, {0, 1}, {1, 1}, {1, 0}, {0, 0}, camada, normal);
}

namespace {

constexpr glm::vec3 CIMA{0.0f, 1.0f, 0.0f};
constexpr float QUASE_ZERO = 1e-4f;

// Altura do chão da célula num dos seus cantos (u, w ∈ {0, 1}); rampa inclinada
float chaoNoCanto(const mundo::Grade& g, int x, int y, int u, int w) {
    const float base = g.chao(x, y) * mundo::QUARTO;
    float t = 0;
    switch (g.rampa(x, y)) {
        case mundo::Rampa::Leste: t = static_cast<float>(u); break;
        case mundo::Rampa::Oeste: t = 1.0f - static_cast<float>(u); break;
        case mundo::Rampa::Sul: t = static_cast<float>(w); break;
        case mundo::Rampa::Norte: t = 1.0f - static_cast<float>(w); break;
        default: return base;
    }
    return base + t;  // a rampa sobe 1 bloco
}

// Uma aresta da célula: os dois cantos (vistos de dentro da célula, da esquerda para a direita) e o vizinho
struct Aresta {
    glm::ivec2 vizinho;
    std::array<glm::ivec2, 2> cantos;  // (u, w) de cada ponta
};
// Esquerda/direita para quem olha de dentro da célula para a aresta (normal da face aponta para dentro)
constexpr Aresta ARESTAS[] = {
    {{-1, 0}, {{{0, 1}, {0, 0}}}},  // oeste
    {{1, 0}, {{{1, 0}, {1, 1}}}},   // leste
    {{0, -1}, {{{0, 0}, {1, 0}}}},  // norte
    {{0, 1}, {{{1, 1}, {0, 1}}}},   // sul
};

// Face vertical na aresta entre baixo[i] e cima[i] de cada ponta; a normal aponta para dentro da célula.
// v = −altura (a textura repete a cada bloco, de cima para baixo); u = posição ao longo da aresta.
void faceVertical(std::vector<Vertice>& v, glm::vec2 p0, glm::vec2 p1, float baixo0, float baixo1, float cima0,
                  float cima1, float camada) {
    if (cima0 - baixo0 < QUASE_ZERO && cima1 - baixo1 < QUASE_ZERO) return;
    const glm::vec3 a{p0.x, baixo0, p0.y}, b{p1.x, baixo1, p1.y}, c{p1.x, cima1, p1.y}, d{p0.x, cima0, p0.y};
    const glm::vec3 normal = glm::normalize(glm::cross(b - a, CIMA));
    adicionarQuadradoUV(v, a, b, c, d, {0, -baixo0}, {1, -baixo1}, {1, -cima1}, {0, -cima0}, camada, normal);
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
            const float teto = g.teto(x, y) * mundo::QUARTO;
            auto canto = [&](int u, int w) { return chaoNoCanto(g, x, y, u, w); };
            const float camadaDoChao = g.detalhe(x, y) == mundo::Detalhe::Caixa ? static_cast<float>(CamadaCaixa) : cChao;

            // Chão (visto de cima, inclinado na rampa) e teto (visto de baixo)
            const glm::vec3 a{fx, canto(0, 0), fz}, b{fx, canto(0, 1), fz + 1}, c{fx + 1, canto(1, 1), fz + 1}, d{fx + 1, canto(1, 0), fz};
            adicionarQuadrado(v, a, b, c, d, camadaDoChao, glm::normalize(glm::cross(b - a, d - a)));
            adicionarQuadrado(v, {fx, teto, fz + 1}, {fx, teto, fz}, {fx + 1, teto, fz}, {fx + 1, teto, fz + 1}, cTeto, -CIMA);

            for (const Aresta& ar : ARESTAS) {
                const int nx = x + ar.vizinho.x, ny = y + ar.vizinho.y;
                const glm::ivec2 c0 = ar.cantos[0], c1 = ar.cantos[1];
                const glm::vec2 p0{fx + c0.x, fz + c0.y}, p1{fx + c1.x, fz + c1.y};
                const float meu0 = canto(c0.x, c0.y), meu1 = canto(c1.x, c1.y);
                if (ehParede(nx, ny)) {
                    faceVertical(v, p0, p1, meu0, meu1, teto, teto, cParede);  // parede do chão ao teto
                    continue;
                }
                // O mesmo canto visto do vizinho
                auto doVizinho = [&](glm::ivec2 cc) { return chaoNoCanto(g, nx, ny, cc.x - ar.vizinho.x, cc.y - ar.vizinho.y); };
                const float viz0 = doVizinho(c0), viz1 = doVizinho(c1);
                // Frente do degrau: onde o vizinho é mais alto, desenhada deste lado (o de baixo)
                float camadaDegrau = cParede;
                if (g.detalhe(nx, ny) == mundo::Detalhe::Caixa) camadaDegrau = static_cast<float>(CamadaCaixa);
                else if (g.detalhe(nx, ny) == mundo::Detalhe::Escada || g.detalhe(x, y) == mundo::Detalhe::Escada)
                    camadaDegrau = static_cast<float>(CamadaDegrau);
                faceVertical(v, p0, p1, meu0, meu1, std::max(meu0, viz0), std::max(meu1, viz1), camadaDegrau);
                // Beiral: o teto do vizinho é mais baixo; a face fica deste lado (o de teto mais alto)
                const float tetoViz = g.teto(nx, ny) * mundo::QUARTO;
                if (tetoViz < teto) faceVertical(v, p0, p1, tetoViz, tetoViz, teto, teto, cParede);
            }
        }
    }
    return v;
}

}
