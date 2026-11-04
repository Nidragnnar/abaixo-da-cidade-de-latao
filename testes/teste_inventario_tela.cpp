#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string

#include "jogo/Inventario.h"
#include "render/TelaInventario.h"
#include "render/Texto.h"

namespace {
glm::vec2 centro(render::Retangulo r) { return {r.x + r.largura / 2.0f, r.y + r.altura / 2.0f}; }
}

TEST_CASE("espaço sob o mouse") {
    auto l = render::layoutInventario(480, 270);
    CHECK(render::espacoSob(l, centro(l.espacos[5])) == 5);
    CHECK(render::espacoSob(l, centro(l.equipados[2])) == 18);
    CHECK_FALSE(render::espacoSob(l, {0, 0}).has_value());
}

TEST_CASE("todos os espaços cabem na tela baixa sem se sobrepor") {
    auto l = render::layoutInventario(480, 270);
    for (auto& r : l.espacos) {
        REQUIRE(r.x >= 0);
        REQUIRE(r.y >= 0);
        REQUIRE(r.x + r.largura <= 480);
        REQUIRE(r.y + r.altura <= 270);
    }
    for (int a = 0; a < 16; ++a)
        for (int b = a + 1; b < 16; ++b) {
            auto& p = l.espacos[a];
            auto& q = l.espacos[b];
            REQUIRE((p.x + p.largura <= q.x || q.x + q.largura <= p.x || p.y + p.altura <= q.y ||
                     q.y + q.altura <= p.y));
        }
}

TEST_CASE("espaços 1 a 4 são a primeira linha") {
    auto l = render::layoutInventario(480, 270);
    CHECK(l.espacos[0].y == l.espacos[3].y);
    CHECK(l.espacos[4].y > l.espacos[0].y);
}

TEST_CASE("descrição do item") {
    CHECK(render::descricaoItem(jogo::criarItem(jogo::TipoItem::Sabre)) == "Dano 6–9   Intervalo 0,50 s");
    CHECK(render::descricaoItem(jogo::criarItem(jogo::TipoItem::Colete)) == "Defesa 3");
    CHECK(render::descricaoItem(jogo::criarItem(jogo::TipoItem::Tonico)) == "Cura 15 de vida");
    jogo::Item r{jogo::TipoItem::Revolver, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Dano, 2}}};
    CHECK(render::descricaoItem(r) == "Dano 9–12   Intervalo 0,40 s   Carga 6   Recarga 1,50 s");
}

TEST_CASE("inventário desenha um ícone por item e o arrastado no mouse") {
    jogo::Inventario inv;  // 2 tônicos no espaço 0 + 3 equipados
    auto l = render::layoutInventario(480, 270);
    auto parado = render::construirInventario(inv, l, std::nullopt, std::nullopt, {0, 0});
    CHECK(parado.icones.size() == 4 * 6);
    auto arrastando = render::construirInventario(inv, l, std::nullopt, 0, {200, 100});
    CHECK(arrastando.icones.size() == 4 * 6);  // o do espaço 0 sai do lugar e vai para o mouse
    CHECK(arrastando.icones.back().posicao.x == doctest::Approx(200 - 8.0f));
}

TEST_CASE("descrição mostra a recarga das armas de fogo") {
    CHECK(render::descricaoItem(jogo::criarItem(jogo::TipoItem::Revolver)).find("Recarga 1,50 s") != std::string::npos);
    jogo::Item b{jogo::TipoItem::Bacamarte, jogo::Qualidade::ObraPrima,
                 {{jogo::TipoBonus::Dano, 4}, {jogo::TipoBonus::Recarga, 29}}};
    CHECK(render::descricaoItem(b).find("Recarga 1,56 s") != std::string::npos);  // 2,2 × 0,71
}

TEST_CASE("textos do inventário não saem do painel") {
    auto l = render::layoutInventario(480, 270);
    const float direita = static_cast<float>(l.painel.x + l.painel.largura);
    for (auto tipo : {jogo::TipoItem::Tesla, jogo::TipoItem::Bacamarte, jogo::TipoItem::Revolver}) {
        jogo::Inventario inv;
        jogo::Item arma = jogo::criarItem(tipo);
        arma.qualidade = jogo::Qualidade::ObraPrima;
        arma.bonus = {{jogo::TipoBonus::Dano, 9}, {jogo::TipoBonus::Velocidade, 20}};
        inv.adicionar(arma);
        inv.equipar(1);
        auto e = render::construirInventario(inv, l, 17, std::nullopt, {0, 0});  // mouse sobre a arma de fogo
        for (const auto& v : e.texto) REQUIRE(v.posicao.x <= direita);
    }
}

