#include <doctest/doctest.h>

#include "mundo/Consultas.h"
#include "mundo/Grade.h"

TEST_CASE("linha de visão numa sala aberta") {
    auto m = mundo::carregarDeTexto({"######", "#@...#", "#....#", "######"});
    CHECK(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {4.5f, 2.5f}));
}

TEST_CASE("parede bloqueia a linha de visão") {
    auto m = mundo::carregarDeTexto({"#####", "#@#.#", "#####"});
    CHECK_FALSE(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {3.5f, 1.5f}));
}

TEST_CASE("enxerga a própria parede de destino") {
    auto m = mundo::carregarDeTexto({"####", "#@.#", "####"});
    CHECK(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {3.5f, 1.5f}));
}

TEST_CASE("não vê através de quina diagonal") {
    auto m = mundo::carregarDeTexto({"####", "#@##", "##.#", "####"});
    CHECK_FALSE(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {2.5f, 2.5f}));
}

TEST_CASE("comporta fechada bloqueia a visão") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    CHECK_FALSE(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {3.5f, 1.5f}));
}

TEST_CASE("exploração marca perto e com visão, não longe nem atrás de parede") {
    auto m = mundo::carregarDeTexto({"####################", "#@...#.............#", "####################"});
    mundo::Exploracao e(m.grade.largura(), m.grade.altura());
    mundo::explorarAoRedor(e, m.grade, m.inicio, mundo::RAIO_EXPLORACAO);
    CHECK(e.vista(4, 1));
    CHECK(e.vista(5, 1));        // a parede que fecha o corredor
    CHECK_FALSE(e.vista(6, 1));  // atrás dela
    CHECK_FALSE(e.vista(18, 1)); // longe
}

TEST_CASE("célula à frente encontra a comporta") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    CHECK(mundo::celulaAFrente(m.grade, m.inicio, 0.0f, 1.5f) == glm::ivec2(2, 1));
}

TEST_CASE("célula à frente ignora o que está atrás e devolve nada se só há chão") {
    auto m = mundo::carregarDeTexto({"#######", "#C@...#", "#######"});
    CHECK_FALSE(mundo::celulaAFrente(m.grade, m.inicio, 0.0f, 1.5f).has_value());
}

TEST_CASE("célula à frente inclui a célula do jogador (em cima do elevador)") {
    auto m = mundo::carregarDeTexto({"####", "#@E#", "####"});
    CHECK(mundo::celulaAFrente(m.grade, {2.5f, 1.5f}, 3.14159f, 1.5f) == glm::ivec2(2, 1));
}

TEST_CASE("distâncias respeitam a comporta trancada") {
    auto m = mundo::carregarDeTexto({"######", "#@.T.#", "######"});
    auto semTranca = mundo::distanciasNaGrade(m.grade, {1, 1}, false);
    auto comTranca = mundo::distanciasNaGrade(m.grade, {1, 1}, true);
    CHECK(semTranca[1 * 6 + 4] == -1);
    CHECK(comTranca[1 * 6 + 4] == 3);
}
