#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string

#include "ui/Menus.h"

namespace {

ui::EntradaMenu mover(int d) {
    ui::EntradaMenu e;
    e.mover = d;
    return e;
}
ui::EntradaMenu confirmar() {
    ui::EntradaMenu e;
    e.confirmar = true;
    return e;
}
ui::EntradaMenu voltar() {
    ui::EntradaMenu e;
    e.voltar = true;
    return e;
}
ui::EntradaMenu ajustar(int d) {
    ui::EntradaMenu e;
    e.ajustar = d;
    return e;
}
ui::EntradaMenu digitar(std::string t) {
    ui::EntradaMenu e;
    e.texto = std::move(t);
    return e;
}

}

TEST_CASE("menu principal: mover dá a volta e confirmar devolve a ação") {
    ui::Menus m;
    jogo::Config c;
    CHECK(m.tela() == ui::Tela::Principal);
    CHECK(m.titulo() == "Abaixo da Cidade de Latão");
    CHECK(m.itens(c) == std::vector<std::string>{"Novo jogo", "Placar", "Opções", "Sair"});
    CHECK(m.selecionado() == 0);
    m.atualizar(mover(-1), c);
    CHECK(m.selecionado() == 3);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::Sair);
    m.atualizar(mover(1), c);
    CHECK(m.selecionado() == 0);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::NovoJogo);
    CHECK(m.tela() == ui::Tela::Jogo);
}

TEST_CASE("Esc no menu principal não faz nada; placar abre e volta") {
    ui::Menus m;
    jogo::Config c;
    CHECK(m.atualizar(voltar(), c) == ui::Acao::Nenhuma);
    CHECK(m.tela() == ui::Tela::Principal);
    m.atualizar(mover(1), c);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::AbrirPlacar);
    CHECK(m.tela() == ui::Tela::Placar);
    CHECK(m.itens(c) == std::vector<std::string>{"Voltar"});
    m.atualizar(voltar(), c);
    CHECK(m.tela() == ui::Tela::Principal);
}

TEST_CASE("Esc no jogo pausa; Esc na pausa continua") {
    ui::Menus m;
    jogo::Config c;
    m.abrir(ui::Tela::Jogo);
    CHECK(m.atualizar(voltar(), c) == ui::Acao::Nenhuma);
    CHECK(m.tela() == ui::Tela::Pausa);
    CHECK(m.titulo() == "Pausa");
    CHECK(m.atualizar(voltar(), c) == ui::Acao::Continuar);
    CHECK(m.tela() == ui::Tela::Jogo);
}

TEST_CASE("foco perdido só pausa o jogo") {
    ui::Menus m;
    m.abrir(ui::Tela::Jogo);
    m.focoPerdido();
    CHECK(m.tela() == ui::Tela::Pausa);
    for (auto t : {ui::Tela::Principal, ui::Tela::Fim, ui::Tela::Opcoes, ui::Tela::Placar, ui::Tela::Pausa}) {
        m.abrir(t);
        m.focoPerdido();
        CHECK(m.tela() == t);
    }
}

TEST_CASE("tela de fim: Enter começa outra, Esc volta ao menu") {
    ui::Menus m;
    jogo::Config c;
    m.abrir(ui::Tela::Fim);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::NovoJogo);
    CHECK(m.tela() == ui::Tela::Jogo);
    m.abrir(ui::Tela::Fim);
    CHECK(m.atualizar(voltar(), c) == ui::Acao::Abandonar);
    CHECK(m.tela() == ui::Tela::Principal);
}

TEST_CASE("abandonar pede confirmação") {
    ui::Menus m;
    jogo::Config c;
    m.abrir(ui::Tela::Pausa);
    m.atualizar(mover(-1), c);
    CHECK(m.itens(c)[3] == "Abandonar partida");
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::Nenhuma);
    CHECK(m.itens(c)[3] == "Confirmar: abandonar?");
    m.atualizar(mover(1), c);
    m.atualizar(mover(-1), c);
    CHECK(m.itens(c)[3] == "Abandonar partida");
    m.atualizar(confirmar(), c);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::Abandonar);
    CHECK(m.tela() == ui::Tela::Principal);
}

