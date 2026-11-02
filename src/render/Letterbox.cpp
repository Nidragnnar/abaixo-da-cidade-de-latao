#include "render/Letterbox.h"

namespace render {

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

}
