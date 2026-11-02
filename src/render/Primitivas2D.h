#pragma once
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace render {

// Pixels da tela baixa, origem no canto de cima à esquerda
struct VerticeCor {
    glm::vec2 posicao;
    glm::vec4 cor;
};

struct VerticeTex {
    glm::vec2 posicao;
    glm::vec2 uv;
    float camada;  // camada do array de texturas
    glm::vec4 cor;  // multiplica a textura
};

// Retângulo de a (cima, esquerda) a b (baixo, direita): 6 vértices, começando por a e (b.x, a.y)
void adicionarRetangulo(std::vector<VerticeCor>& v, glm::vec2 a, glm::vec2 b, glm::vec4 cor);
void adicionarQuadro(std::vector<VerticeTex>& v, glm::vec2 a, glm::vec2 b, float camada, glm::vec4 cor);

}