TEST_CASE("opções ajustam dentro dos limites e voltam para quem abriu") {
    ui::Menus m;
    jogo::Config c;
    m.abrir(ui::Tela::Pausa);
    m.atualizar(mover(2), c);  // "Opções" é o terceiro item da pausa
    m.atualizar(confirmar(), c);
    REQUIRE(m.tela() == ui::Tela::Opcoes);
    CHECK(m.titulo() == "Opções");
    CHECK(m.itens(c) == std::vector<std::string>{"Sensibilidade do mouse: 5", "Volume dos efeitos: 8",
                                                 "Volume da música: 5", "Tela cheia: não", "Semente fixa: aleatória",
                                                 "Voltar"});
    for (int i = 0; i < 6; ++i) CHECK(m.atualizar(ajustar(1), c) == (i < 5 ? ui::Acao::OpcoesMudaram : ui::Acao::Nenhuma));
    CHECK(c.sensibilidade == 10);
    m.atualizar(mover(1), c);
    for (int i = 0; i < 9; ++i) m.atualizar(ajustar(-1), c);
    CHECK(c.volumeEfeitos == 0);
    m.atualizar(mover(1), c);
    m.atualizar(mover(1), c);
    CHECK(m.atualizar(ajustar(1), c) == ui::Acao::OpcoesMudaram);
    CHECK(c.telaCheia);
    CHECK(m.itens(c)[3] == "Tela cheia: sim");
    CHECK(m.atualizar(voltar(), c) == ui::Acao::SalvarOpcoes);
    CHECK(m.tela() == ui::Tela::Pausa);
    CHECK(m.selecionado() == 2);  // de volta em "Opções"
}

TEST_CASE("Voltar nas opções do menu principal volta ao principal") {
    ui::Menus m;
    jogo::Config c;
    m.atualizar(mover(1), c);
    m.atualizar(mover(1), c);
    m.atualizar(confirmar(), c);
    REQUIRE(m.tela() == ui::Tela::Opcoes);
    m.atualizar(mover(-1), c);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::SalvarOpcoes);
    CHECK(m.tela() == ui::Tela::Principal);
    CHECK(m.selecionado() == 2);
}

TEST_CASE("semente: só dígitos, até 4294967295") {
    ui::Menus m;
    jogo::Config c;
    m.abrir(ui::Tela::Opcoes);
    for (int i = 0; i < 4; ++i) m.atualizar(mover(1), c);
    CHECK(m.atualizar(digitar("12a3"), c) == ui::Acao::OpcoesMudaram);
    CHECK(c.semente == 123u);
    CHECK(m.itens(c)[4] == "Semente fixa: 123");
    ui::EntradaMenu apagar;
    apagar.apagar = true;
    for (int i = 0; i < 3; ++i) m.atualizar(apagar, c);
    CHECK_FALSE(c.semente.has_value());
    m.atualizar(digitar("4294967295"), c);
    CHECK(c.semente == 4294967295u);
    CHECK(m.atualizar(digitar("1"), c) == ui::Acao::Nenhuma);
    CHECK(c.semente == 4294967295u);
    for (int i = 0; i < 10; ++i) m.atualizar(apagar, c);
    CHECK_FALSE(c.semente.has_value());
    CHECK(m.itens(c)[4] == "Semente fixa: aleatória");
    m.atualizar(mover(-1), c);
    CHECK(m.atualizar(digitar("7"), c) == ui::Acao::Nenhuma);  // fora da semente, dígitos não fazem nada
}

TEST_CASE("mouse em cima seleciona e clique confirma") {
    ui::Menus m;
    jogo::Config c;
    ui::EntradaMenu e;
    e.itemSobMouse = 2;
    m.atualizar(e, c);
    CHECK(m.selecionado() == 2);
    e.clique = true;
    CHECK(m.atualizar(e, c) == ui::Acao::Nenhuma);
    CHECK(m.tela() == ui::Tela::Opcoes);
    e.itemSobMouse = 9;  // fora da lista: ignora
    e.clique = false;
    m.atualizar(e, c);
    CHECK(m.selecionado() == 0);
}

TEST_CASE("clique muda valores: esquerdo aumenta, direito diminui") {
    ui::Menus m;
    jogo::Config c;
    m.abrir(ui::Tela::Opcoes);
    ui::EntradaMenu e;
    e.itemSobMouse = 1;
    e.cliqueDireito = true;
    CHECK(m.atualizar(e, c) == ui::Acao::OpcoesMudaram);
    CHECK(c.volumeEfeitos == 7);
    e.cliqueDireito = false;
    e.clique = true;
    m.atualizar(e, c);
    CHECK(c.volumeEfeitos == 8);
}

