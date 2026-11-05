#include "jogo/Projeteis.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

#include "mundo/Terreno.h"

namespace jogo {

namespace {
constexpr float SUBPASSO = 0.1f;
}

ResultadoProjeteis atualizarProjeteis(std::vector<Projetil>& p, const mundo::Grade& g, glm::vec3 centroJogador,
                                      float raioJogador, float dt) {
    ResultadoProjeteis r;
    const glm::vec2 jogador{centroJogador.x, centroJogador.z};
    std::erase_if(p, [&](Projetil& t) {
        const glm::vec3 deslocamento = t.velocidade * dt;
        const int n = std::max(1, static_cast<int>(std::ceil(glm::length(deslocamento) / SUBPASSO)));
        for (int i = 0; i < n; ++i) {
            t.posicao += deslocamento / static_cast<float>(n);
            const glm::vec2 plano{t.posicao.x, t.posicao.z};
            if (glm::distance(plano, jogador) < raioJogador + RAIO_PROJETIL &&
                std::abs(t.posicao.y - centroJogador.y) <= mundo::ALTURA_CORPO / 2 + RAIO_PROJETIL) {
                r.golpes.push_back({t.dano, t.autor});
                return true;
            }
            if (g.solida(static_cast<int>(std::floor(plano.x)), static_cast<int>(std::floor(plano.y))) ||
                t.posicao.y < mundo::alturaDoChao(g, plano) || t.posicao.y > mundo::alturaDoTeto(g, plano))
                return true;
        }
        return false;
    });
    return r;
}

}
