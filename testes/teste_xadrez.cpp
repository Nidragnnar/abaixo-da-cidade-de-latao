#include <doctest/doctest.h>

#include "render/Xadrez.h"

TEST_CASE("xadrez de reserva") {
    auto px = render::gerarXadrez(64);
    REQUIRE(px.size() == 64 * 64 * 4);
    CHECK(px[0] == 255);
    CHECK(px[1] == 0);
    CHECK(px[2] == 255);
    CHECK(px[3] == 255);
    CHECK(px[8 * 4] == 0);  // pixel (8, 0) é preto
}