TEST_CASE("caixa de largar visível dentro do painel, sem cobrir espaços nem equipados") {
    auto l = render::layoutInventario(480, 270);
    const auto& r = l.largar;
    CHECK(r.largura >= 80);
    CHECK(r.altura >= 20);
    CHECK(r.x >= l.painel.x);
    CHECK(r.y >= l.painel.y);
    CHECK(r.x + r.largura <= l.painel.x + l.painel.largura);
    CHECK(r.y + r.altura <= l.painel.y + l.painel.altura);
    const auto separados = [](const render::Retangulo& p, const render::Retangulo& q) {
        return p.x + p.largura <= q.x || q.x + q.largura <= p.x || p.y + p.altura <= q.y || q.y + q.altura <= p.y;
    };
    for (const auto& e : l.espacos) CHECK(separados(r, e));
    for (const auto& e : l.equipados) CHECK(separados(r, e));
    CHECK(render::sobLargar(l, centro(r)));
    CHECK_FALSE(render::sobLargar(l, centro(l.espacos[0])));
}

TEST_CASE("soltar fora do painel também larga") {
    auto l = render::layoutInventario(480, 270);
    CHECK(render::sobLargar(l, {2, 2}));
    CHECK(render::sobLargar(l, {static_cast<float>(l.painel.x + l.painel.largura + 5), 100}));
    CHECK_FALSE(render::sobLargar(l, {static_cast<float>(l.painel.x + 4), static_cast<float>(l.painel.y + 4)}));
}

TEST_CASE("textos não invadem a caixa de largar (só o rótulo dela)") {
    auto l = render::layoutInventario(480, 270);
    const auto& r = l.largar;
    for (auto tipo : {jogo::TipoItem::Tesla, jogo::TipoItem::Bacamarte, jogo::TipoItem::Revolver}) {
        jogo::Inventario inv;
        jogo::Item arma = jogo::criarItem(tipo);
        arma.qualidade = jogo::Qualidade::ObraPrima;
        arma.bonus = {{jogo::TipoBonus::Dano, 9}, {jogo::TipoBonus::Velocidade, 20}};
        inv.adicionar(arma);
        auto e = render::construirInventario(inv, l, 1, std::nullopt, {0, 0});  // detalhes do item novo
        const auto rotulo = render::construirTexto("Largar aqui", {0, 0}, 1, glm::vec4(1));
        size_t dentro = 0;
        for (size_t g = 0; g + 6 <= e.texto.size(); g += 6) {
            const glm::vec2 a = e.texto[g].posicao;  // canto de cima à esquerda do glifo
            if (a.x + 8 > r.x && a.x < r.x + r.largura && a.y + 8 > r.y && a.y < r.y + r.altura) dentro += 6;
        }
        CHECK(dentro == rotulo.size());
    }
}

TEST_CASE("detalhes de obra-prima com nome longo cabem embaixo no painel") {
    auto l = render::layoutInventario(480, 270);
    const float baixo = static_cast<float>(l.painel.y + l.painel.altura);
    for (auto tipo : {jogo::TipoItem::Tesla, jogo::TipoItem::Bacamarte, jogo::TipoItem::Revolver, jogo::TipoItem::ArmaduraLatao}) {
        jogo::Inventario inv;
        jogo::Item item = jogo::criarItem(tipo);
        item.qualidade = jogo::Qualidade::ObraPrima;
        item.bonus = jogo::classe(tipo) == jogo::ClasseItem::Armadura
                         ? std::vector<jogo::Bonus>{{jogo::TipoBonus::Defesa, 9}, {jogo::TipoBonus::VidaMaxima, 20}}
                         : std::vector<jogo::Bonus>{{jogo::TipoBonus::Dano, 9}, {jogo::TipoBonus::Recarga, 30}};
        inv.adicionar(item);
        auto e = render::construirInventario(inv, l, 1, std::nullopt, {0, 0});
        for (const auto& v : e.texto) REQUIRE(v.posicao.y <= baixo);
    }
}

TEST_CASE("mensagem da partida aparece acima do painel do inventário") {
    auto l = render::layoutInventario(480, 270);
    jogo::Inventario inv;
    const auto sem = render::construirInventario(inv, l, std::nullopt, std::nullopt, {0, 0});
    const auto com = render::construirInventario(inv, l, std::nullopt, std::nullopt, {0, 0}, "Vida cheia");
    REQUIRE(com.texto.size() == sem.texto.size() + 6 * 10);  // "Vida cheia" tem 10 caracteres
    for (size_t i = 0; i < com.texto.size(); i += 6) {
        if (i < sem.texto.size() && com.texto[i].posicao == sem.texto[i].posicao) continue;
        CHECK(com.texto[i].posicao.y >= 0);
        CHECK(com.texto[i].posicao.y + 8 <= l.painel.y);
    }
}

TEST_CASE("descrição do bacamarte mostra o bônus por chumbo") {
    jogo::Item b{jogo::TipoItem::Bacamarte, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Dano, 4}}};
    CHECK(render::descricaoItem(b).starts_with("Dano 4–6 ×7"));  // 2–4 + metade de 4
}
