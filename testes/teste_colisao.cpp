#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "mundo/Colisao.h"
#include "mundo/Grade.h"

// Corredor aberto nas colunas 1..4, parede na coluna 5
static const std::vector<std::string> CORREDOR = {"#######", "#@...##", "#######"};

TEST_CASE("para encostado na parede") {
    auto m = mundo::carregarDeTexto(CORREDOR);
    auto p = mundo::moverComColisao(m.grade, {4.0f, 1.5f}, {1.0f, 0.0f}, 0.25f);
    CHECK(p.x == doctest::Approx(4.75f).epsilon(0.002));
    CHECK(p.y == doctest::Approx(1.5f));
}

TEST_CASE("deslocamento grande não atravessa parede") {
    auto m = mundo::carregarDeTexto(CORREDOR);
    auto p = mundo::moverComColisao(m.grade, {1.5f, 1.5f}, {20.0f, 0.0f}, 0.25f);
    CHECK(p.x < 5.0f);
}

TEST_CASE("desliza ao longo da parede") {
    auto m = mundo::carregarDeTexto({"#####", "#@..#", "#...#", "#####"});
    auto p = mundo::moverComColisao(m.grade, {1.5f, 1.5f}, {0.5f, -1.0f}, 0.25f);
    CHECK(p.x == doctest::Approx(2.0f));                  // andou em X
    CHECK(p.y == doctest::Approx(1.25f).epsilon(0.002));  // parou em Y na parede de cima
}

TEST_CASE("passa por corredor de uma célula sem enroscar na quina") {
    auto m = mundo::carregarDeTexto({"#####", "#@..#", "##.##", "##.##", "#####"});
    glm::vec2 p{2.4f, 1.5f};
    for (int i = 0; i < 60; ++i) p = mundo::moverComColisao(m.grade, p, {0.0f, 0.05f}, 0.25f);
    CHECK(p.y > 3.0f);
}
