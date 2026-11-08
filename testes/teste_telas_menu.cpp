#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string
#include <string>

#include "render/Letterbox.h"
#include "render/TelasMenu.h"
#include "render/Texto.h"
#include "ui/Menus.h"

namespace {
glm::vec2 centro(render::Retangulo r) { return {r.x + r.largura / 2.0f, r.y + r.altura / 2.0f}; }

// As camadas (glifos) do texto montado contêm as do trecho, em sequência
bool contemTexto(const std::vector<render::VerticeTex>& v, std::string_view trecho) {
    const auto t = render::construirTexto(trecho, {0, 0}, 1, glm::vec4(1));
    if (t.empty() || v.size() < t.size()) return false;
    for (size_t i = 0; i + t.size() <= v.size(); i += 6) {
        bool igual = true;
        for (size_t k = 0; k < t.size() && igual; k += 6) igual = v[i + k].camada == t[k].camada;
        if (igual) return true;
    }
    return false;
}
}

TEST_CASE("título do jogo cabe na tela baixa") {
    CHECK(render::larguraTexto("Abaixo da Cidade de Latão", 2) <= 480 - 16);
}

TEST_CASE("itens do menu centralizados e sem sobreposição") {
    const auto l = render::layoutMenu(6, 480, 270);
    REQUIRE(l.size() == 6);
    for (size_t i = 0; i < l.size(); ++i) {
        CHECK(l[i].x >= 0);
        CHECK(l[i].y >= 0);
        CHECK(l[i].x + l[i].largura <= 480);
        CHECK(l[i].y + l[i].altura <= 270);
        CHECK(centro(l[i]).x == doctest::Approx(240));
        if (i > 0) CHECK(l[i].y >= l[i - 1].y + l[i - 1].altura);
    }
}

TEST_CASE("item sob o mouse") {
    const auto l = render::layoutMenu(4, 480, 270);
    CHECK(render::itemSob(l, centro(l[2])) == 2);
    CHECK_FALSE(render::itemSob(l, {0, 0}).has_value());
    // Janela 1920×1080 com escala de tela de 125% (desenhável 2400×1350): o centro do item 1 em pixels da janela
    const glm::vec2 alvo = centro(l[1]);
    const glm::vec2 mouse = alvo * (2400.0f / 480.0f) / 1.25f;
    const glm::vec2 naTela = render::janelaParaTelaBaixa(mouse, {1920, 1080}, {2400, 1350}, 480, 270);
    CHECK(render::itemSob(l, naTela) == 1);
}

TEST_CASE("menu destaca o selecionado") {
    const render::VistaMenu v{"Pausa", {"Continuar", "Opções", "Abandonar partida"}, 1};
    const auto e = render::construirMenu(v, 480, 270);
    const auto l = render::layoutMenu(3, 480, 270);
    // A faixa de destaque é o último retângulo de cor e cobre o item 1
    REQUIRE(e.cores.size() >= 12);
    const auto& a = e.cores[e.cores.size() - 6];
    CHECK(a.posicao.x == doctest::Approx(l[1].x));
    CHECK(a.posicao.y == doctest::Approx(l[1].y));
    CHECK(contemTexto(e.texto, "Opções"));
    CHECK(contemTexto(e.texto, "Abandonar partida"));
}

TEST_CASE("menu principal: título inteiro dentro da tela") {
    const render::VistaMenu v{"Abaixo da Cidade de Latão", {"Novo jogo", "Placar", "Opções", "Sair"}, 0};
    const auto e = render::construirMenu(v, 480, 270);
    for (const auto& p : e.texto) {
        REQUIRE(p.posicao.x >= 0);
        REQUIRE(p.posicao.x <= 480);
    }
    for (const auto& p : e.cores) {
        REQUIRE(p.posicao.x >= 0);
        REQUIRE(p.posicao.x <= 480);
    }
}

TEST_CASE("placar: textos não saem da tela, causa longa é cortada") {
    std::vector<jogo::RegistroPlacar> r;
    for (int i = 0; i < 10; ++i) r.push_back({99999, 12, 9, 50, 999, 1u, std::string(80, 'x')});
    const auto e = render::construirPlacar(r, 480, 270);
    for (const auto& p : e.texto) {
        REQUIRE(p.posicao.x <= 480);
        REQUIRE(p.posicao.y <= 270);
    }
    CHECK(contemTexto(e.texto, "..."));
    const auto voltar = render::layoutPlacar(480, 270);
    REQUIRE(voltar.size() == 1);
    CHECK(voltar[0].y + voltar[0].altura <= 270);
}

TEST_CASE("placar vazio avisa") {
    CHECK(contemTexto(render::construirPlacar({}, 480, 270).texto, "Nenhuma partida ainda"));
    const auto um = render::construirPlacar({{1234, 5, 3, 7, 10, 1u, "Destruído por um Colosso"}}, 480, 270);
    CHECK(contemTexto(um.texto, "1. 1234"));
    CHECK(contemTexto(um.texto, "Destruído por um Colosso"));
}

