#include <doctest/doctest.h>

#include <random>

#include "mundo/Aleatorio.h"
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

TEST_CASE("linha de visão entre pontos quaisquer, não só centros") {
    auto m = mundo::carregarDeTexto({"########", "#@.....#", "#..#...#", "#......#", "########"});
    CHECK_FALSE(mundo::temLinhaDeVisao(m.grade, {1.2f, 2.7f}, {5.9f, 2.2f}));  // passa pela parede (3,2)
    CHECK(mundo::temLinhaDeVisao(m.grade, {1.2f, 1.3f}, {6.8f, 1.6f}));
}

TEST_CASE("linha de visão é simétrica em 2000 pares aleatórios") {
    auto m = mundo::carregarDeTexto({"##########", "#@...#...#", "#.##...#.#", "#....#...#", "##########"});
    std::mt19937 rng(7);
    for (int i = 0; i < 2000; ++i) {
        glm::vec2 a{mundo::sortearReal(rng, 1.01f, 8.99f), mundo::sortearReal(rng, 1.01f, 3.99f)};
        glm::vec2 b{mundo::sortearReal(rng, 1.01f, 8.99f), mundo::sortearReal(rng, 1.01f, 3.99f)};
        if (m.grade.solida(int(a.x), int(a.y)) || m.grade.solida(int(b.x), int(b.y))) continue;
        REQUIRE(mundo::temLinhaDeVisao(m.grade, a, b) == mundo::temLinhaDeVisao(m.grade, b, a));
    }
}

TEST_CASE("sortear fica dentro dos limites") {
    std::mt19937 rng(1);
    for (int i = 0; i < 1000; ++i) {
        int v = mundo::sortear(rng, 2, 4);
        REQUIRE((v >= 2 && v <= 4));
        float r = mundo::sortearReal(rng, -1.0f, 1.0f);
        REQUIRE((r >= -1.0f && r <= 1.0f));
    }
}

TEST_CASE("distâncias podem contornar o elevador") {
    auto m = mundo::carregarDeTexto({"#######", "#.....#", "#@..E.#", "#.....#", "#######"});
    auto contorna = mundo::distanciasNaGrade(m.grade, {1, 2}, false, false);
    CHECK(contorna[2 * 7 + 4] == -1);  // o próprio elevador
    CHECK(contorna[2 * 7 + 5] == 6);   // atrás dele, dando a volta
}

TEST_CASE("passagem livre considera o elevador, a visão não") {
    auto m = mundo::carregarDeTexto({"#######", "#@..E.#", "#######"});
    CHECK(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {5.5f, 1.5f}));
    CHECK_FALSE(mundo::temPassagemLivre(m.grade, {1.5f, 1.5f}, {5.5f, 1.5f}));
}
