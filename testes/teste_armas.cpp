#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string
#include <random>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "jogo/Armas.h"
#include "mundo/Gerador.h"

namespace {
jogo::Comandos comando(bool atacar, bool trocar = false, bool recarregar = false) {
    jogo::Comandos c;
    c.atacar = atacar;
    c.trocarArma = trocar;
    c.recarregar = recarregar;
    return c;
}
}

TEST_CASE("chave inglesa acerta à frente, empurra e interrompe") {
    auto a = mundo::andarDeTexto({"#######", "#@a...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Preparando;
    jogo::Armas armas;
    std::mt19937 rng(1);
    auto r = jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(r.acertos == 1);
    CHECK(v[0].vida <= 16 - 4);
    CHECK(v[0].vida >= 16 - 6);
    CHECK(v[0].estado == jogo::EstadoInimigo::Perseguindo);
    CHECK(glm::length(v[0].empurrao) == doctest::Approx(0.3f));
    CHECK(r.barulho == doctest::Approx(4.0f));
}

TEST_CASE("chave inglesa não acerta atrás nem longe") {
    auto a = mundo::andarDeTexto({"########", "#a@...a#", "########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas;
    std::mt19937 rng(1);
    CHECK(jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60).acertos == 0);
}

TEST_CASE("defesa do colosso reduz o dano") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas;
    std::mt19937 rng(1);
    jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(v[0].vida >= 40 - 4);  // 4–6 menos 2
    CHECK(v[0].vida <= 40 - 2);
}

TEST_CASE("segurar o botão respeita o intervalo") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas;
    std::mt19937 rng(1);
    int golpes = 0;
    for (int i = 0; i < 60; ++i)
        golpes += jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60).acertos;
    CHECK(golpes == 3);  // 1 s ÷ 0,35 s → golpes em 0; 0,35; 0,70
}

TEST_CASE("revólver acerta o primeiro inimigo e não atravessa parede") {
    auto a = mundo::andarDeTexto({"###########", "#@...a#..a#", "###########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas;
    armas.ativa = jogo::ArmaAtiva::Revolver;
    std::mt19937 rng(1);
    auto r = jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(r.disparou);
    CHECK(armas.carga == 5);
    CHECK(v[0].vida <= 16 - 7);
    CHECK(v[1].vida == 16);
    CHECK(r.barulho == doctest::Approx(10.0f));
}

TEST_CASE("revólver vazio avisa e recarga move balas da reserva") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Armas armas;
    armas.ativa = jogo::ArmaAtiva::Revolver;
    armas.carga = 0;
    armas.reserva = 4;
    std::mt19937 rng(1);
    auto r = jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK_FALSE(r.disparou);
    CHECK(r.mensagem == "Sem munição — R para recarregar");
    jogo::atualizarArmas(armas, comando(false, false, true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    for (int i = 0; i < 90; ++i) jogo::atualizarArmas(armas, comando(false), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.carga == 4);
    CHECK(armas.reserva == 0);
}

TEST_CASE("sem bala nenhuma a mensagem é outra") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Armas armas;
    armas.ativa = jogo::ArmaAtiva::Revolver;
    armas.carga = 0;
    armas.reserva = 0;
    std::mt19937 rng(1);
    CHECK(jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60).mensagem ==
          "Sem munição");
}

TEST_CASE("recarregar cheio não faz nada; trocar de arma cancela a recarga") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Armas armas;
    armas.ativa = jogo::ArmaAtiva::Revolver;
    std::mt19937 rng(1);
    jogo::atualizarArmas(armas, comando(false, false, true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.recarga == 0.0f);
    armas.carga = 2;
    jogo::atualizarArmas(armas, comando(false, false, true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.recarga > 0.0f);
    jogo::atualizarArmas(armas, comando(false, true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.ativa == jogo::ArmaAtiva::ChaveInglesa);
    CHECK(armas.recarga == 0.0f);
    CHECK(armas.carga == 2);
}

TEST_CASE("inimigo com vida zero é destruído") {
    auto a = mundo::andarDeTexto({"#####", "#@r.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].vida = 1;
    jogo::Armas armas;
    std::mt19937 rng(1);
    auto r = jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(r.destruidos == 1);
    CHECK(v[0].estado == jogo::EstadoInimigo::Destruido);
}

TEST_CASE("tiro reto passa por cima do rato colado nos pés; olhando para baixo acerta") {
    auto a = mundo::andarDeTexto({"#######", "#@r...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas;
    armas.ativa = jogo::ArmaAtiva::Revolver;
    std::mt19937 rng(1);
    auto reto = jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60, 0.0f);
    CHECK(reto.acertos == 0);
    armas.intervalo = 0;
    auto baixo = jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60,
                                      glm::radians(-20.0f));
    CHECK(baixo.acertos == 1);
}

TEST_CASE("rato longe é acertado mirando reto (tolerância vertical)") {
    auto a = mundo::andarDeTexto({"##########", "#@......r#", "##########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas;
    armas.ativa = jogo::ArmaAtiva::Revolver;
    std::mt19937 rng(1);
    CHECK(jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60, 0.0f).acertos == 1);
}

TEST_CASE("tiro para o teto não acerta o autômato à frente") {
    auto a = mundo::andarDeTexto({"#######", "#@..a.#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas;
    armas.ativa = jogo::ArmaAtiva::Revolver;
    std::mt19937 rng(1);
    CHECK(jogo::atualizarArmas(armas, comando(true), a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60,
                               glm::radians(50.0f)).acertos == 0);
}
