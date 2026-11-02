#include <doctest/doctest.h>

#include "render/Hud.h"
#include "render/Texto.h"

TEST_CASE("barra de vida pela metade") {
    render::EstadoHud e{15, 30, jogo::ArmaAtiva::ChaveInglesa, 6, 12, false, 0, 9.0f, "", 1, 0};
    auto h = render::construirHud(e, 480, 270);
    // segundo retângulo = preenchimento
    float largura = h.cores[6 + 1].posicao.x - h.cores[6].posicao.x;
    CHECK(largura == doctest::Approx(50.0f));
}

TEST_CASE("vida negativa não desenha barra para trás") {
    render::EstadoHud e{-5, 30, jogo::ArmaAtiva::ChaveInglesa, 6, 12, false, 0, 9.0f, "", 1, 0};
    auto h = render::construirHud(e, 480, 270);
    CHECK(h.cores[6 + 1].posicao.x - h.cores[6].posicao.x == doctest::Approx(0.0f));
}

TEST_CASE("flash vermelho só logo depois do dano") {
    render::EstadoHud e{30, 30, jogo::ArmaAtiva::ChaveInglesa, 6, 12, false, 0, 0.05f, "", 1, 0};
    auto com = render::construirHud(e, 480, 270).cores.size();
    e.tempoDesdeDano = 1.0f;
    auto sem = render::construirHud(e, 480, 270).cores.size();
    CHECK(com == sem + 6);
}

TEST_CASE("arma na mão mostra o quadro do estado") {
    render::EstadoHud e{30, 30, jogo::ArmaAtiva::Revolver, 6, 12, false, 0.1f, 9.0f, "", 1, 0};
    CHECK(render::construirHud(e, 480, 270).arma[0].camada == 3.0f);  // revólver tiro
    e.recarregando = true;
    e.animacao = 0;
    CHECK(render::construirHud(e, 480, 270).arma[0].camada == 4.0f);
    e.arma = jogo::ArmaAtiva::ChaveInglesa;
    e.recarregando = false;
    CHECK(render::construirHud(e, 480, 270).arma[0].camada == 0.0f);
}

TEST_CASE("munição só aparece com o revólver") {
    render::EstadoHud e{30, 30, jogo::ArmaAtiva::ChaveInglesa, 6, 12, false, 0, 9.0f, "", 1, 0};
    auto chave = render::construirHud(e, 480, 270).texto.size();
    e.arma = jogo::ArmaAtiva::Revolver;
    auto revolver = render::construirHud(e, 480, 270).texto.size();
    CHECK(revolver == chave + render::construirTexto("6 | 12", {0, 0}, 1, glm::vec4(1)).size());
}

TEST_CASE("tela de fim só fala do placar se entrou nele") {
    render::DadosFim d{3, 1, 12, 0, 560, 4242u, "Destruído por um Rato mecânico", 0};
    auto sem = render::construirTelaFim(d, 480, 270).texto.size();
    d.posicaoPlacar = 3;
    auto com = render::construirTelaFim(d, 480, 270).texto.size();
    CHECK(com == sem + render::construirTexto("3º lugar no placar!", {0, 0}, 1, glm::vec4(1)).size());
}

TEST_CASE("mira no centro da tela") {
    render::EstadoHud e{30, 30, jogo::ArmaAtiva::Revolver, 6, 12, false, 0, 9.0f, "", 1, 0};
    auto h = render::construirHud(e, 480, 270);
    // dois traços (horizontal e vertical) logo depois da barra de vida, centrados em (240, 135)
    REQUIRE(h.cores.size() >= 24);
    glm::vec2 centroH = (h.cores[12].posicao + h.cores[14].posicao) / 2.0f;
    glm::vec2 centroV = (h.cores[18].posicao + h.cores[20].posicao) / 2.0f;
    CHECK(centroH == glm::vec2(240, 135));
    CHECK(centroV == glm::vec2(240, 135));
}
