#pragma once
#include <vector>

#include <glm/vec2.hpp>

#include "render/GeometriaMasmorra.h"

namespace render {

// Camadas do array de sprites
enum CamadaSprite : int { SpriteValvulaChave = 0, SpriteElevador = 1, TOTAL_SPRITES };

struct Sprite {
    glm::vec2 posicao;  // (X, Z) do mundo
    int camada;
    float largura;
    float altura;
};

// Um quadrado por sprite, de pé no chão e virado para a câmera (gira só no eixo vertical)
std::vector<Vertice> construirSprites(const std::vector<Sprite>& sprites, glm::vec2 camera);

}
