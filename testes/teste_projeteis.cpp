#include <doctest/doctest.h>

#include "jogo/Projeteis.h"
#include "mundo/Gerador.h"
#include "mundo/Grade.h"

#include <glm/geometric.hpp>

TEST_CASE("projétil anda 10 células por segundo") {
    auto m = mundo::carregarDeTexto({"####################", "#@.................#", "####################"});
    std::vector<jogo::Projetil> p{{{2.5f, 0.5f, 1.5f}, {10.0f, 0.0f, 0.0f}, 4, jogo::TipoInimigo::Atirador}};
    jogo::atualizarProjeteis(p, m.grade, {1.5f, 0.35f, 1.5f}, 0.25f, 0.5f);
    REQUIRE(p.size() == 1);
    CHECK(p[0].posicao.x == doctest::Approx(7.5f));
}

TEST_CASE("projétil some na parede") {
    auto m = mundo::carregarDeTexto({"######", "#@...#", "######"});
    std::vector<jogo::Projetil> p{{{2.5f, 0.5f, 1.5f}, {10.0f, 0.0f, 0.0f}, 4, jogo::TipoInimigo::Atirador}};
    jogo::atualizarProjeteis(p, m.grade, {1.5f, 0.35f, 1.5f}, 0.25f, 1.0f);
    CHECK(p.empty());
}

TEST_CASE("projétil acerta o jogador e informa o autor") {
    auto m = mundo::carregarDeTexto({"#######", "#@....#", "#######"});
    std::vector<jogo::Projetil> p{{{5.5f, 0.5f, 1.5f}, {-10.0f, 0.0f, 0.0f}, 4, jogo::TipoInimigo::Atirador}};
    auto r = jogo::atualizarProjeteis(p, m.grade, {1.5f, 0.35f, 1.5f}, 0.25f, 1.0f);
    REQUIRE(r.golpes.size() == 1);
    CHECK(r.golpes[0].dano == 4);
    CHECK(r.golpes[0].autor == jogo::TipoInimigo::Atirador);
    CHECK(p.empty());
}

TEST_CASE("projétil rápido não atravessa o jogador num passo grande") {
    auto m = mundo::carregarDeTexto({"#########", "#@......#", "#########"});
    std::vector<jogo::Projetil> p{{{7.5f, 0.5f, 1.5f}, {-10.0f, 0.0f, 0.0f}, 3, jogo::TipoInimigo::Atirador}};
    auto r = jogo::atualizarProjeteis(p, m.grade, {3.5f, 0.35f, 1.5f}, 0.25f, 0.6f);  // anda 6 células de uma vez
    REQUIRE(r.golpes.size() == 1);
    CHECK(r.golpes[0].dano == 3);
}

TEST_CASE("projétil voa em 3D e bate na beira do degrau") {
    // Plataforma de 2 blocos (colunas 1–2), degrau de 1 bloco (coluna 3), chão 0 (colunas 4–5)
    auto a = mundo::andarDeTexto({"#######", "#@....#", "#######"}, 1, {"0000000", "0884000", "0000000"},
                                 {"0000000", "0kkkkk0", "0000000"});
    const glm::vec3 de{5.5f, 0.5f, 1.5f}, alvo{1.5f, 2.35f, 1.5f};
    std::vector<jogo::Projetil> p{{de, glm::normalize(alvo - de) * 8.0f, 4, jogo::TipoInimigo::Atirador}};
    const auto r = jogo::atualizarProjeteis(p, a.grade, alvo, 0.25f, 2.0f);
    CHECK(r.golpes.empty());
    CHECK(p.empty());
}

TEST_CASE("projétil some ao passar abaixo do chão ou acima do teto") {
    auto a = mundo::andarDeTexto({"#######", "#@....#", "#######"}, 1);
    std::vector<jogo::Projetil> p{{{5.5f, 0.5f, 1.5f}, {0.0f, -2.0f, 0.0f}, 4, jogo::TipoInimigo::Atirador}};
    jogo::atualizarProjeteis(p, a.grade, {1.5f, 0.35f, 1.5f}, 0.25f, 1.0f);
    CHECK(p.empty());
}
