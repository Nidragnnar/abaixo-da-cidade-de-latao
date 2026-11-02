#include <doctest/doctest.h>

#include <stdexcept>

#include "mundo/Grade.h"
#include "mundo/MapaFixo.h"

TEST_CASE("carregarDeTexto lê paredes, chão e início") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "#.#", "###"});
    CHECK(m.grade.largura() == 3);
    CHECK(m.grade.altura() == 4);
    CHECK(m.grade.em(1, 2) == mundo::Celula::Chao);
    CHECK(m.grade.solida(0, 0));
    CHECK(m.inicio == glm::vec2(1.5f, 1.5f));
}

TEST_CASE("fora da grade é parede") {
    mundo::Grade g(2, 2);
    CHECK(g.em(-1, 0) == mundo::Celula::Parede);
    CHECK(g.solida(5, 5));
}

TEST_CASE("texto inválido é recusado") {
    CHECK_THROWS_AS(mundo::carregarDeTexto({"###", "#@"}), std::invalid_argument);
    CHECK_THROWS_AS(mundo::carregarDeTexto({"#x#"}), std::invalid_argument);
    CHECK_THROWS_AS(mundo::carregarDeTexto({"#.#"}), std::invalid_argument);
}

TEST_CASE("mapa fixo da etapa 1 carrega") {
    CHECK_NOTHROW(mundo::carregarDeTexto(mundo::mapaFixoEtapa1()));
}
