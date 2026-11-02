#include <doctest/doctest.h>

#include <cmath>

#include <glm/trigonometric.hpp>

#include "render/Camera.h"

TEST_CASE("projeção tem FOV horizontal de 75 graus") {
    auto proj = render::matrizProjecao(16.0f / 9.0f);
    float t = std::tan(glm::radians(37.5f));
    glm::vec4 borda = proj * glm::vec4(t, 0.0f, -1.0f, 1.0f);  // espaço da câmera
    CHECK(borda.x / borda.w == doctest::Approx(1.0f).epsilon(0.001));
}

TEST_CASE("câmera com ângulo 0 olha para +X") {
    auto v = render::matrizVisao({2.0f, 3.0f}, 0.0f);
    glm::vec4 frente = v * glm::vec4(3.0f, 0.5f, 3.0f, 1.0f);
    CHECK(frente.z < 0.0f);  // à frente da câmera
    CHECK(frente.x == doctest::Approx(0.0f).epsilon(0.001));
}
