#include "jogo/Projeteis.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace jogo {

namespace {
constexpr float SUBPASSO = 0.1f;
}

ResultadoProjeteis atualizarProjeteis(std::vector<Projetil>& p, const mundo::Grade& g, glm::vec2 jogador,
                                      float raioJogador, float dt) {
    ResultadoProjeteis r;
    std::erase_if(p, [&](Projetil& t) {
        const glm::vec2 deslocamento = t.velocidade * dt;
        const int n = std::max(1, static_cast<int>(std::ceil(glm::length(deslocamento) / SUBPASSO)));
        for (int i = 0; i < n; ++i) {
            t.posicao += deslocamento / static_cast<float>(n);
            if (glm::distance(t.posicao, jogador) < raioJogador + RAIO_PROJETIL) {
                r.golpes.push_back({t.dano, t.autor});
                return true;
            }
            if (g.solida(static_cast<int>(std::floor(t.posicao.x)), static_cast<int>(std::floor(t.posicao.y))))
                return true;
        }
        return false;
    });
    return r;
}

}
