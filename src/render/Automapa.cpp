#include "render/Automapa.h"

#include <cmath>

#include "mundo/Gerador.h"

namespace render {

namespace {

constexpr glm::vec3 COR_CHAO{0.35f, 0.33f, 0.30f};
constexpr glm::vec3 COR_COMPORTA{0.80f, 0.60f, 0.20f};
constexpr glm::vec3 COR_TRANCADA{0.80f, 0.20f, 0.20f};
constexpr glm::vec3 COR_ELEVADOR{0.30f, 0.80f, 0.40f};
constexpr glm::vec3 COR_CHAVE{0.30f, 0.80f, 0.90f};
constexpr glm::vec3 COR_JOGADOR{1.0f, 1.0f, 1.0f};

void retangulo(std::vector<VerticeCor>& v, glm::vec2 a, glm::vec2 b, glm::vec3 cor) {
    v.insert(v.end(), {{a, cor}, {{b.x, a.y}, cor}, {b, cor}, {a, cor}, {b, cor}, {{a.x, b.y}, cor}});
}

}

std::vector<VerticeCor> construirAutomapa(const mundo::Grade& g, const mundo::Exploracao& e, glm::vec2 posJogador,
                                          float angulo, std::optional<glm::vec2> chave, int telaL, int telaA) {
    constexpr float P = static_cast<float>(PIXELS_POR_CELULA);
    const float lado = mundo::LADO_ANDAR * P;
    const glm::vec2 origem{std::floor((telaL - lado) / 2), std::floor((telaA - lado) / 2)};
    auto naTela = [&](glm::vec2 celula) { return origem + celula * P; };

    std::vector<VerticeCor> v;
    retangulo(v, origem, origem + lado, glm::vec3(0.0f));

    for (int y = 0; y < g.altura(); ++y)
        for (int x = 0; x < g.largura(); ++x) {
            if (!e.vista(x, y)) continue;
            glm::vec3 cor;
            switch (g.em(x, y)) {
                case mundo::Celula::Parede: continue;
                case mundo::Celula::Comporta:
                case mundo::Celula::ComportaAberta: cor = COR_COMPORTA; break;
                case mundo::Celula::ComportaTrancada: cor = COR_TRANCADA; break;
                case mundo::Celula::Elevador: cor = COR_ELEVADOR; break;
                default: cor = COR_CHAO; break;
            }
            retangulo(v, naTela({x, y}), naTela({x + 1, y + 1}), cor);
        }

    if (chave && e.vista(static_cast<int>(chave->x), static_cast<int>(chave->y))) {
        const glm::vec2 c = naTela(*chave);
        retangulo(v, c - 1.5f, c + 1.5f, COR_CHAVE);
    }

    // Triângulo com o centro na posição do jogador, ponta na direção do olhar
    const glm::vec2 p = naTela(posJogador);
    const glm::vec2 frente{std::cos(angulo), std::sin(angulo)};
    const glm::vec2 lado90{-frente.y, frente.x};
    v.push_back({p + frente * 4.0f, COR_JOGADOR});
    v.push_back({p - frente * 2.0f + lado90 * 3.0f, COR_JOGADOR});
    v.push_back({p - frente * 2.0f - lado90 * 3.0f, COR_JOGADOR});
    return v;
}

}
