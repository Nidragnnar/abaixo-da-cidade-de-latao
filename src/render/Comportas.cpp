#include "render/Comportas.h"

#include <glm/geometric.hpp>

#include "mundo/Terreno.h"

namespace render {

namespace {

// Face erguida em `base`, da altura da passagem: p0 embaixo à esquerda, p1 embaixo à direita (vista de quem olha a frente)
void face(std::vector<Vertice>& v, glm::vec3 p0, glm::vec3 p1, float camada, float altura) {
    const glm::vec3 cima{0.0f, altura, 0.0f};
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
            // Do chão ao teto da célula; abrir sobe a placa para dentro do teto
            const float chao = g.chao(x, y) * mundo::QUARTO, altura = (g.teto(x, y) - g.chao(x, y)) * mundo::QUARTO;
            const float base = chao + abertura(x, y) * altura;
            const float fx = static_cast<float>(x), fz = static_cast<float>(y);
            // Paredes em cima e embaixo na grade → passagem em X → placa no plano X = x + 0,5
            const bool passagemEmX = g.em(x, y - 1) == mundo::Celula::Parede && g.em(x, y + 1) == mundo::Celula::Parede;
            if (passagemEmX) {
                face(v, {fx + 0.5f, base, fz}, {fx + 0.5f, base, fz + 1}, camada, altura);  // vista de -X
                face(v, {fx + 0.5f, base, fz + 1}, {fx + 0.5f, base, fz}, camada, altura);  // vista de +X
            } else {
                face(v, {fx + 1, base, fz + 0.5f}, {fx, base, fz + 0.5f}, camada, altura);  // vista de -Z
                face(v, {fx, base, fz + 0.5f}, {fx + 1, base, fz + 0.5f}, camada, altura);  // vista de +Z
            }
        }
    return v;
}

}
