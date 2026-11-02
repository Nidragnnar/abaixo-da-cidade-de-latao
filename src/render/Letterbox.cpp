#include "render/Letterbox.h"

namespace render {

bool contem(const Retangulo& r, glm::vec2 p) {
    return p.x >= r.x && p.y >= r.y && p.x < r.x + r.largura && p.y < r.y + r.altura;
}

Retangulo calcularLetterbox(int janelaL, int janelaA, int baseL, int baseA) {
    if (janelaL <= 0 || janelaA <= 0 || baseL <= 0 || baseA <= 0) return {0, 0, 0, 0};
    // Compara as proporções em inteiros para não perder pixel por arredondamento
    int largura = janelaL, altura = janelaA;
    if (static_cast<long long>(janelaL) * baseA > static_cast<long long>(janelaA) * baseL)
        largura = static_cast<int>(static_cast<long long>(janelaA) * baseL / baseA);  // faixas nas laterais
    else
        altura = static_cast<int>(static_cast<long long>(janelaL) * baseA / baseL);  // faixas em cima e embaixo
    return {(janelaL - largura) / 2, (janelaA - altura) / 2, largura, altura};
}

glm::vec2 janelaParaTelaBaixa(glm::vec2 mouse, glm::ivec2 tamanhoJanela, glm::ivec2 tamanhoDesenhavel, int baseL, int baseA) {
    if (tamanhoJanela.x <= 0 || tamanhoJanela.y <= 0) return {-1.0f, -1.0f};
    const glm::vec2 escala = glm::vec2(tamanhoDesenhavel) / glm::vec2(tamanhoJanela);
    const Retangulo r = calcularLetterbox(tamanhoDesenhavel.x, tamanhoDesenhavel.y, baseL, baseA);
    if (r.largura <= 0 || r.altura <= 0) return {-1.0f, -1.0f};
    const glm::vec2 pixel = mouse * escala - glm::vec2(r.x, r.y);
    return pixel * glm::vec2(static_cast<float>(baseL) / r.largura, static_cast<float>(baseA) / r.altura);
}

}
