#include "mundo/Colisao.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/geometric.hpp>

namespace mundo {

namespace {

constexpr float FOLGA = 0.001f;  // afasta da parede para o próximo teste não acusar contato

// Move só num eixo (0 = X, 1 = Y) e encosta na primeira célula sólida tocada
void moverEixo(const Grade& grade, glm::vec2& p, float delta, int eixo, float raio) {
    if (delta == 0.0f) return;
    glm::vec2 novo = p;
    novo[eixo] += delta;

    const int x0 = static_cast<int>(std::floor(novo.x - raio));
    const int x1 = static_cast<int>(std::floor(novo.x + raio));
    const int y0 = static_cast<int>(std::floor(novo.y - raio));
    const int y1 = static_cast<int>(std::floor(novo.y + raio));

    bool bateu = false;
    // Célula sólida mais próxima no sentido do movimento
    int limite = delta > 0 ? std::numeric_limits<int>::max() : std::numeric_limits<int>::min();
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            if (grade.solida(x, y)) {
                const int c = eixo == 0 ? x : y;
                limite = delta > 0 ? std::min(limite, c) : std::max(limite, c);
                bateu = true;
            }

    if (bateu) novo[eixo] = delta > 0 ? limite - raio - FOLGA : limite + 1 + raio + FOLGA;
    p = novo;
}

}

glm::vec2 moverComColisao(const Grade& grade, glm::vec2 posicao, glm::vec2 deslocamento, float raio) {
    const float distancia = glm::length(deslocamento);
    if (distancia == 0.0f) return posicao;
    const int subpassos = std::max(1, static_cast<int>(std::ceil(distancia / raio)));
    const glm::vec2 parte = deslocamento / static_cast<float>(subpassos);
    for (int i = 0; i < subpassos; ++i) {
        moverEixo(grade, posicao, parte.x, 0, raio);
        moverEixo(grade, posicao, parte.y, 1, raio);
    }
    return posicao;
}

}
