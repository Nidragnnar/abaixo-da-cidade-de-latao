#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string

#include "render/Hud.h"
#include "render/Icones.h"
#include "render/Texto.h"

namespace {

// Largura do retângulo n (6 vértices cada, começando por a e (b.x, a.y))
float largura(const std::vector<render::VerticeCor>& v, size_t n) { return v[n * 6 + 1].posicao.x - v[n * 6].posicao.x; }

}

TEST_CASE("barra de vida pela metade") {
    render::EstadoHud e;
    e.vida = 15;
    e.vidaMaxima = 30;
    CHECK(largura(render::construirHud(e, 480, 270).cores, 2) == doctest::Approx(50.0f));
}

TEST_CASE("vida negativa não desenha barra para trás") {
    render::EstadoHud e;
    e.vida = -5;
    CHECK(largura(render::construirHud(e, 480, 270).cores, 2) == doctest::Approx(0.0f));
}

TEST_CASE("barra de XP proporcional") {
    render::EstadoHud e;
    e.xp = 10;
    e.xpProximo = 20;
    CHECK(largura(render::construirHud(e, 480, 270).cores, 4) == doctest::Approx(50.0f));
}

TEST_CASE("mira no centro da tela") {
    render::EstadoHud e;
    auto h = render::construirHud(e, 480, 270);
    glm::vec2 centroH = (h.cores[5 * 6].posicao + h.cores[5 * 6 + 2].posicao) / 2.0f;
    glm::vec2 centroV = (h.cores[6 * 6].posicao + h.cores[6 * 6 + 2].posicao) / 2.0f;
    CHECK(centroH == glm::vec2(240, 135));
    CHECK(centroV == glm::vec2(240, 135));
}

TEST_CASE("flash vermelho só logo depois do dano") {
    render::EstadoHud e;
    e.tempoDesdeDano = 0.05f;
    auto com = render::construirHud(e, 480, 270).cores.size();
    e.tempoDesdeDano = 1.0f;
    auto sem = render::construirHud(e, 480, 270).cores.size();
    CHECK(com == sem + 6);
}

TEST_CASE("texto da munição pelo tipo") {
    render::EstadoHud e;
    e.armaDeFogoAtiva = true;
    e.carga = 2;
    e.reserva = 8;
    e.municao = jogo::TipoMunicao::Cartuchos;
    CHECK(render::textoMunicao(e) == "2 | 8 cartuchos");
    e.semRecarga = true;
    e.reserva = 12;
    e.municao = jogo::TipoMunicao::Celulas;
    CHECK(render::textoMunicao(e) == "| 12 células");
    e.recarregando = true;
    CHECK(render::textoMunicao(e) == "RECARREGANDO");
    e.armaDeFogoAtiva = false;
    CHECK(render::textoMunicao(e).empty());
}

TEST_CASE("quadro da arma na mão por tipo") {
    CHECK(render::quadroArma(jogo::TipoItem::ChaveInglesa, false, false) == 0);
    CHECK(render::quadroArma(jogo::TipoItem::Revolver, true, false) == 3);
    CHECK(render::quadroArma(jogo::TipoItem::Revolver, false, true) == 4);
    CHECK(render::quadroArma(jogo::TipoItem::Martelo, true, false) == 8);
    CHECK(render::quadroArma(jogo::TipoItem::Bacamarte, false, true) == 11);
    CHECK(render::quadroArma(jogo::TipoItem::Tesla, true, false) == 13);
}

TEST_CASE("arma na mão usa o quadro do estado") {
    render::EstadoHud e;
    e.arma = jogo::TipoItem::Revolver;
    e.armaDeFogoAtiva = true;
    e.animacao = 0.1f;
    CHECK(render::construirHud(e, 480, 270).arma[0].camada == 3.0f);
}

TEST_CASE("itens rápidos com ícone aparecem") {
    render::EstadoHud e;
    auto sem = render::construirHud(e, 480, 270).icones.size();
    e.rapidos[0] = {render::iconeDoItem(jogo::TipoItem::Tonico), 2};
    auto com = render::construirHud(e, 480, 270);
    CHECK(com.icones.size() == sem + 6);
    CHECK(com.icones.back().camada == float(render::iconeDoItem(jogo::TipoItem::Tonico)));
}

