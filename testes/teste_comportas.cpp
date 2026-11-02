#include <doctest/doctest.h>

#include <algorithm>

#include "mundo/Grade.h"
#include "render/Comportas.h"
#include "render/Zonas.h"

TEST_CASE("comporta fechada gera duas faces no meio da célula") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    auto v = render::construirComportas(m.grade, [](int, int) { return 0.0f; });
    REQUIRE(v.size() == 12);
    for (const auto& p : v) CHECK(p.posicao.x == doctest::Approx(2.5f));  // passagem em X → plano X = 2,5
}

TEST_CASE("comporta em passagem vertical fica no plano Z") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "#C#", "#.#", "###"});
    for (const auto& p : render::construirComportas(m.grade, [](int, int) { return 0.0f; }))
        CHECK(p.posicao.z == doctest::Approx(2.5f));
}

TEST_CASE("comporta meio aberta sobe metade") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    float menorY = 10;
    for (const auto& p : render::construirComportas(m.grade, [](int, int) { return 0.5f; }))
        menorY = std::min(menorY, p.posicao.y);
    CHECK(menorY == doctest::Approx(0.5f));
}

TEST_CASE("comporta aberta não aparece") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    m.grade.definir(2, 1, mundo::Celula::ComportaAberta);
    CHECK(render::construirComportas(m.grade, [](int, int) { return 1.0f; }).empty());
}

TEST_CASE("comporta trancada usa a própria textura") {
    auto m = mundo::carregarDeTexto({"#####", "#@T.#", "#####"});
    for (const auto& p : render::construirComportas(m.grade, [](int, int) { return 0.0f; }))
        CHECK(p.camada == float(render::CamadaComportaTrancada));
}

TEST_CASE("cor ambiente por zona") {
    CHECK(render::corAmbiente(mundo::Zona::Esgoto) == glm::vec3(0.10f, 0.09f, 0.06f));
    CHECK(render::corAmbiente(mundo::Zona::Fabrica) == glm::vec3(0.12f, 0.07f, 0.03f));
    CHECK(render::corAmbiente(mundo::Zona::Minas) == glm::vec3(0.05f, 0.07f, 0.11f));
}
