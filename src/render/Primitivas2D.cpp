#include "render/Primitivas2D.h"

namespace render {

void adicionarRetangulo(std::vector<VerticeCor>& v, glm::vec2 a, glm::vec2 b, glm::vec4 cor) {
    v.insert(v.end(), {{a, cor}, {{b.x, a.y}, cor}, {b, cor}, {a, cor}, {b, cor}, {{a.x, b.y}, cor}});
}

void adicionarQuadro(std::vector<VerticeTex>& v, glm::vec2 a, glm::vec2 b, float camada, glm::vec4 cor) {
    // V = 0 é a linha de cima da imagem (stb_image carrega de cima para baixo)
    const VerticeTex va{a, {0, 0}, camada, cor}, vb{{b.x, a.y}, {1, 0}, camada, cor};
    const VerticeTex vc{b, {1, 1}, camada, cor}, vd{{a.x, b.y}, {0, 1}, camada, cor};
    v.insert(v.end(), {va, vb, vc, va, vc, vd});
}

}
