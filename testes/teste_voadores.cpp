#include <doctest/doctest.h>

#include <cmath>
#include <functional>
#include <random>
#include <string>

#include <glm/geometric.hpp>

#include "jogo/Armas.h"
#include "jogo/IA.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

namespace {

// Roda a IA com os três campos (andarilho, saltador e voador) e os pés do jogador; `cada` é chamado a cada passo
jogo::EventosIA simularVoo(std::vector<jogo::Inimigo>& v, mundo::Grade& g, glm::vec2 jogador, float pes, float segundos,
                           const std::function<void(const std::vector<jogo::Inimigo>&)>& cada = {},
                           bool abrirComportas = true) {
    std::mt19937 rng(3);
    jogo::EventosIA total;
    std::vector<int> andarilho, saltador, voador;
    float desdeCampo = jogo::RECALCULO_CAMINHO;
    for (int i = 0; i < static_cast<int>(std::lround(segundos * 60)); ++i) {
        if (desdeCampo >= jogo::RECALCULO_CAMINHO) {
            andarilho = mundo::distanciasComAltura(g, glm::ivec2(jogador), mundo::ANDARILHO);
            saltador = mundo::distanciasComAltura(g, glm::ivec2(jogador), mundo::SALTADOR);
            voador = mundo::distanciasComAltura(g, glm::ivec2(jogador), mundo::VOADOR);
            desdeCampo = 0;
        }
        jogo::MundoIA m{g, jogador, andarilho, rng};
        m.campoSaltador = &saltador;
        m.campoVoador = &voador;
        m.pesJogador = pes;
        auto e = jogo::atualizarInimigos(v, m, 1.0f / 60);
        if (abrirComportas)
            for (auto c : e.comportasParaAbrir) g.definir(c.x, c.y, mundo::Celula::ComportaAberta);
        total.golpes.insert(total.golpes.end(), e.golpes.begin(), e.golpes.end());
        total.tiros.insert(total.tiros.end(), e.tiros.begin(), e.tiros.end());
        if (cada) cada(v);
        desdeCampo += 1.0f / 60;
    }
    return total;
}

// Confere, para cada voador vivo, que o corpo inteiro está entre o chão e o teto sob o centro
void conferirAltura(const std::vector<jogo::Inimigo>& v, const mundo::Grade& g) {
    for (const auto& i : v) {
        if (!jogo::dados(i.tipo).voa || i.estado == jogo::EstadoInimigo::Destruido) continue;
        REQUIRE(i.corpo.pes >= mundo::alturaDoChao(g, i.posicao) - 1e-3f);
        REQUIRE(i.corpo.pes + jogo::dados(i.tipo).altura <= mundo::alturaDoTeto(g, i.posicao) + 1e-3f);
    }
}

// Sala alta (teto 4 blocos acima do chão): poço de 1 bloco no meio e uma caixa antes da vespa
mundo::Andar salaComPoco() {
    return mundo::andarDeTexto({"##########", "#@......v#", "##########"}, 2, {"0000000000", "0440004840", "0000000000"},
                               {"0000000000", "0gggggggg0", "0000000000"});
}

}

TEST_CASE("voador atravessa poço e caixa") {
    auto a = salaComPoco();
    auto v = jogo::inimigosDasMarcas(a);
    REQUIRE(v[0].tipo == jogo::TipoInimigo::Vespa);
    const auto e = simularVoo(v, a.grade, a.inicio, 1.0f, 6.0f, [&](const auto& vs) { conferirAltura(vs, a.grade); });
    CHECK_FALSE(e.golpes.empty());
    // O autômato no mesmo lugar não passa do poço
    auto b = mundo::andarDeTexto({"##########", "#@......a#", "##########"}, 2, {"0000000000", "0440004840", "0000000000"},
                                 {"0000000000", "0gggggggg0", "0000000000"});
    auto chao = jogo::inimigosDasMarcas(b);
    CHECK(simularVoo(chao, b.grade, b.inicio, 1.0f, 6.0f).golpes.empty());
}

TEST_CASE("parede barra o voador no campo") {
    auto a = mundo::andarDeTexto({"#####", "#@#.#", "#####"}, 2);
    const auto campo = mundo::distanciasComAltura(a.grade, glm::ivec2(a.inicio), mundo::VOADOR);
    CHECK(campo[1 * 5 + 3] == -1);
}

