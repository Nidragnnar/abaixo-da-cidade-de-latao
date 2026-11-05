#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>

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

TEST_CASE("degrau gera face vertical voltada para a célula de baixo") {
    auto m = mundo::carregarDeTexto({"####", "#@.#", "####"}, {"0000", "0020", "0000"});
    bool achou = false;
    const auto v = render::construirGeometria(m.grade, Zona::Esgoto);
    for (size_t i = 0; i + 6 <= v.size(); i += 6) {
        float xmin = 9, xmax = -9, ymin = 9, ymax = -9;
        for (size_t k = 0; k < 6; ++k) {
            xmin = std::min(xmin, v[i + k].posicao.x), xmax = std::max(xmax, v[i + k].posicao.x);
            ymin = std::min(ymin, v[i + k].posicao.y), ymax = std::max(ymax, v[i + k].posicao.y);
        }
        if (xmin == doctest::Approx(2.0f) && xmax == doctest::Approx(2.0f) && ymin == doctest::Approx(0.0f) &&
            ymax == doctest::Approx(0.5f)) {
            achou = true;
            CHECK(v[i].normal.x < 0.0f);
        }
    }
    CHECK(achou);
}

TEST_CASE("rampa: chão inclinado nos cantos") {
    auto m = mundo::carregarDeTexto({"#####", "#@..#", "#####"}, {"00000", "00l40", "00000"});
    int vistos = 0;
    for (const auto& v : render::construirGeometria(m.grade, Zona::Esgoto)) {
        if (v.camada != float(render::ChaoEsgoto) || v.posicao.x < 1.99f || v.posicao.x > 3.01f) continue;
        if (v.posicao.z < 0.99f || v.posicao.z > 2.01f || v.normal.y <= 0.0f) continue;
        // Chão em x = 2 (fim da célula 1 e começo da rampa) a 0; em x = 3 (topo da rampa e célula 3) a 1
        if (std::abs(v.posicao.x - 2.0f) < 0.01f) CHECK(v.posicao.y == doctest::Approx(0.0f));
        if (std::abs(v.posicao.x - 3.0f) < 0.01f) CHECK(v.posicao.y == doctest::Approx(1.0f));
        ++vistos;
    }
    CHECK(vistos > 0);
}

TEST_CASE("textura não estica: v vai de 0 a 3 numa parede de 3 blocos") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"}, {}, {"000", "0c0", "000"});
    float menor = 99, maior = -99;
    for (const auto& v : render::construirGeometria(m.grade, Zona::Esgoto))
        if (v.camada == float(render::ParedeEsgoto)) menor = std::min(menor, v.uv.y), maior = std::max(maior, v.uv.y);
    CHECK(maior - menor == doctest::Approx(3.0f));
}

TEST_CASE("caixa usa a textura de caixa e escada a de degrau") {
    auto m = mundo::carregarDeTexto({"#####", "#@..#", "#####"}, {"00000", "00040", "00000"});
    m.grade.definirDetalhe(3, 1, mundo::Detalhe::Caixa);
    bool caixa = false;
    for (const auto& v : render::construirGeometria(m.grade, Zona::Esgoto)) caixa |= v.camada == float(render::CamadaCaixa);
    CHECK(caixa);
    auto e = mundo::carregarDeTexto({"#####", "#@..#", "#####"}, {"00000", "00120", "00000"});
    e.grade.definirDetalhe(2, 1, mundo::Detalhe::Escada);
    bool degrau = false;
    for (const auto& v : render::construirGeometria(e.grade, Zona::Esgoto)) degrau |= v.camada == float(render::CamadaDegrau);
    CHECK(degrau);
}
