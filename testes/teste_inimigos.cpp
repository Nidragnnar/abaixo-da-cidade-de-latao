#include <doctest/doctest.h>

#include "jogo/Inimigos.h"
#include "mundo/Gerador.h"

TEST_CASE("escala por andar") {
    CHECK(jogo::escalar(10, 1) == 10);
    CHECK(jogo::escalar(10, 3) == 13);  // 10 × 1,30
    CHECK(jogo::escalar(40, 5) == 64);  // 40 × 1,60
    CHECK(jogo::escalar(2, 1) == 2);
}

TEST_CASE("inimigo criado com a tabela da spec") {
    auto c = jogo::criarInimigo(jogo::TipoInimigo::Colosso, {5, 5}, 1);
    CHECK(c.vida == 40);
    CHECK(jogo::dados(jogo::TipoInimigo::Colosso).defesa == 2);
    CHECK(jogo::dados(jogo::TipoInimigo::Rato).xp == 3);
    CHECK(c.estado == jogo::EstadoInimigo::Parado);
}

TEST_CASE("quantidade de inimigos por andar") {
    for (uint32_t s = 1; s <= 100; ++s)
        for (int andar : {1, 2, 5}) {
            auto a = mundo::gerarAndar(s, andar);
            auto v = jogo::gerarInimigos(a, s);
            INFO("semente " << s << " andar " << andar);
            REQUIRE(int(v.size()) >= 6 + 2 * andar - 2);  // tolera até 2 que não acharam lugar
            REQUIRE(int(v.size()) <= 10 + 2 * andar);
        }
}

TEST_CASE("tipos respeitam o andar mínimo") {
    for (uint32_t s = 1; s <= 100; ++s) {
        for (auto& i : jogo::gerarInimigos(mundo::gerarAndar(s, 1), s)) {
            REQUIRE(i.tipo != jogo::TipoInimigo::Atirador);
            REQUIRE(i.tipo != jogo::TipoInimigo::Colosso);
        }
        for (auto& i : jogo::gerarInimigos(mundo::gerarAndar(s, 2), s)) REQUIRE(i.tipo != jogo::TipoInimigo::Colosso);
    }
}

TEST_CASE("nenhum inimigo na sala inicial, em parede ou no elevador") {
    for (uint32_t s = 1; s <= 100; ++s) {
        auto a = mundo::gerarAndar(s, 3);
        for (auto& i : jogo::gerarInimigos(a, s)) {
            REQUIRE_FALSE(a.salas[0].contem(int(i.posicao.x), int(i.posicao.y)));
            REQUIRE(a.grade.em(int(i.posicao.x), int(i.posicao.y)) == mundo::Celula::Chao);
        }
    }
}

TEST_CASE("mesma semente, mesmos inimigos") {
    auto a = mundo::gerarAndar(9, 2);
    auto v1 = jogo::gerarInimigos(a, 9), v2 = jogo::gerarInimigos(a, 9);
    REQUIRE(v1.size() == v2.size());
    for (size_t i = 0; i < v1.size(); ++i) CHECK(v1[i].posicao == v2[i].posicao);
}

TEST_CASE("marcas do texto viram inimigos") {
    auto a = mundo::andarDeTexto({"######", "#@rac#", "#..g.#", "######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    REQUIRE(v.size() == 4);
    CHECK(v[0].tipo == jogo::TipoInimigo::Rato);
    CHECK(v[0].posicao == glm::vec2(2.5f, 1.5f));
    CHECK(a.grade.em(2, 1) == mundo::Celula::Chao);
}