TEST_CASE("comporta fechada segura o voador até abrir") {
    auto a = mundo::andarDeTexto({"#########", "#@..C..v#", "#########"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    const auto e = simularVoo(v, a.grade, a.inicio, 0.0f, 3.0f, {}, false);
    CHECK(v[0].posicao.x > 4.9f);
    CHECK(e.comportasParaAbrir.empty());  // simularVoo não junta pedidos; o que vale é não ter atravessado
}

TEST_CASE("altura do voador entre chão e teto") {
    // Vespa e dirigível juntos sobre poço, caixa e degraus, num teto que muda de altura
    auto a = mundo::andarDeTexto({"##########", "#@..d...v#", "##########"}, 3, {"0000000000", "0440004840", "0000000000"},
                                 {"0000000000", "0gg99ggkk0", "0000000000"});
    auto v = jogo::inimigosDasMarcas(a);
    simularVoo(v, a.grade, a.inicio, 1.0f, 8.0f, [&](const auto& vs) { conferirAltura(vs, a.grade); });
}

TEST_CASE("voador passa por corredor baixo") {
    // Teto 1,25 acima do chão (5 quartos) em todo o corredor
    auto a = mundo::andarDeTexto({"################", "#@............v#", "################"}, 3, {},
                                 {"0000000000000000", "0555555555555550", "0000000000000000"});
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    const auto e = simularVoo(v, a.grade, a.inicio, 0.0f, 8.0f, [&](const auto& vs) { conferirAltura(vs, a.grade); });
    CHECK_FALSE(e.golpes.empty());
}

TEST_CASE("voador parado no ar não oscila") {
    // Jogador do outro lado da parede: sem caminho, a vespa fica no lugar
    auto a = mundo::andarDeTexto({"#######", "#@#..v#", "#######"}, 2, {}, {"0000000", "0ggggg0", "0000000"});
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simularVoo(v, a.grade, a.inicio, 0.0f, 2.0f);
    const float antes = v[0].corpo.pes;
    CHECK(antes > 1.0f);  // subiu do chão para a altura de voo
    simularVoo(v, a.grade, a.inicio, 0.0f, 10.0f / 60, [&](const auto& vs) {
        CHECK(std::abs(vs[0].corpo.pes - antes) <= 0.05f);
    });
}

TEST_CASE("vespa mergulha, morde e sobe") {
    auto a = mundo::andarDeTexto({"#######", "#@....#", "#######"}, 2, {}, {"0000000", "0ggggg0", "0000000"});
    auto v = std::vector{jogo::criarInimigo(jogo::TipoInimigo::Vespa, {2.9f, 1.5f}, 2)};
    v[0].corpo.pes = jogo::ALTURA_VOO_VESPA;
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    jogo::EventosIA e;
    float pesNaMordida = -1;
    for (int i = 0; i < 60 && v[0].estado != jogo::EstadoInimigo::Recuperando; ++i) {
        auto passo = simularVoo(v, a.grade, a.inicio, 0.0f, 1.0f / 60);
        e.golpes.insert(e.golpes.end(), passo.golpes.begin(), passo.golpes.end());
        pesNaMordida = v[0].corpo.pes;
    }
    REQUIRE(v[0].estado == jogo::EstadoInimigo::Recuperando);
    CHECK(pesNaMordida == doctest::Approx(jogo::ALTURA_PEITO).epsilon(0.2));
    REQUIRE(e.golpes.size() == 1);
    CHECK(e.golpes[0].dano >= 2);
    CHECK(e.golpes[0].dano <= 5);  // 2–4 escalado para o andar 2
    simularVoo(v, a.grade, a.inicio, 0.0f, 0.75f);
    CHECK(v[0].corpo.pes > jogo::ALTURA_VOO_VESPA - 0.1f);
}

TEST_CASE("chave inglesa só acerta a vespa embaixo") {
    auto a = mundo::andarDeTexto({"#######", "#@v...#", "#######"}, 2, {}, {"0000000", "0ggggg0", "0000000"});
    jogo::Inventario inv;
    std::mt19937 rng(1);
    auto alta = jogo::inimigosDasMarcas(a);
    alta[0].corpo.pes = jogo::ALTURA_VOO_VESPA;
    jogo::Armas branca;
    jogo::Comandos atacar;
    atacar.atacar = true;
    auto r = jogo::atualizarArmas(branca, inv, jogo::calcularAtributos(inv, 1), atacar, a.inicio, 0.0f, 0.0f, 0.0f,
                                  a.grade, alta, rng, 1.0f / 60);
    CHECK(r.acertos == 0);
    auto baixa = jogo::inimigosDasMarcas(a);
    baixa[0].corpo.pes = jogo::ALTURA_PEITO;
    jogo::Armas branca2;
    r = jogo::atualizarArmas(branca2, inv, jogo::calcularAtributos(inv, 1), atacar, a.inicio, 0.0f, 0.0f, 0.0f, a.grade,
                             baixa, rng, 1.0f / 60);
    CHECK(r.acertos == 1);
}

TEST_CASE("tiro acerta a vespa no alto") {
    auto a = mundo::andarDeTexto({"#######", "#@...v#", "#######"}, 2, {}, {"0000000", "0ggggg0", "0000000"});
    auto v = jogo::inimigosDasMarcas(a);
    v[0].corpo.pes = jogo::ALTURA_VOO_VESPA;
    jogo::Inventario inv;
    jogo::Armas fogo;
    fogo.ativa = jogo::ArmaAtiva::Fogo;
    std::mt19937 rng(1);
    jogo::Comandos atacar;
    atacar.atacar = true;
    // Do olho (0,5) ao centro da vespa (1,35), 4 células à frente
    const float inclinacao = std::atan((jogo::ALTURA_VOO_VESPA + 0.15f - 0.5f) / 4.0f);
    const auto r = jogo::atualizarArmas(fogo, inv, jogo::calcularAtributos(inv, 1), atacar, a.inicio, 0.0f, 0.0f,
                                        inclinacao, a.grade, v, rng, 1.0f / 60);
    CHECK(r.acertos == 1);
}

TEST_CASE("voador destruído cai até o chão") {
    auto a = salaComPoco();
    auto v = std::vector{jogo::criarInimigo(jogo::TipoInimigo::Vespa, {4.5f, 1.5f}, 2)};  // em cima do poço
    v[0].corpo.pes = 2.0f;
    v[0].estado = jogo::EstadoInimigo::Destruido;
    simularVoo(v, a.grade, a.inicio, 1.0f, 3.0f);
    CHECK(v[0].corpo.pes == doctest::Approx(0.0f));
}

TEST_CASE("plataforma não protege do voador") {
    // Jogador numa plataforma de 2 blocos (colunas 1–2); vespa no chão da sala
    auto a = mundo::andarDeTexto({"#########", "#@.....v#", "#########"}, 2, {"000000000", "088000000", "000000000"},
                                 {"000000000", "0ggggggg0", "000000000"});
    auto v = jogo::inimigosDasMarcas(a);
    const auto e = simularVoo(v, a.grade, a.inicio, 2.0f, 6.0f, [&](const auto& vs) { conferirAltura(vs, a.grade); });
    CHECK_FALSE(e.golpes.empty());
}

namespace {
size_t bombas(const jogo::EventosIA& e) {
    size_t n = 0;
    for (const auto& t : e.tiros) n += t.raioExplosao > 0 && t.gravidade > 0;
    return n;
}
}

TEST_CASE("dirigível solta bomba com intervalo") {
    auto a = mundo::andarDeTexto({"#######", "#@.d..#", "#######"}, 3, {}, {"0000000", "0ggggg0", "0000000"});
    auto v = jogo::inimigosDasMarcas(a);
    v[0].corpo.pes = 4.0f - jogo::DESCIDA_DIRIGIVEL;
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    auto e = simularVoo(v, a.grade, a.inicio, 0.0f, 0.1f);
    REQUIRE(bombas(e) == 1);
    CHECK(e.tiros[0].velocidade == glm::vec3(0.0f));
    CHECK(simularVoo(v, a.grade, a.inicio, 0.0f, 2.0f).tiros.empty());  // 2,1 s desde a primeira
    CHECK(bombas(simularVoo(v, a.grade, a.inicio, 0.0f, 0.6f)) == 1);
}

TEST_CASE("dirigível sem ver não solta bomba") {
    auto a = mundo::andarDeTexto({"#####", "#@#d#", "#####"}, 3, {}, {"00000", "0ggg0", "00000"});
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    CHECK(simularVoo(v, a.grade, a.inicio, 0.0f, 3.0f).tiros.empty());
}

TEST_CASE("bomba com dirigível em cima do jogador") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 3, {}, {"00000", "0ggg0", "00000"});
    auto v = std::vector{jogo::criarInimigo(jogo::TipoInimigo::Dirigivel, a.inicio, 3)};
    v[0].corpo.pes = 4.0f - jogo::DESCIDA_DIRIGIVEL;
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    auto e = simularVoo(v, a.grade, a.inicio, 0.0f, 1.0f / 60);
    REQUIRE(bombas(e) == 1);
    CHECK_FALSE(std::isnan(e.tiros[0].posicao.x));
    CHECK_FALSE(std::isnan(v[0].posicao.x));
    CHECK(e.tiros[0].velocidade == glm::vec3(0.0f));
}

