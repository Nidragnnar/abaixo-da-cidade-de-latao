#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>
#include <string>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "jogo/Chefes.h"
#include "jogo/IA.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

namespace {

struct Resultado {
    jogo::EventosIA total;  // golpes, tiros, invocações e ondas somados
};

// Roda a IA como a Partida: campos a cada 0,5 s, contagem de ratos e voadores vivos, invocações viram inimigos
Resultado simularChefe(std::vector<jogo::Inimigo>& v, mundo::Grade& g, glm::vec2 jogador, float segundos,
                       std::optional<mundo::Sala> arena = {},
                       const std::function<void(int, std::vector<jogo::Inimigo>&)>& cada = {}) {
    std::mt19937 rng(3);
    Resultado r;
    std::vector<int> campo, voador;
    float desdeCampo = jogo::RECALCULO_CAMINHO;
    for (int passo = 0; passo < static_cast<int>(std::lround(segundos * 60)); ++passo) {
        if (desdeCampo >= jogo::RECALCULO_CAMINHO) {
            campo = mundo::distanciasComAltura(g, glm::ivec2(jogador), mundo::ANDARILHO);
            voador = mundo::distanciasComAltura(g, glm::ivec2(jogador), mundo::VOADOR);
            desdeCampo = 0;
        }
        jogo::MundoIA m{g, jogador, campo, rng};
        m.campoSaltador = &campo;
        m.campoVoador = &voador;
        m.arena = arena;
        for (const auto& i : v) {
            if (i.estado == jogo::EstadoInimigo::Destruido) continue;
            m.ratosVivos += i.tipo == jogo::TipoInimigo::Rato;
            m.voadoresVivos += jogo::dados(i.tipo).voa;
        }
        auto e = jogo::atualizarInimigos(v, m, 1.0f / 60);
        for (const auto& c : e.invocacoes) v.push_back(jogo::criarInimigo(c.tipo, c.posicao, 2));
        r.total.golpes.insert(r.total.golpes.end(), e.golpes.begin(), e.golpes.end());
        r.total.tiros.insert(r.total.tiros.end(), e.tiros.begin(), e.tiros.end());
        r.total.invocacoes.insert(r.total.invocacoes.end(), e.invocacoes.begin(), e.invocacoes.end());
        r.total.ondas.insert(r.total.ondas.end(), e.ondas.begin(), e.ondas.end());
        if (cada) cada(passo, v);
        desdeCampo += 1.0f / 60;
    }
    return r;
}

int golpesDe(const jogo::EventosIA& e, jogo::TipoInimigo t) {
    int n = 0;
    for (const auto& g : e.golpes) n += g.autor == t;
    return n;
}

// Sala aberta 12 × 3 (x 1–12, y 1–3)
mundo::Andar salao(const std::string& meio) {
    return mundo::andarDeTexto({"##############", "#............#", meio, "#............#", "##############"}, 2);
}

}

TEST_CASE("investida em linha reta e dano ao encostar") {
    auto a = salao("#@........R..#");
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].recarga = jogo::INVESTIDA_INTERVALO;  // pronta
    float desvio = 0;
    const auto r = simularChefe(v, a.grade, a.inicio, 4.0f, {}, [&](int, auto& vs) {
        if (vs[0].estado == jogo::EstadoInimigo::Preparando && vs[0].tempoNoEstado > jogo::INVESTIDA_MIRA)
            desvio = std::max(desvio, std::abs(vs[0].posicao.y - 2.5f));
    });
    REQUIRE(golpesDe(r.total, jogo::TipoInimigo::RainhaRatos) == 1);
    for (const auto& g : r.total.golpes)
        if (g.autor == jogo::TipoInimigo::RainhaRatos) {
            CHECK(g.dano >= 7);
            CHECK(g.dano <= 10);
        }
    CHECK(desvio < 0.05f);
}

TEST_CASE("tonta ao bater na parede") {
    auto a = salao("#@.....R.....#");
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Preparando;  // mirando no jogador, a oeste
    const auto mira = simularChefe(v, a.grade, a.inicio, jogo::INVESTIDA_MIRA + 0.05f);
    REQUIRE(v[0].estado == jogo::EstadoInimigo::Preparando);
    // O jogador sai da frente: ela segue reto até a parede oeste
    const glm::vec2 longe{12.5f, 1.5f};
    bool tonta = false;
    const auto r = simularChefe(v, a.grade, longe, 2.0f, {}, [&](int, auto& vs) {
        tonta |= vs[0].estado == jogo::EstadoInimigo::Recuperando;
    });
    CHECK(tonta);
    CHECK(golpesDe(r.total, jogo::TipoInimigo::RainhaRatos) == 0);
    CHECK(v[0].posicao.x < 2.0f);
    simularChefe(v, a.grade, longe, jogo::TONTURA);
    CHECK(v[0].estado != jogo::EstadoInimigo::Recuperando);
}

