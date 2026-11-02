#include <doctest/doctest.h>

#include <cmath>

#include "render/Letterbox.h"

TEST_CASE("letterbox 16:9 numa tela 16:9 ocupa tudo") {
    auto r = render::calcularLetterbox(1920, 1080, 480, 270);
    CHECK(r.x == 0);
    CHECK(r.y == 0);
    CHECK(r.largura == 1920);
    CHECK(r.altura == 1080);
}

TEST_CASE("letterbox numa tela 16:10 põe faixas em cima e embaixo") {
    auto r = render::calcularLetterbox(1920, 1200, 480, 270);
    CHECK(r.x == 0);
    CHECK(r.y == 60);
    CHECK(r.largura == 1920);
    CHECK(r.altura == 1080);
}

TEST_CASE("letterbox numa tela 4:3 põe faixas nas laterais") {
    auto r = render::calcularLetterbox(1024, 768, 480, 270);
    CHECK(r.largura == 1024);
    CHECK(r.altura == 576);
    CHECK(r.y == 96);
}

TEST_CASE("letterbox com janela zerada") {
    auto r = render::calcularLetterbox(0, 0, 480, 270);
    CHECK(r.largura == 0);
    CHECK(r.altura == 0);
}

TEST_CASE("janela para tela baixa sem faixas") {
    auto p = render::janelaParaTelaBaixa({640, 360}, {1280, 720}, {1280, 720}, 480, 270);
    CHECK(p.x == doctest::Approx(240));
    CHECK(p.y == doctest::Approx(135));
}

TEST_CASE("janela para tela baixa com faixas e escala de tela 150%") {
    // janela lógica 1280×800 (16:10) em 150% → 1920×1200 pixels; imagem 1920×1080 com faixas de 60 px
    auto p = render::janelaParaTelaBaixa({640, 40}, {1280, 800}, {1920, 1200}, 480, 270);
    CHECK(p.y == doctest::Approx(0.0f).epsilon(0.01));  // 40 lógicos = 60 px = topo da imagem
    CHECK(p.x == doctest::Approx(240));
}

TEST_CASE("janela zerada não divide por zero") {
    auto p = render::janelaParaTelaBaixa({10, 10}, {0, 0}, {0, 0}, 480, 270);
    CHECK(std::isfinite(p.x));
    CHECK(std::isfinite(p.y));
}