TEST_CASE("pilha de caixas numa sala baixa não prende o voador no teto") {
    // Sala com teto 2 blocos; pilha (chão 1,75, teto da sala) logo abaixo da linha por onde a vespa vem
    auto a = mundo::andarDeTexto({"#######", "#@...v#", "#.....#", "#######"}, 2,
                                 {"0000000", "0000000", "0007000", "0000000"},
                                 {"0000000", "0888880", "0881880", "0000000"});
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    const auto e = simularVoo(v, a.grade, a.inicio, 0.0f, 6.0f, [&](const auto& vs) { conferirAltura(vs, a.grade); });
    CHECK_FALSE(e.golpes.empty());
}

TEST_CASE("voador acordado por barulho avisa uma vez") {
    // Parede no meio: ele não vê o jogador, mas o barulho o acorda
    auto a = mundo::andarDeTexto({"#######", "#@.#.d#", "#.....#", "#######"}, 3);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    std::mt19937 rng(1);
    const auto campo = mundo::distanciasComAltura(a.grade, glm::ivec2(a.inicio), mundo::ANDARILHO);
    const auto voador = mundo::distanciasComAltura(a.grade, glm::ivec2(a.inicio), mundo::VOADOR);
    jogo::MundoIA m{a.grade, a.inicio, campo, rng};
    m.campoVoador = &voador;
    auto e = jogo::atualizarInimigos(v, m, 1.0f / 60);
    REQUIRE(e.alertas.size() == 1);
    CHECK(e.alertas[0].tipo == jogo::TipoInimigo::Dirigivel);
    CHECK(jogo::atualizarInimigos(v, m, 1.0f / 60).alertas.empty());
}

