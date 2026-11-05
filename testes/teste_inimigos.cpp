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

TEST_CASE("voadores na tabela") {
    using jogo::TipoInimigo;
    CHECK(jogo::dados(TipoInimigo::Vespa).voa);
    CHECK(jogo::dados(TipoInimigo::Dirigivel).voa);
    CHECK_FALSE(jogo::dados(TipoInimigo::Rato).voa);
    CHECK_FALSE(jogo::dados(TipoInimigo::Colosso).voa);
    CHECK(jogo::dados(TipoInimigo::Vespa).vida == 6);
    CHECK(jogo::dados(TipoInimigo::Vespa).xp == 5);
    CHECK(jogo::dados(TipoInimigo::Vespa).danoMin == 2);
    CHECK(jogo::dados(TipoInimigo::Vespa).danoMax == 4);
    CHECK(jogo::dados(TipoInimigo::Dirigivel).vida == 18);
    CHECK(jogo::dados(TipoInimigo::Dirigivel).defesa == 1);
    CHECK(jogo::dados(TipoInimigo::Dirigivel).danoMin == 5);
    CHECK(jogo::dados(TipoInimigo::Dirigivel).danoMax == 8);
    CHECK(jogo::dados(TipoInimigo::Dirigivel).xp == 12);
}

TEST_CASE("voadores só a partir do andar certo") {
    bool vespaNo2 = false, vespaNo3 = false, dirigivelNo3 = false;
    for (uint32_t s = 1; s <= 200; ++s) {
        for (auto& i : jogo::gerarInimigos(mundo::gerarAndar(s, 1), s)) REQUIRE_FALSE(jogo::dados(i.tipo).voa);
        for (auto& i : jogo::gerarInimigos(mundo::gerarAndar(s, 2), s)) {
            REQUIRE(i.tipo != jogo::TipoInimigo::Dirigivel);
            vespaNo2 |= i.tipo == jogo::TipoInimigo::Vespa;
        }
        for (auto& i : jogo::gerarInimigos(mundo::gerarAndar(s, 3), s)) {
            vespaNo3 |= i.tipo == jogo::TipoInimigo::Vespa;
            dirigivelNo3 |= i.tipo == jogo::TipoInimigo::Dirigivel;
        }
    }
    CHECK(vespaNo2);
    CHECK(vespaNo3);
    CHECK(dirigivelNo3);
}

TEST_CASE("marcas v e d viram voadores") {
    auto a = mundo::andarDeTexto({"#####", "#@vd#", "#####"}, 3);
    auto v = jogo::inimigosDasMarcas(a);
    REQUIRE(v.size() == 2);
    CHECK(v[0].tipo == jogo::TipoInimigo::Vespa);
    CHECK(v[1].tipo == jogo::TipoInimigo::Dirigivel);
    CHECK(v[1].posicao == glm::vec2(3.5f, 1.5f));
}
