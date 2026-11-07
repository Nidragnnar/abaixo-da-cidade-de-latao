#include <doctest/doctest.h>

#include <glm/common.hpp>

#include "jogo/Inimigos.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

namespace {
int indice(int x, int y) { return y * mundo::LADO_ANDAR + x; }
}

TEST_CASE("andares de chefe") {
    CHECK(mundo::andarDeChefe(2));
    CHECK(mundo::andarDeChefe(4));
    CHECK(mundo::andarDeChefe(6));
    CHECK_FALSE(mundo::andarDeChefe(1));
    CHECK_FALSE(mundo::andarDeChefe(3));
    CHECK_FALSE(mundo::andarDeChefe(5));
    CHECK(mundo::ULTIMO_ANDAR == 6);
    CHECK(jogo::chefeDoAndar(2) == jogo::TipoInimigo::RainhaRatos);
    CHECK(jogo::chefeDoAndar(4) == jogo::TipoInimigo::Capataz);
    CHECK(jogo::chefeDoAndar(6) == jogo::TipoInimigo::GrandeMaquina);
}

TEST_CASE("arena na maior sala fora a inicial") {
    for (uint32_t s = 1; s <= 30; ++s)
        for (int n : {2, 4, 6}) {
            INFO("semente " << s << " andar " << n);
            const auto a = mundo::gerarAndar(s, n);
            REQUIRE(a.arena.has_value());
            REQUIRE(*a.arena != 0);
            const mundo::Sala& arena = a.salas[*a.arena];
            CHECK(arena.contem(a.elevador.x, a.elevador.y));
            for (size_t i = 1; i < a.salas.size(); ++i)
                CHECK(a.salas[i].largura * a.salas[i].altura <= arena.largura * arena.altura);
        }
}

TEST_CASE("andar sem chefe não tem arena") {
    for (int n : {1, 3, 5}) CHECK_FALSE(mundo::gerarAndar(7, n).arena.has_value());
}

TEST_CASE("arena plana e com teto alto") {
    for (uint32_t s = 1; s <= 100; ++s)
        for (int n : {4, 6}) {  // o esgoto já é quase plano; fábrica e minas têm relevo
            INFO("semente " << s << " andar " << n);
            const auto a = mundo::gerarAndar(s, n);
            const mundo::Sala& arena = a.salas[*a.arena];
            const glm::ivec2 c = arena.centro();
            const int chao = a.grade.chao(c.x, c.y);
            for (int y = arena.y; y < arena.y + arena.altura; ++y)
                for (int x = arena.x; x < arena.x + arena.largura; ++x) {
                    if (a.grade.solida(x, y)) continue;
                    REQUIRE(a.grade.chao(x, y) == chao);
                    REQUIRE(a.grade.rampa(x, y) == mundo::Rampa::Nenhuma);
                    REQUIRE(a.grade.detalhe(x, y) == mundo::Detalhe::Nenhum);
                    REQUIRE(a.grade.teto(x, y) - chao >= 16);  // chão da arena baixo: sempre cabe o teto alto
                }
        }
}

TEST_CASE("chefe no centro da arena e nenhum inimigo comum dentro") {
    for (uint32_t s = 1; s <= 30; ++s)
        for (int n : {2, 4, 6}) {
            INFO("semente " << s << " andar " << n);
            const auto a = mundo::gerarAndar(s, n);
            const mundo::Sala& arena = a.salas[*a.arena];
            int chefes = 0;
            for (const auto& i : jogo::gerarInimigos(a, s)) {
                if (jogo::dados(i.tipo).chefe) {
                    ++chefes;
                    CHECK(i.tipo == jogo::chefeDoAndar(n));
                    CHECK(i.posicao == glm::vec2(arena.centro()) + 0.5f);
                } else {
                    CHECK_FALSE(arena.contem(static_cast<int>(i.posicao.x), static_cast<int>(i.posicao.y)));
                }
            }
            CHECK(chefes == 1);
        }
}

TEST_CASE("arena alcançável a pé nos andares de chefe") {
    mundo::RegrasPassagem tudo = mundo::A_PE;
    tudo.atravessaTrancada = tudo.atravessaElevador = true;
    for (uint32_t s = 1; s <= 30; ++s)
        for (int n : {2, 4, 6}) {
            INFO("semente " << s << " andar " << n);
            const auto a = mundo::gerarAndar(s, n);
            const glm::ivec2 inicio(glm::floor(a.inicio));
            CHECK(mundo::distanciasComAltura(a.grade, a.elevador, tudo)[indice(inicio.x, inicio.y)] >= 0);
        }
}

TEST_CASE("elevador da arena não tampa corredor") {
    for (uint32_t s = 1; s <= 300; ++s)
        for (int n : {2, 4, 6}) {
            const auto a = mundo::gerarAndar(s, n);
            const mundo::Sala& arena = a.salas[*a.arena];
            for (const glm::ivec2 d : {glm::ivec2(1, 0), glm::ivec2(-1, 0), glm::ivec2(0, 1), glm::ivec2(0, -1)}) {
                const glm::ivec2 v = a.elevador + d;
                if (arena.contem(v.x, v.y)) continue;
                INFO("semente " << s << " andar " << n);
                REQUIRE(a.grade.em(v.x, v.y) == mundo::Celula::Parede);
            }
        }
}
