#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

#include <glm/geometric.hpp>

#include "jogo/Jogador.h"
#include "mundo/Grade.h"
#include "mundo/Terreno.h"

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

TEST_CASE("inclinar olha para cima e para baixo com limite de 60 graus") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    jogo::Jogador j{m.inicio, 0.0f};
    jogo::Comandos c;
    c.inclinar = 0.3f;
    jogo::atualizarJogador(j, c, m.grade, 1.0f / 60);
    CHECK(j.inclinacao == doctest::Approx(0.3f));
    c.inclinar = 5.0f;
    jogo::atualizarJogador(j, c, m.grade, 1.0f / 60);
    CHECK(j.inclinacao == doctest::Approx(jogo::INCLINACAO_MAXIMA));
    c.inclinar = -10.0f;
    jogo::atualizarJogador(j, c, m.grade, 1.0f / 60);
    CHECK(j.inclinacao == doctest::Approx(-jogo::INCLINACAO_MAXIMA));
}

namespace {
jogo::Comandos andar(float frente, bool pular = false) {
    jogo::Comandos c;
    c.frente = frente;
    c.pular = pular;
    return c;
}
// Corredor "#@...#" com alturas e tetos (relativos) de uma linha
mundo::MapaCarregado linha(const std::string& mapa, const std::string& alturas, const std::string& tetos = {}) {
    const std::string borda(mapa.size(), '#'), zeros(mapa.size(), '0');
    return mundo::carregarDeTexto({borda, mapa, borda}, {zeros, alturas, zeros},
                                  tetos.empty() ? std::vector<std::string>{} : std::vector<std::string>{zeros, tetos, zeros});
}
}

TEST_CASE("dano de queda") {
    CHECK(mundo::danoDeQueda(2.0f) == 0);
    CHECK(mundo::danoDeQueda(2.49f) == 0);
    CHECK(mundo::danoDeQueda(2.5f) == 5);
    CHECK(mundo::danoDeQueda(3.0f) == 10);
    CHECK(mundo::danoDeQueda(4.0f) == 20);
}

TEST_CASE("anda e sobe degrau de 1/4, para no de 1/2") {
    auto m = linha("#@...#", "000130");
    jogo::Jogador j{m.inicio, 0.0f};
    for (int i = 0; i < 120; ++i) jogo::atualizarJogador(j, andar(1), m.grade, 1.0f / 60);
    CHECK(j.corpo.pes == doctest::Approx(0.25f));
    CHECK(j.posicao.x < 4.0f - 0.24f);
    CHECK(j.posicao.x > 3.5f);
}

TEST_CASE("rampa acompanha o chão") {
    auto m = linha("#@...#", "00l4aa", "008880");
    jogo::Jogador j{m.inicio, 0.0f};
    for (int i = 0; i < 90; ++i) {
        jogo::atualizarJogador(j, andar(1), m.grade, 1.0f / 60);
        if (j.posicao.x > 2.3f && j.posicao.x < 2.7f)
            CHECK(j.corpo.pes == doctest::Approx(mundo::alturaDoChao(m.grade, j.posicao)).epsilon(0.02));
    }
    CHECK(j.corpo.pes == doctest::Approx(1.0f));  // em cima, na célula de chão 4
}

TEST_CASE("pulo alcança 1 bloco mas não 1,25") {
    auto alcanca = [](const std::string& alturas) {
        auto m = linha("#@...#", alturas, "099999");
        jogo::Jogador j{m.inicio, 0.0f};
        for (int i = 0; i < 30; ++i) jogo::atualizarJogador(j, andar(1), m.grade, 1.0f / 60);  // encosta na beira
        jogo::atualizarJogador(j, andar(1, true), m.grade, 1.0f / 60);
        for (int i = 0; i < 90; ++i) jogo::atualizarJogador(j, andar(1), m.grade, 1.0f / 60);
        return j.corpo.pes;
    };
    CHECK(alcanca("000440") == doctest::Approx(1.0f));
    CHECK(alcanca("000550") == doctest::Approx(0.0f));
}

TEST_CASE("teto corta o pulo") {
    auto m = linha("#@...#", "000000", "066660");
    jogo::Jogador j{m.inicio, 0.0f};
    jogo::atualizarJogador(j, andar(0, true), m.grade, 1.0f / 60);
    float maisAlto = 0;
    for (int i = 0; i < 60; ++i) {
        jogo::atualizarJogador(j, andar(0), m.grade, 1.0f / 60);
        maisAlto = std::max(maisAlto, j.corpo.pes);
    }
    CHECK(maisAlto + mundo::ALTURA_CORPO <= 1.5f + 1e-3f);
    CHECK(j.corpo.noChao);
}

TEST_CASE("cair devolve a altura da queda") {
    auto m = linha("#@...#", "0cc000", "0kkkk0");
    jogo::Jogador j{m.inicio, 0.0f};
    j.corpo.pes = 3.0f;
    float queda = 0;
    for (int i = 0; i < 120; ++i) queda = std::max(queda, jogo::atualizarJogador(j, andar(1), m.grade, 1.0f / 60));
    CHECK(queda == doctest::Approx(3.0f).epsilon(0.02));
    CHECK(j.corpo.pes == doctest::Approx(0.0f));
}

TEST_CASE("pular subindo a rampa nunca joga o corpo para trás") {
    auto m = linha("#@......#", "00l444440", "099999990");
    jogo::Jogador j{m.inicio, 0.0f};
    float antes = j.posicao.x;
    bool pulou = false;
    for (int i = 0; i < 120; ++i) {
        const bool pular = !pulou && j.posicao.x >= 2.2f;  // pula no meio da rampa
        pulou |= pular;
        jogo::atualizarJogador(j, andar(1, pular), m.grade, 1.0f / 60);
        REQUIRE(j.posicao.x >= antes - 1e-4f);
        antes = j.posicao.x;
    }
    CHECK(pulou);
    CHECK(j.corpo.pes == doctest::Approx(1.0f));  // terminou em cima
}

TEST_CASE("rampa que termina num degrau de 1/4 se sobe andando") {
    // Achado da demo: os pés (no centro) ficavam meio bloco abaixo do degrau quando a frente já estava no topo
    auto m = linha("#@...#", "00l550", "008880");
    jogo::Jogador j{m.inicio, 0.0f};
    for (int i = 0; i < 120; ++i) jogo::atualizarJogador(j, andar(1), m.grade, 1.0f / 60);
    CHECK(j.corpo.pes == doctest::Approx(1.25f));
    CHECK(j.posicao.x > 3.5f);
}
