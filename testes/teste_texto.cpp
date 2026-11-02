#include <doctest/doctest.h>

#include <string_view>

#include "render/Texto.h"

TEST_CASE("um quadrado por caractere, inclusive acentuado") {
    CHECK(render::construirTexto("Olá", {0, 0}, 1, glm::vec4(1)).size() == 3 * 6);
    CHECK(render::larguraTexto("Olá", 2.0f) == doctest::Approx(48.0f));
}

TEST_CASE("glifos acentuados e desconhecidos") {
    CHECK(render::indiceGlifo(U'A') == 'A' - 32);
    CHECK(render::indiceGlifo(U'ç') > 94);
    CHECK(render::indiceGlifo(U'€') == render::indiceGlifo(U'?'));
}

TEST_CASE("UTF-8 inválido vira interrogação") {
    auto v = render::construirTexto(std::string_view("\xC3", 1), {0, 0}, 1, glm::vec4(1));
    REQUIRE(v.size() == 6);
    CHECK(v[0].camada == float(render::indiceGlifo(U'?')));
}

TEST_CASE("caracteres avançam 8 pixels vezes a escala") {
    auto v = render::construirTexto("ab", {10, 20}, 2, glm::vec4(1));
    REQUIRE(v.size() == 12);
    CHECK(v[0].posicao == glm::vec2(10, 20));
    CHECK(v[6].posicao == glm::vec2(26, 20));
}
