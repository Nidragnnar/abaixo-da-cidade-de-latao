#pragma once
#include <vector>

#include <glm/vec2.hpp>

#include "render/GeometriaMasmorra.h"

namespace render {

// Camadas do array de sprites
// Depois do projétil vêm os quadros dos inimigos (ver AnimacaoInimigos.h)
enum CamadaSprite : int { SpriteValvulaChave = 0, SpriteElevador = 1, SpriteProjetil = 2, PRIMEIRO_INIMIGO = 3 };

struct Sprite {
    glm::vec2 posicao;  // (X, Z) do mundo
    int camada;
    float largura;
    float altura;
    float base = 0.0f;  // altura do pé do sprite (projéteis voam a meia altura)
};

// Um quadrado por sprite, de pé no chão e virado para a câmera (gira só no eixo vertical)
std::vector<Vertice> construirSprites(const std::vector<Sprite>& sprites, glm::vec2 camera);

}
