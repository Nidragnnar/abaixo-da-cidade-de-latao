#pragma once
#include <glm/vec2.hpp>

namespace render {

struct Retangulo {
    int x, y, largura, altura;
};

// Ponto dentro do retângulo (borda de baixo e da direita não contam)
bool contem(const Retangulo& r, glm::vec2 ponto);

// Maior retângulo com a proporção da base, centralizado na janela; janela zerada → retângulo zerado
Retangulo calcularLetterbox(int janelaL, int janelaA, int baseL, int baseA);

// Posição do mouse (coordenadas lógicas da janela) em pixels da tela baixa, levando em conta a escala de tela
// (tamanho desenhável ÷ tamanho da janela) e as faixas pretas; fora da imagem pode sair negativo ou maior
glm::vec2 janelaParaTelaBaixa(glm::vec2 mouse, glm::ivec2 tamanhoJanela, glm::ivec2 tamanhoDesenhavel, int baseL, int baseA);

}
