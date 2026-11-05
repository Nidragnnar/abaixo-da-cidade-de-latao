#include "render/Camera.h"

#include <cmath>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace render {

glm::mat4 matrizVisao(glm::vec2 posicao, float angulo, float inclinacao, float alturaOlhos) {
    const glm::vec3 olho{posicao.x, alturaOlhos, posicao.y};
    const float h = std::cos(inclinacao);  // parte horizontal do olhar
    const glm::vec3 frente{std::cos(angulo) * h, std::sin(inclinacao), std::sin(angulo) * h};
    return glm::lookAt(olho, olho + frente, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 matrizProjecao(float aspecto) {
    const float meioH = glm::radians(FOV_HORIZONTAL_GRAUS * 0.5f);
    const float fovVertical = 2.0f * std::atan(std::tan(meioH) / aspecto);
    return glm::perspective(fovVertical, aspecto, 0.05f, 64.0f);
}

}
