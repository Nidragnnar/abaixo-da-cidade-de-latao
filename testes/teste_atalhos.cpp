#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string_view
#include <set>
#include <string>

#include "jogo/Atalhos.h"

using jogo::AcaoJogo;

TEST_CASE("padrão é o jogo de hoje") {
    const auto a = jogo::Atalhos::padrao();
    CHECK(a.de(AcaoJogo::Frente) == 26);  // W
    CHECK(a.de(AcaoJogo::Esquerda) == 4);  // A
    CHECK(a.de(AcaoJogo::Pular) == 44);
    CHECK(a.de(AcaoJogo::Usar) == 8);  // E
    CHECK(a.de(AcaoJogo::Mapa) == 43);
    CHECK(a.de(AcaoJogo::Atacar) == jogo::BOTAO_MOUSE + 1);
    CHECK(a.de(AcaoJogo::Item4) == 33);
}

TEST_CASE("definir tecla já usada troca as duas") {
    auto a = jogo::Atalhos::padrao();
    a.definir(AcaoJogo::Pular, 8 /* E */);
    CHECK(a.de(AcaoJogo::Pular) == 8);
    CHECK(a.de(AcaoJogo::Usar) == 44);
    a.definir(AcaoJogo::Atacar, 9 /* F, livre */);
    CHECK(a.de(AcaoJogo::Atacar) == 9);
    CHECK(a.acaoDa(9) == AcaoJogo::Atacar);
    CHECK_FALSE(a.acaoDa(jogo::BOTAO_MOUSE + 1));
}

TEST_CASE("Esc e teclas fora da tabela não são aceitas") {
    CHECK_FALSE(jogo::teclaAceita(41));
    CHECK_FALSE(jogo::teclaAceita(0));
    CHECK_FALSE(jogo::teclaAceita(500));
    CHECK_FALSE(jogo::teclaAceita(jogo::BOTAO_MOUSE + 6));
    CHECK(jogo::teclaAceita(jogo::BOTAO_MOUSE + 3));
    CHECK(jogo::teclaAceita(225));  // Shift esquerdo
}

TEST_CASE("nomes e ids vão e voltam") {
    int aceitas = 0;
    for (int t = 0; t < jogo::BOTAO_MOUSE + 10; ++t) {
        if (!jogo::teclaAceita(t)) continue;
        ++aceitas;
        INFO("tecla " << t);
        CHECK(jogo::teclaDoId(jogo::idDaTecla(t)) == t);
        CHECK_FALSE(jogo::nomeDaTecla(t).empty());
    }
    CHECK(aceitas > 100);
    CHECK(jogo::nomeDaTecla(44) == "Espaço");
    CHECK(jogo::idDaTecla(44) == "espaco");
    CHECK(jogo::idDaTecla(225) == "shift_esq");
    CHECK(jogo::nomeDaTecla(jogo::BOTAO_MOUSE + 1) == "Botão esquerdo");
    CHECK(jogo::idDaTecla(jogo::BOTAO_MOUSE + 4) == "mouse4");
    CHECK(jogo::nomeDaTecla(26) == "W");
    CHECK(jogo::idDaTecla(58) == "f1");
    CHECK_FALSE(jogo::teclaDoId("banana"));
    for (int i = 0; i < jogo::TOTAL_ACOES; ++i) {
        const auto acao = static_cast<AcaoJogo>(i);
        CHECK_FALSE(jogo::idDaAcao(acao).empty());
        CHECK(jogo::acaoDoId(jogo::idDaAcao(acao)) == acao);
    }
    CHECK(jogo::idDaAcao(AcaoJogo::TrocarArma) == "trocar_arma");
}

TEST_CASE("repetidas são corrigidas sem perder tecla") {
    auto a = jogo::Atalhos::padrao();
    a.tecla[static_cast<int>(AcaoJogo::Usar)] = 44;       // mesma do Pular
    a.tecla[static_cast<int>(AcaoJogo::Recarregar)] = 8;  // ocupa o padrão do Usar (E)
    a.tecla[static_cast<int>(AcaoJogo::Mapa)] = 41;       // Esc: não aceita
    a.corrigirRepetidas();
    std::set<int> usadas;
    for (int t : a.tecla) {
        CHECK(jogo::teclaAceita(t));
        usadas.insert(t);
    }
    CHECK(usadas.size() == jogo::TOTAL_ACOES);
    CHECK(a.de(AcaoJogo::Pular) == 44);  // a primeira fica
    CHECK(a.de(AcaoJogo::Recarregar) == 8);
    CHECK(a.de(AcaoJogo::Mapa) == 43);   // voltou ao padrão
}

TEST_CASE("apertada lê teclado e mouse") {
    uint8_t teclado[512] = {};
    teclado[44] = 1;
    CHECK(jogo::apertada(44, teclado, 0));
    CHECK_FALSE(jogo::apertada(26, teclado, 0));
    CHECK(jogo::apertada(jogo::BOTAO_MOUSE + 3, teclado, 0b100));
    CHECK_FALSE(jogo::apertada(jogo::BOTAO_MOUSE + 1, teclado, 0b100));
    CHECK_FALSE(jogo::apertada(44, nullptr, 0));
}

TEST_CASE("comTeclas troca o id pelo nome") {
    const auto nome = [](AcaoJogo a) { return std::string(jogo::nomeDaTecla(jogo::Atalhos::padrao().de(a))); };
    CHECK(jogo::comTeclas("Sem munição — {recarregar} para recarregar", nome) == "Sem munição — R para recarregar");
    CHECK(jogo::comTeclas("Inventário ({inventario} fecha)", nome) == "Inventário (I fecha)");
    CHECK(jogo::comTeclas("sem chaves {desconhecida}", nome) == "sem chaves {desconhecida}");
}
