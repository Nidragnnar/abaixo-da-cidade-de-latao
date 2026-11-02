#include <doctest/doctest.h>

#include <glm/geometric.hpp>

#include "jogo/Jogador.h"
#include "mundo/Grade.h"

TEST_CASE("anda para a frente na direção do ângulo") {
    auto m = mundo::carregarDeTexto({"##########", "#@.......#", "##########"});
    jogo::Jogador j{m.inicio, 0.0f};
    for (int i = 0; i < 60; ++i) jogo::atualizarJogador(j, {1, 0, 0}, m.grade, 1.0f / 60);
    CHECK(j.posicao.x == doctest::Approx(4.5f).epsilon(0.01));  // 3 células em 1 s
}

TEST_CASE("lado positivo anda para a direita (+Z com ângulo 0)") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "#.#", "#.#", "###"});
    jogo::Jogador j{m.inicio, 0.0f};
    jogo::atualizarJogador(j, {0, 1, 0}, m.grade, 0.1f);
    CHECK(j.posicao.y > 1.5f);
}

TEST_CASE("diagonal não é mais rápida") {
    auto m = mundo::carregarDeTexto({"########", "#@.....#", "#......#", "#......#", "########"});
    jogo::Jogador j{m.inicio, 0.0f};
    jogo::atualizarJogador(j, {1, 1, 0}, m.grade, 0.1f);
    CHECK(glm::length(j.posicao - m.inicio) == doctest::Approx(0.3f).epsilon(0.01));
}

TEST_CASE("giro altera o ângulo") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    jogo::Jogador j{m.inicio, 0.0f};
    jogo::atualizarJogador(j, {0, 0, 0.5f}, m.grade, 1.0f / 60);
    CHECK(j.angulo == doctest::Approx(0.5f));
}