TEST_CASE("mouse parado não desfaz a navegação pelo teclado") {
    ui::Menus m;
    jogo::Config c;
    ui::EntradaMenu e;
    e.itemSobMouse = 2;
    m.atualizar(e, c);
    REQUIRE(m.selecionado() == 2);
    e.mover = -1;  // mouse continua parado em cima do item 2
    m.atualizar(e, c);
    CHECK(m.selecionado() == 1);
    e.mover = 0;
    m.atualizar(e, c);
    CHECK(m.selecionado() == 1);
    // Abrir outra tela com o mouse parado no mesmo lugar não muda a seleção nova
    // (no laço, todo quadro leva o item sob o mouse)
    auto parado = [](ui::EntradaMenu x) {
        x.itemSobMouse = 2;
        return x;
    };
    m.atualizar(parado(confirmar()), c);  // item 1 = Placar
    REQUIRE(m.tela() == ui::Tela::Placar);
    m.atualizar(parado(voltar()), c);
    REQUIRE(m.tela() == ui::Tela::Principal);
    m.atualizar(parado(mover(1)), c);  // Opções
    m.atualizar(parado(confirmar()), c);
    REQUIRE(m.tela() == ui::Tela::Opcoes);
    m.atualizar(parado({}), c);  // o mouse ainda na altura do item 2
    CHECK(m.selecionado() == 0);
}

TEST_CASE("clique vale para o item sob o mouse mesmo depois do teclado") {
    ui::Menus m;
    jogo::Config c;
    ui::EntradaMenu e;
    e.itemSobMouse = 3;
    m.atualizar(e, c);
    m.atualizar(mover(-1), c);  // teclado leva para o item 2, mouse parado no 3
    e.clique = true;
    CHECK(m.atualizar(e, c) == ui::Acao::Sair);
}

TEST_CASE("sem partida salva o menu é o de antes") {
    ui::Menus m;
    jogo::Config c;
    CHECK(m.itens(c) == std::vector<std::string>{"Novo jogo", "Placar", "Opções", "Sair"});
}

TEST_CASE("com partida salva, o primeiro item é Continuar partida e devolve ContinuarSalva") {
    ui::Menus m;
    jogo::Config c;
    m.definirPartidaSalva(true);
    CHECK(m.itens(c) == std::vector<std::string>{"Continuar partida", "Novo jogo", "Placar", "Opções", "Sair"});
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::ContinuarSalva);
    CHECK(m.tela() == ui::Tela::Jogo);
}

TEST_CASE("Novo jogo com partida salva pede confirmação") {
    ui::Menus m;
    jogo::Config c;
    m.definirPartidaSalva(true);
    m.atualizar(mover(1), c);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::Nenhuma);
    CHECK(m.itens(c)[1] == "Apagar a partida salva?");
    m.atualizar(mover(1), c);  // mudou de item: desiste
    m.atualizar(mover(-1), c);
    CHECK(m.itens(c)[1] == "Novo jogo");
    m.atualizar(confirmar(), c);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::NovoJogo);
    CHECK(m.tela() == ui::Tela::Jogo);
}

TEST_CASE("com partida salva, placar e opções voltam para o item certo") {
    ui::Menus m;
    jogo::Config c;
    m.definirPartidaSalva(true);
    m.atualizar(mover(2), c);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::AbrirPlacar);
    m.atualizar(voltar(), c);
    CHECK(m.tela() == ui::Tela::Principal);
    CHECK(m.selecionado() == 2);
    m.atualizar(mover(1), c);
    m.atualizar(confirmar(), c);
    REQUIRE(m.tela() == ui::Tela::Opcoes);
    m.atualizar(voltar(), c);
    CHECK(m.selecionado() == 3);
}

TEST_CASE("pausa tem Salvar e sair e devolve SalvarESair") {
    ui::Menus m;
    jogo::Config c;
    m.abrir(ui::Tela::Pausa);
    CHECK(m.itens(c) == std::vector<std::string>{"Continuar", "Salvar e sair", "Opções", "Abandonar partida"});
    m.atualizar(mover(1), c);
    CHECK(m.atualizar(confirmar(), c) == ui::Acao::SalvarESair);
}

TEST_CASE("aviso do menu some ao trocar de tela") {
    ui::Menus m;
    jogo::Config c;
    m.abrir(ui::Tela::Pausa);
    m.definirAviso("Não foi possível salvar");
    m.atualizar(mover(1), c);
    CHECK(m.aviso() == "Não foi possível salvar");  // continua na pausa
    m.atualizar(mover(-1), c);
    REQUIRE(m.atualizar(confirmar(), c) == ui::Acao::Continuar);
    CHECK(m.aviso().empty());
    m.atualizar(voltar(), c);  // pausa de novo
    CHECK(m.aviso().empty());
}
