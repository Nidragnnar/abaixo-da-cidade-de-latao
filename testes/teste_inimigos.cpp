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
            std::erase_if(v, [](const jogo::Inimigo& i) { return jogo::dados(i.tipo).chefe; });  // o chefe é a mais
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

TEST_CASE("chefes na tabela") {
    using jogo::TipoInimigo;
    const auto& r = jogo::dados(TipoInimigo::RainhaRatos);
    CHECK(r.chefe);
    CHECK(r.vida == 140); CHECK(r.defesa == 0); CHECK(r.velocidade == doctest::Approx(2.5f));
    CHECK(r.raio == doctest::Approx(0.5f)); CHECK(r.altura == doctest::Approx(0.8f)); CHECK(r.xp == 60);
    const auto& c = jogo::dados(TipoInimigo::Capataz);
    CHECK(c.chefe);
    CHECK(c.vida == 240); CHECK(c.defesa == 3); CHECK(c.velocidade == doctest::Approx(1.0f));
    CHECK(c.raio == doctest::Approx(0.6f)); CHECK(c.altura == doctest::Approx(1.4f)); CHECK(c.xp == 120);
    const auto& m = jogo::dados(TipoInimigo::GrandeMaquina);
    CHECK(m.chefe);
    CHECK(m.vida == 420); CHECK(m.defesa == 2); CHECK(m.velocidade == doctest::Approx(0.0f));
    CHECK(m.raio == doctest::Approx(0.8f)); CHECK(m.altura == doctest::Approx(1.8f)); CHECK(m.xp == 250);
    // Chefes não escalam com o andar
    CHECK(jogo::criarInimigo(TipoInimigo::GrandeMaquina, {5, 5}, 6).vida == 420);
}

TEST_CASE("inimigos comuns com raio 0,25 e sem marca de chefe") {
    for (auto t : {jogo::TipoInimigo::Rato, jogo::TipoInimigo::Automato, jogo::TipoInimigo::Atirador,
                   jogo::TipoInimigo::Colosso, jogo::TipoInimigo::Vespa, jogo::TipoInimigo::Dirigivel}) {
        CHECK(jogo::dados(t).raio == doctest::Approx(0.25f));
        CHECK_FALSE(jogo::dados(t).chefe);
    }
}

TEST_CASE("chefes nunca saem no sorteio comum") {
    for (uint32_t s = 1; s <= 100; ++s)
        for (int n : {1, 3, 5, 7})
            for (auto& i : jogo::gerarInimigos(mundo::gerarAndar(s, n), s)) REQUIRE_FALSE(jogo::dados(i.tipo).chefe);
}

TEST_CASE("marcas R, P e M viram chefes") {
    auto a = mundo::andarDeTexto({"######", "#@RPM#", "######"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    REQUIRE(v.size() == 3);
    CHECK(v[0].tipo == jogo::TipoInimigo::RainhaRatos);
    CHECK(v[1].tipo == jogo::TipoInimigo::Capataz);
    CHECK(v[2].tipo == jogo::TipoInimigo::GrandeMaquina);
}