TEST_CASE("ícones dos itens depois dos inimigos") {
    CHECK(render::iconeDoItem(jogo::TipoItem::ChaveInglesa) == render::PRIMEIRO_ITEM);
    CHECK(render::iconeDaMunicao(jogo::TipoMunicao::Balas) == render::PRIMEIRO_ITEM + 10);
    CHECK(render::ICONE_ENGRENAGENS == render::PRIMEIRO_ITEM + 13);
    CHECK(render::TOTAL_SPRITES == render::PRIMEIRO_ITEM + 14 + 4);  // + lampião, lustre, bomba e explosão
    CHECK(render::PRIMEIRO_ITEM == render::PRIMEIRO_INIMIGO + 9 * render::QUADROS_POR_INIMIGO);  // 6 inimigos e 3 chefes
}

TEST_CASE("tela de fim só fala do placar se entrou nele") {
    render::DadosFim d{3, 1, 12, 0, 560, 4242u, "Destruído por um Rato mecânico", 0};
    auto sem = render::construirTelaFim(d, 480, 270).texto.size();
    d.posicaoPlacar = 3;
    auto com = render::construirTelaFim(d, 480, 270).texto.size();
    CHECK(com == sem + render::construirTexto("3º lugar no placar!", {0, 0}, 1, glm::vec4(1)).size());
}

TEST_CASE("tela de fim mostra como continuar") {
    const auto h = render::construirTelaFim({3, 2, 10, 50, 560, 42u, "Destruído por um Rato", 0}, 480, 270);
    const std::string rodape = "Enter: nova partida   Esc: menu";
    const auto esperado = render::construirTexto(rodape, {0, 0}, 1, glm::vec4(1));
    REQUIRE(h.texto.size() >= esperado.size());
    const size_t inicio = h.texto.size() - esperado.size();
    for (size_t k = 0; k < esperado.size(); ++k) REQUIRE(h.texto[inicio + k].camada == esperado[k].camada);
    CHECK(render::larguraTexto(rodape, 1) <= 480);
}

namespace {
// O texto `s` aparece em sequência nos glifos de h
bool temTexto(const render::ElementosHud& h, const std::string& s) {
    const auto procurado = render::construirTexto(s, {0, 0}, 1, glm::vec4(1));
    if (procurado.empty() || h.texto.size() < procurado.size()) return false;
    for (size_t i = 0; i + procurado.size() <= h.texto.size(); ++i) {
        bool igual = true;
        for (size_t k = 0; k < procurado.size() && igual; ++k) igual = h.texto[i + k].camada == procurado[k].camada;
        if (igual) return true;
    }
    return false;
}
}

TEST_CASE("barra do chefe só durante a luta") {
    render::EstadoHud e;
    const auto sem = render::construirHud(e, 480, 270);
    CHECK_FALSE(temTexto(sem, "Rainha dos Ratos"));
    e.nomeChefe = "Rainha dos Ratos";
    e.vidaChefe = 70;
    e.vidaMaximaChefe = 140;
    const auto com = render::construirHud(e, 480, 270);
    CHECK(temTexto(com, "Rainha dos Ratos"));
    CHECK(com.cores.size() > sem.cores.size());
}

TEST_CASE("barra do chefe proporcional à vida") {
    render::EstadoHud e;
    e.nomeChefe = "Capataz a Vapor";
    e.vidaMaximaChefe = 240;
    auto direita = [&](int vida) {
        e.vidaChefe = vida;
        const auto h = render::construirHud(e, 480, 270);
        float maior = 0;
        for (const auto& v : h.cores)
            if (v.cor.r > 0.6f && v.cor.g < 0.3f && v.posicao.y < 40) maior = std::max(maior, v.posicao.x);  // vermelho, no alto
        return maior;
    };
    const float cheia = direita(240), metade = direita(120);
    CHECK(cheia > metade);
    CHECK(metade > 0);
}

TEST_CASE("tela de vitória diz Vitória! e o tempo m:ss") {
    render::DadosFim d{6, 9, 80, 300, 4000, 42u, "Venceu a Grande Máquina", 0};
    d.vitoria = true;
    d.segundos = 185;
    const auto h = render::construirTelaFim(d, 480, 270);
    CHECK(temTexto(h, "Vitória!"));
    CHECK(temTexto(h, "Tempo 3:05"));
    CHECK_FALSE(temTexto(h, "Você morreu"));
}