TEST_CASE("ninhada de 3 a cada 8 s e nunca mais de 6 ratos") {
    // Jogador atrás de parede: sem investida, só a ninhada
    auto a = mundo::andarDeTexto({"##########", "#@#......#", "#.#..R...#", "#.#......#", "##########"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].recarga2 = jogo::NINHADA_INTERVALO;
    const mundo::Sala arena{3, 1, 6, 3};
    CHECK(simularChefe(v, a.grade, a.inicio, 0.1f, arena).total.invocacoes.size() == 3);
    CHECK(simularChefe(v, a.grade, a.inicio, jogo::NINHADA_INTERVALO).total.invocacoes.size() == 3);
    CHECK(simularChefe(v, a.grade, a.inicio, jogo::NINHADA_INTERVALO * 2).total.invocacoes.empty());  // 6 vivos
}

TEST_CASE("ratos chamados nascem no chão da arena") {
    auto a = mundo::andarDeTexto({"##########", "#@#......#", "#.#.....R#", "#.#......#", "##########"}, 2);
    auto v = jogo::inimigosDasMarcas(a);  // Rainha no canto: parte dos vizinhos é parede
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].recarga2 = jogo::NINHADA_INTERVALO;
    const mundo::Sala arena{3, 1, 6, 3};
    const auto r = simularChefe(v, a.grade, a.inicio, 0.1f, arena);
    REQUIRE(r.total.invocacoes.size() == 3);
    for (const auto& c : r.total.invocacoes) {
        const int x = static_cast<int>(std::floor(c.posicao.x)), y = static_cast<int>(std::floor(c.posicao.y));
        CHECK(a.grade.em(x, y) == mundo::Celula::Chao);
        CHECK(arena.contem(x, y));
    }
}

TEST_CASE("investida não atravessa nem trava na comporta") {
    auto a = salao("#@........R..#");
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Preparando;
    simularChefe(v, a.grade, a.inicio, jogo::INVESTIDA_MIRA + 0.05f);
    // A comporta tranca no caminho (coluna 6, as três linhas)
    for (int y = 1; y <= 3; ++y) a.grade.definir(6, y, mundo::Celula::ComportaTrancada);
    simularChefe(v, a.grade, a.inicio, 3.0f);
    CHECK(v[0].posicao.x >= 7.0f + jogo::dados(jogo::TipoInimigo::RainhaRatos).raio - 0.01f);
    CHECK(v[0].estado != jogo::EstadoInimigo::Recuperando);
}

TEST_CASE("leque de 5 projéteis nos ângulos certos") {
    auto a = salao("#@....P......#");
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].recarga = jogo::JATO_INTERVALO;
    glm::vec2 deOnde{0.0f};
    const auto r = simularChefe(v, a.grade, a.inicio, 1.0f, {}, [&](int, auto& vs) { deOnde = vs[0].posicao; });
    REQUIRE(r.total.tiros.size() == 5);
    const glm::vec2 base = glm::normalize(a.inicio - deOnde);
    std::vector<float> angulos;
    for (const auto& t : r.total.tiros) {
        const glm::vec2 h = glm::normalize(glm::vec2(t.velocidade.x, t.velocidade.z));
        angulos.push_back(glm::degrees(std::atan2(base.x * h.y - base.y * h.x, glm::dot(base, h))));
        CHECK(t.dano >= 4);
        CHECK(t.dano <= 6);
        CHECK(t.autor == jogo::TipoInimigo::Capataz);
        CHECK(t.raioExplosao == 0.0f);
    }
    std::sort(angulos.begin(), angulos.end());
    for (size_t k = 0; k < 5; ++k) CHECK(angulos[k] == doctest::Approx(jogo::JATO_ANGULOS[k]).epsilon(0.02));
}

TEST_CASE("pancada solta uma onda depois do preparo") {
    auto a = salao("#@.P.........#");
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].recarga2 = jogo::PANCADA_INTERVALO;
    CHECK(simularChefe(v, a.grade, a.inicio, jogo::PANCADA_PREPARO - 0.1f).total.ondas.empty());
    const auto r = simularChefe(v, a.grade, a.inicio, 0.3f);
    REQUIRE(r.total.ondas.size() == 1);
    CHECK(r.total.ondas[0].raio == doctest::Approx(jogo::PANCADA_ALCANCE));
    CHECK(r.total.ondas[0].dano >= jogo::PANCADA_DANO_MIN);
    CHECK(r.total.ondas[0].dano <= jogo::PANCADA_DANO_MAX);
}

