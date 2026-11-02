#include <doctest/doctest.h>

#include "jogo/PassoFixo.h"

TEST_CASE("passo fixo conta passos de 1/60") {
    jogo::PassoFixo p;
    CHECK(p.adicionar(0.051) == 3);
    CHECK(p.alfa() == doctest::Approx(0.06).epsilon(0.01));
}

TEST_CASE("passo fixo acumula sobra entre quadros") {
    jogo::PassoFixo p;
    CHECK(p.adicionar(0.010) == 0);
    CHECK(p.adicionar(0.010) == 1);
}

TEST_CASE("passo fixo limita travadas") {
    jogo::PassoFixo p;
    CHECK(p.adicionar(10.0) == 15);  // 0,25 s no máximo
}

TEST_CASE("zerar descarta a sobra") {
    jogo::PassoFixo p;
    p.adicionar(0.010);
    p.zerar();
    CHECK(p.adicionar(0.010) == 0);
}
