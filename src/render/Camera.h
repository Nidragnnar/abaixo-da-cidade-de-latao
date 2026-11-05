#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "jogo/Jogador.h"

namespace render {

constexpr float ALTURA_OLHOS = jogo::ALTURA_OLHOS;
constexpr float FOV_HORIZONTAL_GRAUS = 75.0f;

// Olho em (x, ALTURA_OLHOS, z) olhando para (cos a · cos i, sin i, sin a · cos i); i = inclinação (+ para cima)
glm::mat4 matrizVisao(glm::vec2 posicao, float angulo, float inclinacao = 0.0f, float alturaOlhos = ALTURA_OLHOS);

// Perspectiva com FOV horizontal fixo (o vertical sai do aspecto)
glm::mat4 matrizProjecao(float aspecto);

}
