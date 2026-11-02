#include <doctest/doctest.h>

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