TEST_CASE("placar: corte da causa não come o acento antes do ponto de corte") {
    // "1. 1 N  andar 1  nível 1  " tem 26 caracteres; cabem 56 (448 px), então sobram 27 da causa antes do "..."
    std::vector<jogo::RegistroPlacar> r{{1, 1, 1, 0, 0, 1u, std::string(26, 'x') + "ãaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}};
    const auto e = render::construirPlacar(r, 480, 270);
    CHECK(contemTexto(e.texto, "..."));
    CHECK(contemTexto(e.texto, "xã..."));  // o "ã" fica; sai só o que vem depois dele
    // Sem glifo '?' (o que aparece quando o UTF-8 fica partido)
    const auto interrogacao = render::construirTexto("?", {0, 0}, 1, glm::vec4(1));
    for (size_t i = 0; i < e.texto.size(); i += 6) CHECK(e.texto[i].camada != interrogacao[0].camada);
    // A linha é a mais longa que cabe: com mais um caractere passaria de 448 px
    float direita = 0;
    for (const auto& v : e.texto)
        if (v.posicao.y > 40 && v.posicao.y < 60) direita = std::max(direita, v.posicao.x);
    CHECK(direita > 448 - 16);
}

TEST_CASE("menu mostra o aviso embaixo dos itens") {
    render::VistaMenu v{"Abaixo da Cidade de Latão", {"Novo jogo", "Placar", "Opções", "Sair"}, 0};
    CHECK_FALSE(contemTexto(render::construirMenu(v, 480, 270).texto, "Partida salva não pôde ser lida"));
    v.aviso = "Partida salva não pôde ser lida";
    const auto e = render::construirMenu(v, 480, 270);
    CHECK(contemTexto(e.texto, "Partida salva não pôde ser lida"));
    const auto& a = e.cores[e.cores.size() - 6];  // o destaque continua sendo o último retângulo
    CHECK(a.posicao.y == doctest::Approx(render::layoutMenu(4, 480, 270)[0].y));
}

TEST_CASE("tela do placar mostra a letra") {
    const auto e = render::construirPlacar({{1234, 5, 3, 7, 10, 1u, "Morto", jogo::Dificuldade::Dificil}}, 480, 270);
    CHECK(contemTexto(e.texto, "1. 1234 D  andar 5"));
}

TEST_CASE("listas longas cabem na tela") {
    for (int n : {11, 18}) {
        INFO("itens: " << n);
        const auto r = render::layoutMenu(n, 480, 270);
        REQUIRE(r.size() == static_cast<size_t>(n));
        CHECK(r.front().y > 48 + 16);  // abaixo do título
        CHECK(r.back().y + r.back().altura <= 270 - 4);
        for (size_t i = 1; i < r.size(); ++i) CHECK(r[i].y >= r[i - 1].y + r[i - 1].altura);
        // o aviso ainda cabe embaixo
        render::VistaMenu v{"Opções", std::vector<std::string>(static_cast<size_t>(n), "x"), 0, "Esc cancela"};
        for (const auto& p : render::construirMenu(v, 480, 270).texto) CHECK(p.posicao.y <= 270);
    }
    // até 10 itens, a posição é a de hoje
    const auto dez = render::layoutMenu(10, 480, 270);
    CHECK(dez[0].y == 96 - 2);
    CHECK(dez[9].y == 96 + 9 * 14 - 2);
}

TEST_CASE("opções da pausa cabem no painel") {
    ui::Menus m;
    jogo::Config c;
    c.dificuldade = jogo::Dificuldade::Pesadelo;
    c.semente = 4294967295u;
    c.sensibilidadeControle = 10;
    m.abrir(ui::Tela::Pausa);
    m.atualizar(ui::EntradaMenu{.mover = 2}, c);
    m.atualizar(ui::EntradaMenu{.confirmar = true}, c);
    REQUIRE(m.tela() == ui::Tela::Opcoes);
    for (const auto& t : m.itens(c)) CHECK(render::larguraTexto(t, 1) <= 264 + 40);
}

TEST_CASE("tela Teclas cabe no painel, esperando ou não") {
    ui::Menus m;
    jogo::Config c;
    c.teclas.definir(jogo::AcaoJogo::Frente, 81);  // "Seta para baixo", um dos nomes mais longos
    m.abrir(ui::Tela::Opcoes);
    m.atualizar(ui::EntradaMenu{.mover = 8}, c);
    m.atualizar(ui::EntradaMenu{.confirmar = true}, c);
    REQUIRE(m.tela() == ui::Tela::Teclas);
    for (const auto& t : m.itens(c)) CHECK(render::larguraTexto(t, 1) <= 264 + 40);
    m.atualizar(ui::EntradaMenu{.confirmar = true}, c);
    REQUIRE(m.esperandoTecla());
    for (const auto& t : m.itens(c)) CHECK(render::larguraTexto(t, 1) <= 264 + 40);
    CHECK(render::larguraTexto(m.aviso(), 1) <= 480 - 16);
}
