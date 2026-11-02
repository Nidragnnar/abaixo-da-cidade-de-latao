#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string

#include "jogo/Demo.h"
#include "jogo/Partida.h"

namespace {

// Roda o piloto até descer ou acabar o tempo; devolve se pegou a válvula-chave antes de descer
bool pilotar(jogo::Partida& p, float segundos) {
    jogo::PilotoDemo piloto;
    const float dt = 1.0f / 60.0f;
    bool pegou = false;
    for (int i = 0; i < static_cast<int>(segundos * 60) && p.trocasDeAndar() == 0; ++i) {
        p.atualizar(piloto.comandar(p, dt), dt);
        if (p.trocasDeAndar() == 0 && !p.andar().chave) pegou = true;
    }
    return pegou;
}

}

TEST_CASE("piloto chega ao elevador em 50 sementes") {
#ifdef NDEBUG
    const uint32_t sementes = 50;
#else
    const uint32_t sementes = 20;  // Debug é bem mais lento
#endif
    for (uint32_t s = 1; s <= sementes; ++s) {
        jogo::Partida p(s, 1);
        p.tornarInvencivel();
        pilotar(p, 180);
        INFO("semente ", s);
        CHECK(p.trocasDeAndar() == 1);
    }
}

TEST_CASE("piloto pega a válvula antes de descer num andar trancado") {
    int testados = 0;
    for (uint32_t s = 1; s <= 200 && testados < 3; ++s) {
        jogo::Partida p(s, 2);  // tranca só a partir do andar 2
        if (!p.andar().chave) continue;
        ++testados;
        p.tornarInvencivel();
        const bool pegou = pilotar(p, 180);
        INFO("semente ", s);
        CHECK(p.trocasDeAndar() == 1);
        CHECK(pegou);
    }
    CHECK(testados == 3);
}
