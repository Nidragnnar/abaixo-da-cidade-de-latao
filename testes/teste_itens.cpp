#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string
#include <random>
#include <variant>

#include "jogo/Itens.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

#include <glm/common.hpp>

TEST_CASE("tabelas da spec") {
    CHECK(jogo::dadosArma(jogo::TipoItem::Bacamarte).chumbos == 7);
    CHECK(jogo::dadosArma(jogo::TipoItem::Bacamarte).carga == 2);
    CHECK(jogo::dadosArma(jogo::TipoItem::Tesla).alcance == doctest::Approx(15.0f));
    CHECK(jogo::dadosArma(jogo::TipoItem::Tesla).carga == 0);
    CHECK(jogo::dadosArma(jogo::TipoItem::Martelo).empurrao == doctest::Approx(0.6f));
    CHECK(jogo::dadosArmadura(jogo::TipoItem::ArmaduraLatao).defesa == 5);
    CHECK(jogo::criarItem(jogo::TipoItem::Revolver).carga == 6);
}

TEST_CASE("nomes gerados") {
    CHECK(jogo::nomeItem(jogo::criarItem(jogo::TipoItem::Sabre)) == "Sabre de oficial");
    jogo::Item r{jogo::TipoItem::Revolver, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Dano, 3}}};
    CHECK(jogo::nomeItem(r) == "Revólver Calibrado +3");
    jogo::Item c{jogo::TipoItem::ChaveInglesa, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Velocidade, 15}}};
    CHECK(jogo::nomeItem(c) == "Chave inglesa Veloz +15%");
    jogo::Item l{jogo::TipoItem::ArmaduraLatao, jogo::Qualidade::ObraPrima,
                 {{jogo::TipoBonus::Defesa, 2}, {jogo::TipoBonus::VidaMaxima, 8}}};
    CHECK(jogo::nomeItem(l) == "Armadura de latão Reforçada +2");
    jogo::Item t = jogo::criarItem(jogo::TipoItem::Tonico);
    t.quantidade = 3;
    CHECK(jogo::nomeItem(t) == "Tônico restaurador ×3");
}

TEST_CASE("disponibilidade por andar") {
    std::mt19937 rng(1);
    for (int i = 0; i < 2000; ++i) {
        auto t = jogo::gerarItem(1, rng).tipo;
        REQUIRE(t != jogo::TipoItem::Bacamarte);
        REQUIRE(t != jogo::TipoItem::Tesla);
        REQUIRE(t != jogo::TipoItem::Colete);
        REQUIRE(t != jogo::TipoItem::ArmaduraLatao);
    }
    bool viuTesla = false, viuLatao = false;
    for (int i = 0; i < 5000; ++i) {
        auto t = jogo::gerarItem(5, rng).tipo;
        viuTesla |= t == jogo::TipoItem::Tesla;
        viuLatao |= t == jogo::TipoItem::ArmaduraLatao;
    }
    CHECK(viuTesla);
    CHECK(viuLatao);
}

TEST_CASE("frequência de qualidade no andar 1") {
    std::mt19937 rng(2);
    int comum = 0, obra = 0, total = 0;
    for (int i = 0; i < 10000; ++i) {
        auto it = jogo::gerarItem(1, rng);
        if (it.tipo == jogo::TipoItem::Tonico) continue;
        ++total;
        comum += it.qualidade == jogo::Qualidade::Comum;
        obra += it.qualidade == jogo::Qualidade::ObraPrima;
    }
    CHECK(comum * 100.0 / total == doctest::Approx(70.0).epsilon(0.05));
    CHECK(obra * 100.0 / total == doctest::Approx(5.0).epsilon(0.3));
}

TEST_CASE("bônus só onde cabem") {
    std::mt19937 rng(3);
    for (int i = 0; i < 5000; ++i) {
        auto it = jogo::gerarItem(6, rng);
        auto c = jogo::classe(it.tipo);
        REQUIRE(it.bonus.size() == (it.qualidade == jogo::Qualidade::Comum        ? 0u
                                    : it.qualidade == jogo::Qualidade::Modificada ? 1u
                                                                                  : 2u));
        if (it.bonus.size() == 2) REQUIRE(it.bonus[0].tipo != it.bonus[1].tipo);
        for (auto b : it.bonus) {
            if (c == jogo::ClasseItem::Armadura)
                REQUIRE((b.tipo == jogo::TipoBonus::Defesa || b.tipo == jogo::TipoBonus::VidaMaxima));
            if (c == jogo::ClasseItem::ArmaBranca)
                REQUIRE((b.tipo == jogo::TipoBonus::Dano || b.tipo == jogo::TipoBonus::Velocidade));
            if (it.tipo == jogo::TipoItem::Tesla)
                REQUIRE((b.tipo != jogo::TipoBonus::Carga && b.tipo != jogo::TipoBonus::Recarga));
            REQUIRE(b.valor > 0);
        }
    }
}

