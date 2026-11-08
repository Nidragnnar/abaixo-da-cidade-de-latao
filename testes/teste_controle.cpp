#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string_view

#include "ui/Controle.h"

using ui::BotaoControle;

namespace {

ui::EstadoControle ligado() {
    ui::EstadoControle e;
    e.ligado = true;
    return e;
}

ui::EstadoControle com(BotaoControle b) {
    auto e = ligado();
    e.botoes[static_cast<size_t>(b)] = true;
    return e;
}

}

TEST_CASE("zona morta ignora drift") {
    CHECK(ui::aplicarZonaMorta(0.15f) == 0.0f);
    CHECK(ui::aplicarZonaMorta(-0.19f) == 0.0f);
    CHECK(ui::aplicarZonaMorta(1.0f) == doctest::Approx(1.0f));
    CHECK(ui::aplicarZonaMorta(-0.6f) == doctest::Approx(-0.5f));
    auto e = ligado();
    e.esqX = 0.15f;
    e.esqY = -0.1f;
    e.dirX = 0.18f;
    e.dirY = -0.19f;
    const auto m = ui::movimentoDoControle(e, 0.5f, 5, false);
    CHECK(m.frente == 0.0f);
    CHECK(m.lado == 0.0f);
    CHECK(m.giro == 0.0f);
    CHECK(m.inclinar == 0.0f);
    CHECK_FALSE(m.atacar);
}

TEST_CASE("controle desligado não comanda") {
    ui::EstadoControle cheio;
    cheio.esqX = cheio.esqY = cheio.dirX = cheio.dirY = 1.0f;
    cheio.gatilhoDir = 1.0f;
    cheio.botoes.fill(true);
    const auto m = ui::movimentoDoControle(cheio, 0.5f, 5, false);
    CHECK(m.frente == 0.0f);
    CHECK(m.giro == 0.0f);
    CHECK_FALSE(m.atacar);
    const auto ev = ui::eventosDoControle({}, cheio);
    CHECK_FALSE(ev.pular);
    CHECK_FALSE(ev.pausa);
    CHECK(ev.usarRapido == 0);
    const auto menu = ui::menuDoControle({}, cheio);
    CHECK(menu.mover == 0);
    CHECK_FALSE(menu.confirmar);
    CHECK_FALSE(menu.voltar);
}

TEST_CASE("analógico anda e mira") {
    auto e = ligado();
    e.esqY = -1.0f;  // para cima = frente
    e.esqX = 1.0f;
    e.dirX = 1.0f;
    e.dirY = -1.0f;  // para cima = olhar para cima
    auto m = ui::movimentoDoControle(e, 0.5f, 5, false);
    CHECK(m.frente == doctest::Approx(1.0f));
    CHECK(m.lado == doctest::Approx(1.0f));
    CHECK(m.giro == doctest::Approx(1.5f));
    CHECK(m.inclinar == doctest::Approx(1.5f));
    CHECK(ui::movimentoDoControle(e, 0.5f, 5, true).inclinar == doctest::Approx(-1.5f));
    CHECK(ui::movimentoDoControle(e, 0.5f, 6, false).giro == doctest::Approx(1.5f * 1.25f));
    e.dirX = 0.6f;  // curva: metade da inclinação útil → um quarto da velocidade
    CHECK(ui::movimentoDoControle(e, 0.5f, 5, false).giro == doctest::Approx(1.5f * 0.25f));
}

TEST_CASE("botões disparam uma vez") {
    const auto a = com(BotaoControle::A);
    CHECK(ui::eventosDoControle(ligado(), a).pular);
    CHECK_FALSE(ui::eventosDoControle(a, a).pular);  // segurado
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::Direita)).usarRapido == 2);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::Cima)).usarRapido == 1);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::Baixo)).usarRapido == 3);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::Esquerda)).usarRapido == 4);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::X)).usar);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::B)).recarregar);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::Y)).trocarArma);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::View)).inventario);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::LB)).mapa);
    CHECK(ui::eventosDoControle(ligado(), com(BotaoControle::Start)).pausa);
}

TEST_CASE("gatilho ataca a partir da metade") {
    auto e = ligado();
    e.gatilhoDir = 0.4f;
    CHECK_FALSE(ui::movimentoDoControle(e, 0.016f, 5, false).atacar);
    e.gatilhoDir = 0.6f;
    CHECK(ui::movimentoDoControle(e, 0.016f, 5, false).atacar);
}

TEST_CASE("menu: analógico move uma vez por cruzamento") {
    auto zero = ligado(), meio = ligado(), mais = ligado();
    meio.esqY = 0.8f;
    mais.esqY = 0.9f;
    CHECK(ui::menuDoControle(zero, meio).mover == 1);
    CHECK(ui::menuDoControle(meio, mais).mover == 0);
    CHECK(ui::menuDoControle(mais, zero).mover == 0);
    CHECK(ui::menuDoControle(zero, meio).mover == 1);
    auto cima = ligado();
    cima.esqY = -0.7f;
    CHECK(ui::menuDoControle(zero, cima).mover == -1);
    auto lado = ligado();
    lado.esqX = 0.7f;
    CHECK(ui::menuDoControle(zero, lado).ajustar == 1);
    CHECK(ui::menuDoControle(ligado(), com(BotaoControle::Esquerda)).ajustar == -1);
    CHECK(ui::menuDoControle(ligado(), com(BotaoControle::Baixo)).mover == 1);
}

TEST_CASE("menu: A confirma, B e Start voltam") {
    CHECK(ui::menuDoControle(ligado(), com(BotaoControle::A)).confirmar);
    CHECK(ui::menuDoControle(ligado(), com(BotaoControle::B)).voltar);
    CHECK(ui::menuDoControle(ligado(), com(BotaoControle::Start)).voltar);
    const auto a = com(BotaoControle::A);
    CHECK_FALSE(ui::menuDoControle(a, a).confirmar);
}

TEST_CASE("cursor anda e fica na janela") {
    auto e = ligado();
    e.esqX = -1.0f;
    CHECK(ui::moverCursor({10, 10}, e, 1.0f, {1280, 720}).x == 0.0f);
    e.esqX = 1.0f;
    CHECK(ui::moverCursor({0, 10}, e, 0.1f, {1280, 720}).x == doctest::Approx(70.0f));
    CHECK(ui::moverCursor({1275, 10}, e, 1.0f, {1280, 720}).x <= 1279.0f);
    // esquerdo parado: o direito move
    auto d = ligado();
    d.dirY = 1.0f;
    CHECK(ui::moverCursor({100, 100}, d, 0.1f, {1280, 720}).y == doctest::Approx(170.0f));
    CHECK(ui::moverCursor({100, 100}, ui::EstadoControle{}, 0.1f, {1280, 720}) == glm::vec2(100, 100));
}

TEST_CASE("nomes no controle") {
    CHECK(ui::nomeNoControle(jogo::AcaoJogo::Recarregar) == "B");
    CHECK(ui::nomeNoControle(jogo::AcaoJogo::Inventario) == "View");
    CHECK(ui::nomeNoControle(jogo::AcaoJogo::Atacar) == "RT");
    CHECK(ui::nomeNoControle(jogo::AcaoJogo::Item2) == "Direita");
    CHECK(ui::nomeNoControle(jogo::AcaoJogo::Frente) == "Analógico esquerdo");
}
