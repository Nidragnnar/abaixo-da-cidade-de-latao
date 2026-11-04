#include <doctest/doctest.h>

#include "jogo/Inventario.h"

TEST_CASE("inventário inicial") {
    jogo::Inventario inv;
    CHECK(inv.armaBranca().tipo == jogo::TipoItem::ChaveInglesa);
    CHECK(inv.armaDeFogo().tipo == jogo::TipoItem::Revolver);
    CHECK(inv.armaDeFogo().carga == 6);
    CHECK(inv.armadura().tipo == jogo::TipoItem::Casaco);
    REQUIRE(inv.espaco(0).has_value());
    CHECK(inv.espaco(0)->quantidade == 2);
}

TEST_CASE("tônicos empilham até 9") {
    jogo::Inventario inv;
    for (int i = 0; i < 8; ++i) inv.adicionar(jogo::criarItem(jogo::TipoItem::Tonico));
    CHECK(inv.espaco(0)->quantidade == 9);
    CHECK(inv.espaco(1)->quantidade == 1);
}

TEST_CASE("cheio recusa") {
    jogo::Inventario inv;
    for (int i = 1; i < jogo::ESPACOS_INVENTARIO; ++i) REQUIRE(inv.adicionar(jogo::criarItem(jogo::TipoItem::Sabre)));
    CHECK_FALSE(inv.adicionar(jogo::criarItem(jogo::TipoItem::Martelo)));
}

TEST_CASE("equipar troca no mesmo espaço") {
    jogo::Inventario inv;
    inv.adicionar(jogo::criarItem(jogo::TipoItem::Martelo));  // espaço 1
    REQUIRE(inv.equipar(1));
    CHECK(inv.armaBranca().tipo == jogo::TipoItem::Martelo);
    CHECK(inv.espaco(1)->tipo == jogo::TipoItem::ChaveInglesa);
    CHECK_FALSE(inv.equipar(0));  // tônico não equipa
    CHECK_FALSE(inv.equipar(5));  // vazio
}

TEST_CASE("remover e trocar espaços") {
    jogo::Inventario inv;
    inv.adicionar(jogo::criarItem(jogo::TipoItem::Sabre));  // espaço 1
    inv.trocar(0, 7);
    CHECK_FALSE(inv.espaco(0).has_value());
    CHECK(inv.espaco(7)->tipo == jogo::TipoItem::Tonico);
    auto r = inv.remover(1);
    REQUIRE(r.has_value());
    CHECK(r->tipo == jogo::TipoItem::Sabre);
    CHECK_FALSE(inv.espaco(1).has_value());
}

TEST_CASE("consumir tônico") {
    jogo::Inventario inv;
    CHECK(inv.consumirTonico(0));
    CHECK(inv.espaco(0)->quantidade == 1);
    CHECK(inv.consumirTonico(0));
    CHECK_FALSE(inv.espaco(0).has_value());
    CHECK_FALSE(inv.consumirTonico(0));
}

TEST_CASE("atributos com nível e bônus") {
    jogo::Inventario inv;
    inv.adicionar({jogo::TipoItem::Colete, jogo::Qualidade::ObraPrima,
                   {{jogo::TipoBonus::Defesa, 2}, {jogo::TipoBonus::VidaMaxima, 7}}});
    inv.equipar(1);
    auto a = jogo::calcularAtributos(inv, 3);
    CHECK(a.danoBase == 2);
    CHECK(a.defesa == 3 + 2);
    CHECK(a.vidaMaxima == 30 + 16 + 7);
}

TEST_CASE("cada tipo de item tem o seu espaço de equipamento") {
    CHECK(jogo::espacoDeEquipamento(jogo::TipoItem::Sabre) == 0);
    CHECK(jogo::espacoDeEquipamento(jogo::TipoItem::ChaveInglesa) == 0);
    CHECK(jogo::espacoDeEquipamento(jogo::TipoItem::Tesla) == 1);
    CHECK(jogo::espacoDeEquipamento(jogo::TipoItem::Colete) == 2);
    CHECK_FALSE(jogo::espacoDeEquipamento(jogo::TipoItem::Tonico).has_value());
}