TEST_CASE("chão do andar") {
    for (uint32_t s = 1; s <= 100; ++s) {
        auto a = mundo::gerarAndar(s, 3);
        auto chao = jogo::gerarChao(a, s);
        int itens = 0, municao = 0, engrenagens = 0;
        for (auto& c : chao) {
            INFO("semente " << s);
            REQUIRE_FALSE(a.salas[0].contem(int(c.posicao.x), int(c.posicao.y)));
            REQUIRE(a.grade.em(int(c.posicao.x), int(c.posicao.y)) == mundo::Celula::Chao);
            if (std::holds_alternative<jogo::Item>(c.conteudo)) ++itens;
            else if (std::holds_alternative<jogo::MunicaoNoChao>(c.conteudo)) ++municao;
            else {
                ++engrenagens;
                int v = std::get<int>(c.conteudo);
                REQUIRE((v >= 5 * 3 && v <= 20 * 3));
            }
        }
        REQUIRE((itens >= 4 && itens <= 7 + static_cast<int>(a.pontosSecretos.size())));  // + prêmio de cada segredo
        REQUIRE((municao >= 3 && municao <= 5));
        REQUIRE((engrenagens >= 3 && engrenagens <= 6));
    }
}

TEST_CASE("o que os inimigos soltam") {
    std::mt19937 rng(4);
    int itens = 0, municao = 0;
    for (int i = 0; i < 4000; ++i)
        for (auto& c : jogo::soltarDoInimigo(jogo::TipoInimigo::Automato, {1, 1}, 1, rng)) {
            itens += std::holds_alternative<jogo::Item>(c.conteudo);
            municao += std::holds_alternative<jogo::MunicaoNoChao>(c.conteudo);
        }
    CHECK(itens / 4000.0 == doctest::Approx(0.25).epsilon(0.1));
    CHECK(municao / 4000.0 == doctest::Approx(0.35).epsilon(0.1));
    for (int i = 0; i < 200; ++i) {
        bool balas = false;
        for (auto& c : jogo::soltarDoInimigo(jogo::TipoInimigo::Atirador, {1, 1}, 1, rng))
            if (auto m = std::get_if<jogo::MunicaoNoChao>(&c.conteudo)) balas |= m->tipo == jogo::TipoMunicao::Balas;
        REQUIRE(balas);
    }
}

TEST_CASE("arma de impacto com mais dano é Pesada, não Afiada") {
    jogo::Item m{jogo::TipoItem::Martelo, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Dano, 2}}};
    CHECK(jogo::nomeItem(m).find("Pesado +2") != std::string::npos);
    jogo::Item c{jogo::TipoItem::ChaveInglesa, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Dano, 1}}};
    CHECK(jogo::nomeItem(c) == "Chave inglesa Pesada +1");
    jogo::Item s{jogo::TipoItem::Sabre, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Dano, 3}}};
    CHECK(jogo::nomeItem(s) == "Sabre de oficial Afiado +3");
}

TEST_CASE("ponto secreto tem item") {
    int conferidos = 0;
    for (uint32_t s = 1; s <= 300 && conferidos < 5; ++s) {
        const auto a = mundo::gerarAndar(s, 3 + static_cast<int>(s % 2));
        if (a.pontosSecretos.empty()) continue;
        const auto chao = jogo::gerarChao(a, s);
        for (const glm::vec2 p : a.pontosSecretos) {
            ++conferidos;
            bool tem = false;
            for (const auto& c : chao) tem |= c.posicao == p && std::holds_alternative<jogo::Item>(c.conteudo);
            CHECK(tem);
        }
    }
    CHECK(conferidos >= 5);
}

TEST_CASE("tudo no chão se alcança, nem que seja pulando") {
    for (uint32_t s = 1; s <= 300; ++s) {
        const int n = 1 + static_cast<int>(s % 6);
        const auto a = mundo::gerarAndar(s, n);
        mundo::RegrasPassagem pulando = mundo::SALTADOR;  // o pulo do jogador sobe 1 bloco, como o rato
        pulando.atravessaTrancada = pulando.atravessaElevador = true;
        for (const auto& c : jogo::gerarChao(a, s)) {
            const auto d = mundo::distanciasComAltura(a.grade, glm::ivec2(glm::floor(c.posicao)), pulando);
            INFO("semente ", s, " andar ", n, " coisa em ", c.posicao.x, ",", c.posicao.y);
            REQUIRE(d[static_cast<size_t>(static_cast<int>(a.inicio.y) * 64 + static_cast<int>(a.inicio.x))] >= 0);
        }
    }
}

TEST_CASE("obra-prima sorteada nunca é tônico") {
    std::mt19937 rng(5);
    for (int k = 0; k < 300; ++k) {
        const jogo::Item i = jogo::gerarObraPrima(4, rng);
        REQUIRE(i.tipo != jogo::TipoItem::Tonico);
        REQUIRE(i.qualidade == jogo::Qualidade::ObraPrima);
        REQUIRE(i.bonus.size() == 2);
    }
}
