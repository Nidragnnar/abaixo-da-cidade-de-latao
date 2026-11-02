#include "render/Automapa.h"

#include <cmath>

#include "mundo/Gerador.h"

namespace render {

namespace {

constexpr glm::vec4 COR_PAINEL{0.0f, 0.0f, 0.0f, 1.0f};
constexpr glm::vec4 COR_CHAO{0.35f, 0.33f, 0.30f, 1.0f};
constexpr glm::vec4 COR_COMPORTA{0.80f, 0.60f, 0.20f, 1.0f};
constexpr glm::vec4 COR_TRANCADA{0.80f, 0.20f, 0.20f, 1.0f};
constexpr glm::vec4 COR_ELEVADOR{0.30f, 0.80f, 0.40f, 1.0f};
constexpr glm::vec4 COR_CHAVE{0.30f, 0.80f, 0.90f, 1.0f};
constexpr glm::vec4 COR_JOGADOR{1.0f, 1.0f, 1.0f, 1.0f};

}

std::vector<VerticeCor> construirAutomapa(const mundo::Grade& g, const mundo::Exploracao& e, glm::vec2 posJogador,
                                          float angulo, std::optional<glm::vec2> chave, int telaL, int telaA) {
    constexpr float P = static_cast<float>(PIXELS_POR_CELULA);
    const float lado = mundo::LADO_ANDAR * P;
    const glm::vec2 origem{std::floor((telaL - lado) / 2), std::floor((telaA - lado) / 2)};
    auto naTela = [&](glm::vec2 celula) { return origem + celula * P; };

    std::vector<VerticeCor> v;
    adicionarRetangulo(v, origem, origem + lado, COR_PAINEL);

    for (int y = 0; y < g.altura(); ++y)
        for (int x = 0; x < g.largura(); ++x) {
            if (!e.vista(x, y)) continue;
            glm::vec4 cor;
            switch (g.em(x, y)) {
                case mundo::Celula::Parede: continue;
                case mundo::Celula::Comporta:
                case mundo::Celula::ComportaAberta: cor = COR_COMPORTA; break;
                case mundo::Celula::ComportaTrancada: cor = COR_TRANCADA; break;
                case mundo::Celula::Elevador: cor = COR_ELEVADOR; break;
                default: cor = COR_CHAO; break;
            }
            adicionarRetangulo(v, naTela({x, y}), naTela({x + 1, y + 1}), cor);
        }

    if (chave && e.vista(static_cast<int>(chave->x), static_cast<int>(chave->y))) {
        const glm::vec2 c = naTela(*chave);
        adicionarRetangulo(v, c - 1.5f, c + 1.5f, COR_CHAVE);
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
