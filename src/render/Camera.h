#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace render {

constexpr float ALTURA_OLHOS = 0.5f;
constexpr float FOV_HORIZONTAL_GRAUS = 75.0f;

// Olho em (x, ALTURA_OLHOS, z) olhando para (cos a, 0, sin a)
glm::mat4 matrizVisao(glm::vec2 posicao, float angulo);

// Perspectiva com FOV horizontal fixo (o vertical sai do aspecto)
glm::mat4 matrizProjecao(float aspecto);

}
