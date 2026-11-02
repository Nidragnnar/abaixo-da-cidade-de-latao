#include <doctest/doctest.h>

#include <cmath>

#include "render/Sprites.h"

TEST_CASE("sprite fica de frente para a câmera") {
    auto v = render::construirSprites({{{2.0f, 0.0f}, render::SpriteElevador, 0.9f, 1.0f}}, {0.0f, 0.0f});
    REQUIRE(v.size() == 6);
    for (const auto& p : v) {
        CHECK(p.posicao.x == doctest::Approx(2.0f));
        CHECK(p.normal.x == doctest::Approx(-1.0f));
        CHECK(std::abs(p.posicao.z) <= 0.45f + 1e-4f);
        CHECK(p.posicao.y >= 0.0f);
        CHECK(p.posicao.y <= 1.0f + 1e-4f);
    }
}

TEST_CASE("sprite usa a camada pedida") {
    auto v = render::construirSprites({{{1.0f, 1.0f}, render::SpriteValvulaChave, 0.4f, 0.4f}}, {3.0f, 1.0f});
    for (const auto& p : v) CHECK(p.camada == float(render::SpriteValvulaChave));
}
