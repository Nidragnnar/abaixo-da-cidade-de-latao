#pragma once

namespace render {

struct Retangulo {
    int x, y, largura, altura;
};

// Maior retângulo com a proporção da base, centralizado na janela; janela zerada → retângulo zerado
Retangulo calcularLetterbox(int janelaL, int janelaA, int baseL, int baseA);

}