TEST_CASE("onda fere no chão, não no ar nem fora do raio") {
    const jogo::Onda o{{5.0f, 5.0f}, 2.5f, 10, jogo::TipoInimigo::Capataz};
    CHECK(jogo::ondaAtinge(o, {6.0f, 5.0f}, true));
    CHECK_FALSE(jogo::ondaAtinge(o, {6.0f, 5.0f}, false));  // pulando
    CHECK_FALSE(jogo::ondaAtinge(o, {7.6f, 5.0f}, true));   // longe
}

namespace {
size_t voadoresVivos(const std::vector<jogo::Inimigo>& v) {
    size_t n = 0;
    for (const auto& i : v) n += jogo::dados(i.tipo).voa && i.estado != jogo::EstadoInimigo::Destruido;
    return n;
}
}

TEST_CASE("Grande Máquina não sai do lugar nem com empurrão") {
    auto a = salao("#@...M.......#");
    auto v = jogo::inimigosDasMarcas(a);
    const glm::vec2 antes = v[0].posicao;
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].empurrao = {0.6f, 0.0f};
    simularChefe(v, a.grade, {4.0f, 2.5f}, 2.0f);  // jogador colado nela
    CHECK(v[0].posicao == antes);
}

TEST_CASE("rajada de 3 a cada 2 s") {
    auto a = salao("#@...M.......#");
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].recarga = jogo::RAJADA_INTERVALO;
    CHECK(simularChefe(v, a.grade, a.inicio, 0.5f).total.tiros.size() == 3);
    CHECK(simularChefe(v, a.grade, a.inicio, 1.4f).total.tiros.empty());
    CHECK(simularChefe(v, a.grade, a.inicio, 1.0f).total.tiros.size() == 3);
}

TEST_CASE("fase 2 com metade da vida: rajada a cada 1,2 s") {
    auto a = salao("#@...M.......#");
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].vida = 210;  // metade de 420
    v[0].recarga = jogo::RAJADA_INTERVALO_FASE2;
    v[0].recarga2 = -100.0f;  // sem chamado neste teste
    CHECK(simularChefe(v, a.grade, a.inicio, 2.2f).total.tiros.size() == 6);
}

TEST_CASE("chama 2 vespas e depois 1 dirigível, nunca mais de 4 voadores") {
    auto a = mundo::andarDeTexto({"##########", "#@#......#", "#.#..M...#", "#.#......#", "##########"}, 6, {},
                                 {"0000000000", "0gggggggg0", "0gggggggg0", "0gggggggg0", "0000000000"});
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].vida = 200;
    v[0].recarga2 = jogo::CHAMADO_INTERVALO;
    const auto primeiro = simularChefe(v, a.grade, a.inicio, 0.1f);
    REQUIRE(primeiro.total.invocacoes.size() == 2);
    CHECK(primeiro.total.invocacoes[0].tipo == jogo::TipoInimigo::Vespa);
    const auto segundo = simularChefe(v, a.grade, a.inicio, jogo::CHAMADO_INTERVALO);
    REQUIRE(segundo.total.invocacoes.size() == 1);
    CHECK(segundo.total.invocacoes[0].tipo == jogo::TipoInimigo::Dirigivel);
    simularChefe(v, a.grade, a.inicio, jogo::CHAMADO_INTERVALO * 5, {}, [&](int, auto& vs) {
        REQUIRE(voadoresVivos(vs) <= static_cast<size_t>(jogo::MAXIMO_VOADORES));
    });
    CHECK(voadoresVivos(v) == static_cast<size_t>(jogo::MAXIMO_VOADORES));
}

TEST_CASE("sem fase 2 não chama ninguém") {
    auto a = salao("#@...M.......#");
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].recarga2 = jogo::CHAMADO_INTERVALO;
    CHECK(simularChefe(v, a.grade, a.inicio, 1.0f).total.invocacoes.empty());
}

TEST_CASE("chefe não acorda antes da luta, nem por barulho") {
    auto a = salao("#@...P.......#");
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    CHECK(v[0].estado == jogo::EstadoInimigo::Parado);
    std::mt19937 rng(1);
    const auto campo = mundo::distanciasComAltura(a.grade, glm::ivec2(a.inicio), mundo::ANDARILHO);
    jogo::MundoIA m{a.grade, a.inicio, campo, rng};
    jogo::atualizarInimigos(v, m, 1.0f / 60);  // vê o jogador, mas a luta não começou
    CHECK(v[0].estado == jogo::EstadoInimigo::Parado);
    m.lutaComChefe = true;
    jogo::atualizarInimigos(v, m, 1.0f / 60);
    CHECK(v[0].estado == jogo::EstadoInimigo::Perseguindo);
}
