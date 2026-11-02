#include <doctest/doctest.h>

#include "mundo/Consultas.h"
#include "mundo/Grade.h"
#include "render/Automapa.h"

TEST_CASE("nada explorado: só o painel e o jogador") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    mundo::Exploracao e(3, 3);
    CHECK(render::construirAutomapa(m.grade, e, m.inicio, 0.0f, {}, 480, 270).size() == 6 + 3);
}

TEST_CASE("célula de chão vista vira um quadrado") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    mundo::Exploracao e(3, 3);
    e.marcarTudo();  // paredes vistas não desenham nada
    CHECK(render::construirAutomapa(m.grade, e, m.inicio, 0.0f, {}, 480, 270).size() == 6 + 6 + 3);
}

TEST_CASE("jogador aparece na posição certa do mapa") {
    auto m = mundo::carregarDeTexto({"####", "#.@#", "####"});
    mundo::Exploracao e(4, 3);
    auto v = render::construirAutomapa(m.grade, e, m.inicio, 0.0f, {}, 480, 270);
    glm::vec2 centro = (v[v.size() - 1].posicao + v[v.size() - 2].posicao + v[v.size() - 3].posicao) / 3.0f;
    CHECK(centro.x == doctest::Approx(112 + 2.5f * 4).epsilon(0.05));
    CHECK(centro.y == doctest::Approx(7 + 1.5f * 4).epsilon(0.05));
}

TEST_CASE("chave só aparece se a célula dela foi vista") {
    auto m = mundo::carregarDeTexto({"#####", "#@.K#", "#####"});
    mundo::Exploracao e(5, 3);
    auto semVer = render::construirAutomapa(m.grade, e, m.inicio, 0.0f, m.chave, 480, 270).size();
    e.marcar(3, 1);
    auto vendo = render::construirAutomapa(m.grade, e, m.inicio, 0.0f, m.chave, 480, 270).size();
    CHECK(vendo == semVer + 6 + 6);  // chão da célula + marca da chave
}