TEST_CASE("voador que vê o jogador avisa uma vez") {
    auto a = mundo::andarDeTexto({"#######", "#@...v#", "#######"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    std::mt19937 rng(1);
    const auto campo = mundo::distanciasComAltura(a.grade, glm::ivec2(a.inicio), mundo::ANDARILHO);
    jogo::MundoIA m{a.grade, a.inicio, campo, rng};
    m.campoVoador = &campo;
    REQUIRE(jogo::atualizarInimigos(v, m, 1.0f / 60).alertas.size() == 1);
    CHECK(jogo::atualizarInimigos(v, m, 1.0f / 60).alertas.empty());
}

TEST_CASE("vespa destruída rente ao teto não salta para baixo") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 2);  // teto 1 bloco
    auto v = std::vector{jogo::criarInimigo(jogo::TipoInimigo::Vespa, {3.5f, 1.5f}, 2)};
    v[0].corpo.pes = 1.0f - 0.3f - jogo::FOLGA_VOO;
    v[0].estado = jogo::EstadoInimigo::Destruido;
    simularVoo(v, a.grade, a.inicio, 0.0f, 1.0f / 60);
    CHECK(v[0].corpo.pes > 0.55f);
}

TEST_CASE("dirigível rente ao chão não solta bomba") {
    // Corredor com teto 1,25: o dirigível voa baixo e a bomba não daria tempo de fugir
    auto a = mundo::andarDeTexto({"#######", "#@.d..#", "#######"}, 3, {}, {"0000000", "0555550", "0000000"});
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    CHECK(simularVoo(v, a.grade, a.inicio, 0.0f, 3.0f).tiros.empty());
}
