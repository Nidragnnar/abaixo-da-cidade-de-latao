#include <doctest/doctest.h>

#include <glm/geometric.hpp>

#include "mundo/Grade.h"
#include "render/GeometriaMasmorra.h"

using mundo::Zona;

TEST_CASE("célula isolada gera 4 paredes, chão e teto") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    CHECK(render::construirGeometria(m.grade, Zona::Esgoto).size() == 6 * 6);
}

TEST_CASE("duas células vizinhas não geram parede entre elas") {
    auto m = mundo::carregarDeTexto({"####", "#@.#", "####"});
    CHECK(render::construirGeometria(m.grade, Zona::Esgoto).size() == 10 * 6);  // 6 paredes + 2 chãos + 2 tetos
}

TEST_CASE("normal da parede aponta para a célula aberta") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    for (const auto& v : render::construirGeometria(m.grade, Zona::Esgoto))
        if (v.camada == static_cast<float>(render::ParedeEsgoto)) {
            glm::vec3 centro{1.5f, 0.5f, 1.5f};
            CHECK(glm::dot(v.normal, centro - v.posicao) > 0.0f);
        }
}

TEST_CASE("comporta no corredor não gera parede entre ela e o corredor") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    // 3 células com chão e teto = 6 quadrados; paredes: célula 1 (cima, baixo, esquerda),
    // comporta (cima, baixo), célula 3 (cima, baixo, direita) = 8
    CHECK(render::construirGeometria(m.grade, Zona::Esgoto).size() == 14 * 6);
}

TEST_CASE("zona da fábrica usa as camadas da fábrica") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    for (const auto& v : render::construirGeometria(m.grade, Zona::Fabrica))
        CHECK((v.camada == float(render::ParedeFabrica) || v.camada == float(render::ChaoFabrica) ||
               v.camada == float(render::TetoFabrica)));
}
