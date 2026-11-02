#include "render/Sprites.h"

#include <glm/geometric.hpp>

namespace render {

std::vector<Vertice> construirSprites(const std::vector<Sprite>& sprites, glm::vec2 camera) {
    std::vector<Vertice> v;
    v.reserve(sprites.size() * 6);
    for (const Sprite& s : sprites) {
        glm::vec2 paraCamera = camera - s.posicao;
        paraCamera = glm::length(paraCamera) > 1e-4f ? glm::normalize(paraCamera) : glm::vec2(1.0f, 0.0f);
        const glm::vec3 normal{paraCamera.x, 0.0f, paraCamera.y};
        // Direita de quem olha da câmera: com ela, o quadrado fica anti-horário visto de frente
        const glm::vec3 direita{normal.z, 0.0f, -normal.x};
        const glm::vec3 centro{s.posicao.x, 0.0f, s.posicao.y};
        const glm::vec3 p0 = centro - direita * (s.largura * 0.5f), p1 = centro + direita * (s.largura * 0.5f);
        const glm::vec3 alto{0.0f, s.altura, 0.0f};
        adicionarQuadrado(v, p0, p1, p1 + alto, p0 + alto, static_cast<float>(s.camada), normal);
    }
    return v;
}

}
