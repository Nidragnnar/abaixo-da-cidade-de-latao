#include <doctest/doctest.h>

#include <glm/geometric.hpp>

#include "mundo/Grade.h"
#include "render/GeometriaMasmorra.h"

TEST_CASE("célula isolada gera 4 paredes, chão e teto") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    CHECK(render::construirGeometria(m.grade).size() == 6 * 6);
}

TEST_CASE("duas células vizinhas não geram parede entre elas") {
    auto m = mundo::carregarDeTexto({"####", "#@.#", "####"});
    CHECK(render::construirGeometria(m.grade).size() == 10 * 6);  // 6 paredes + 2 chãos + 2 tetos
}

TEST_CASE("normal da parede aponta para a célula aberta") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    for (const auto& v : render::construirGeometria(m.grade))
        if (v.camada == static_cast<float>(render::ParedeEsgoto)) {
            glm::vec3 centro{1.5f, 0.5f, 1.5f};
            CHECK(glm::dot(v.normal, centro - v.posicao) > 0.0f);
        }
}
