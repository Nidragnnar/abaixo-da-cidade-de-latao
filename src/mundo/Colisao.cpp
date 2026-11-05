#include "mundo/Colisao.h"

#include "mundo/Terreno.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace mundo {

namespace {

constexpr float FOLGA = 0.001f;  // afasta da parede para o próximo teste não acusar contato

// Célula que a caixa não pode ocupar: parede, comporta fechada, elevador, chão alto demais ou teto baixo demais
// (na rampa, o chão no ponto da caixa mais perto dela)
bool bloqueia(const Grade& g, int x, int y, glm::vec2 centro, float pes, float subida) {
    if (g.bloqueiaPassagem(x, y)) return true;
    const glm::vec2 perto = glm::clamp(centro, glm::vec2(x, y), glm::vec2(x + 1, y + 1) - 1e-4f);
    if (alturaDoChao(g, perto) > pes + subida + 1e-3f) return true;
    return alturaDoTeto(g, perto) < pes + ALTURA_CORPO - 1e-3f;
}

// Move só num eixo (0 = X, 1 = Y) e encosta na primeira célula sólida tocada
void moverEixo(const Grade& grade, glm::vec2& p, float delta, int eixo, float raio, float pes, float subida) {
    if (delta == 0.0f) return;
    glm::vec2 novo = p;
    novo[eixo] += delta;

    const int x0 = static_cast<int>(std::floor(novo.x - raio));
    const int x1 = static_cast<int>(std::floor(novo.x + raio));
    const int y0 = static_cast<int>(std::floor(novo.y - raio));
    const int y1 = static_cast<int>(std::floor(novo.y + raio));

    // Células que a caixa já ocupava: só parede barra (o chão da rampa sob o corpo sobe à frente; o pouso e o
    // degrau são do movimento vertical, não da colisão)
    auto jaOcupava = [&](int x, int y) {
        return x >= static_cast<int>(std::floor(p.x - raio)) && x <= static_cast<int>(std::floor(p.x + raio)) &&
               y >= static_cast<int>(std::floor(p.y - raio)) && y <= static_cast<int>(std::floor(p.y + raio));
    };

    // Na rampa a frente da caixa já está mais alta que os pés (no centro): o degrau à frente se mede a partir dela
    float referencia = pes;
    for (const glm::vec2 canto : {glm::vec2(-raio, -raio), glm::vec2(raio, -raio), glm::vec2(-raio, raio), glm::vec2(raio, raio)})
        if (grade.rampa(static_cast<int>(std::floor(p.x + canto.x)), static_cast<int>(std::floor(p.y + canto.y))) != Rampa::Nenhuma)
            referencia = std::max(referencia, std::min(alturaDoChao(grade, p + canto), pes + subida));

    bool bateu = false;
    // Célula sólida mais próxima no sentido do movimento
    int limite = delta > 0 ? std::numeric_limits<int>::max() : std::numeric_limits<int>::min();
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            if (jaOcupava(x, y) ? grade.bloqueiaPassagem(x, y) : bloqueia(grade, x, y, novo, referencia, subida)) {
                const int c = eixo == 0 ? x : y;
                limite = delta > 0 ? std::min(limite, c) : std::max(limite, c);
                bateu = true;
            }

    if (bateu) novo[eixo] = delta > 0 ? limite - raio - FOLGA : limite + 1 + raio + FOLGA;
    p = novo;
}

}

glm::vec2 moverComColisao(const Grade& grade, glm::vec2 posicao, glm::vec2 deslocamento, float raio, float pes,
                          float subida) {
    const float distancia = glm::length(deslocamento);
    if (distancia == 0.0f) return posicao;
    const int subpassos = std::max(1, static_cast<int>(std::ceil(distancia / raio)));
    const glm::vec2 parte = deslocamento / static_cast<float>(subpassos);
    for (int i = 0; i < subpassos; ++i) {
        moverEixo(grade, posicao, parte.x, 0, raio, pes, subida);
        moverEixo(grade, posicao, parte.y, 1, raio, pes, subida);
    }
    return posicao;
}

}
