#include <doctest/doctest.h>

#include <stdexcept>

#include "mundo/Grade.h"
#include "mundo/Zona.h"

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

TEST_CASE("carregarDeTexto lê comporta, tranca, elevador e chave") {
    auto m = mundo::carregarDeTexto({"#######", "#@CTKE#", "#######"});
    CHECK(m.grade.em(2, 1) == mundo::Celula::Comporta);
    CHECK(m.grade.em(3, 1) == mundo::Celula::ComportaTrancada);
    CHECK(m.grade.em(4, 1) == mundo::Celula::Chao);
    CHECK(m.chave == glm::vec2(4.5f, 1.5f));
    CHECK(m.elevador == glm::ivec2(5, 1));
    CHECK(m.grade.solida(2, 1));
    CHECK_FALSE(m.grade.solida(5, 1));
}

TEST_CASE("comporta aberta não é sólida") {
    mundo::Grade g(1, 1);
    g.definir(0, 0, mundo::Celula::ComportaAberta);
    CHECK_FALSE(g.solida(0, 0));
}

TEST_CASE("zona do andar") {
    CHECK(mundo::zonaDoAndar(1) == mundo::Zona::Esgoto);
    CHECK(mundo::zonaDoAndar(2) == mundo::Zona::Esgoto);
    CHECK(mundo::zonaDoAndar(3) == mundo::Zona::Fabrica);
    CHECK(mundo::zonaDoAndar(4) == mundo::Zona::Fabrica);
    CHECK(mundo::zonaDoAndar(5) == mundo::Zona::Minas);
    CHECK(mundo::zonaDoAndar(50) == mundo::Zona::Minas);
}
