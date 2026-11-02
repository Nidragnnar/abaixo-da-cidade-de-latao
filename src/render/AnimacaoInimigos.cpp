#include "render/AnimacaoInimigos.h"

#include <cmath>

namespace render {

int camadaDoInimigo(jogo::TipoInimigo t, jogo::EstadoInimigo e, float tempoNoEstado) {
    const int base = PRIMEIRO_INIMIGO + static_cast<int>(t) * QUADROS_POR_INIMIGO;
    switch (e) {
        case jogo::EstadoInimigo::Perseguindo:
            return base + 1 + static_cast<int>(std::floor(tempoNoEstado / 0.25f)) % 2;
        case jogo::EstadoInimigo::Preparando: return base + 3;
        case jogo::EstadoInimigo::Recuperando: return base + 4;
        case jogo::EstadoInimigo::Destruido: return base + (tempoNoEstado < 0.3f ? 5 : 6);
        default: return base;
    }
}

glm::vec2 tamanhoDoInimigo(jogo::TipoInimigo t) {
    // A altura vem da tabela do jogo (a mesma que a mira vertical usa)
    const float largura = t == jogo::TipoInimigo::Rato ? 0.4f : t == jogo::TipoInimigo::Colosso ? 0.9f : 0.6f;
    return {largura, jogo::dados(t).altura};
}

}
