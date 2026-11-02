#include <doctest/doctest.h>

#include <cmath>
#include <random>

#include <glm/geometric.hpp>

#include "jogo/IA.h"
#include "mundo/Consultas.h"
#include "mundo/Gerador.h"

namespace {

// Roda a IA por alguns segundos, recalculando o campo a cada 0,5 s e abrindo na hora as comportas pedidas
jogo::EventosIA simular(std::vector<jogo::Inimigo>& v, mundo::Grade& g, glm::vec2 jogador, float segundos) {
    std::mt19937 rng(3);
    jogo::EventosIA total;
    std::vector<int> campo;
    float desdeCampo = jogo::RECALCULO_CAMINHO;
    const int n = static_cast<int>(std::lround(segundos * 60));
    for (int i = 0; i < n; ++i) {
        if (desdeCampo >= jogo::RECALCULO_CAMINHO) {
            campo = mundo::distanciasNaGrade(g, glm::ivec2(jogador), false, false);
            desdeCampo = 0;
        }
        auto e = jogo::atualizarInimigos(v, {g, jogador, campo, rng}, 1.0f / 60);
        for (auto c : e.comportasParaAbrir) g.definir(c.x, c.y, mundo::Celula::ComportaAberta);
        total.dano += e.dano;
        if (e.autor) total.autor = e.autor;
        total.tiros.insert(total.tiros.end(), e.tiros.begin(), e.tiros.end());
        desdeCampo += 1.0f / 60;
    }
    return total;
}

}

TEST_CASE("parado acorda ao ver o jogador e se aproxima") {
    auto a = mundo::andarDeTexto({"############", "#@........r#", "############"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 1.0f);
    CHECK(v[0].estado != jogo::EstadoInimigo::Parado);
    CHECK(v[0].posicao.x < 10.5f - 2.0f);  // rato rápido: 3,5 células/s
}

TEST_CASE("parede impede ver; barulho acorda mesmo assim") {
    auto a = mundo::andarDeTexto({"#########", "#@..#..a#", "#########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 0.5f);
    CHECK(v[0].estado == jogo::EstadoInimigo::Parado);
    jogo::acordarPorBarulho(v, a.inicio, 10.0f);
    CHECK(v[0].estado == jogo::EstadoInimigo::Perseguindo);
}

TEST_CASE("autômato prepara o golpe e acerta quem ficou perto") {
    auto a = mundo::andarDeTexto({"#####", "#@a.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    auto eventos = simular(v, a.grade, a.inicio, 0.7f);  // aproxima até 0,9 e prepara 0,5 s
    CHECK(eventos.dano >= 4);
    CHECK(eventos.dano <= 6);
    CHECK(eventos.autor == jogo::TipoInimigo::Automato);
}

TEST_CASE("golpe não acerta se o jogador se afastou durante o preparo") {
    auto a = mundo::andarDeTexto({"#########", "#@a.....#", "#########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 0.1f);  // começou a preparar
    REQUIRE(v[0].estado == jogo::EstadoInimigo::Preparando);
    auto eventos = simular(v, a.grade, {6.5f, 1.5f}, 0.5f);  // jogador longe no fim do preparo
    CHECK(eventos.dano == 0);
}

TEST_CASE("atirador mantém distância e atira") {
    auto a = mundo::andarDeTexto({"##############", "#@...g.......#", "##############"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    auto eventos = simular(v, a.grade, a.inicio, 2.0f);
    CHECK(glm::distance(v[0].posicao, a.inicio) >= 3.0f - 0.3f);
    REQUIRE(eventos.tiros.size() >= 1);
    CHECK(glm::length(eventos.tiros[0].velocidade) == doctest::Approx(10.0f));
}

TEST_CASE("atirador perto demais recua") {
    auto a = mundo::andarDeTexto({"##############", "#@g..........#", "##############"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 1.5f);
    CHECK(glm::distance(v[0].posicao, a.inicio) > 2.5f);
}

TEST_CASE("inimigo abre a comporta e passa") {
    auto a = mundo::andarDeTexto({"###########", "#@...C...a#", "###########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simular(v, a.grade, a.inicio, 5.0f);
    CHECK(a.grade.em(5, 1) == mundo::Celula::ComportaAberta);
    CHECK(v[0].posicao.x < 5.0f);
}

TEST_CASE("comporta trancada segura o inimigo") {
    auto a = mundo::andarDeTexto({"###########", "#@...T...a#", "###########"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simular(v, a.grade, a.inicio, 5.0f);
    CHECK(v[0].posicao.x > 6.0f);
}

TEST_CASE("dois inimigos no corredor não se sobrepõem") {
    auto a = mundo::andarDeTexto({"############", "#@.......aa#", "############"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simular(v, a.grade, {1.5f, 1.5f}, 3.0f);
    CHECK(glm::distance(v[0].posicao, v[1].posicao) >= 2 * jogo::RAIO_INIMIGO - 0.05f);
}

TEST_CASE("empurrão interrompe e desloca") {
    auto a = mundo::andarDeTexto({"#######", "#@a...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 0.1f);
    REQUIRE(v[0].estado == jogo::EstadoInimigo::Preparando);
    const float antes = v[0].posicao.x;
    v[0].empurrao = {0.3f, 0.0f};
    v[0].estado = jogo::EstadoInimigo::Perseguindo;  // quem interrompe é a arma (Tarefa 5)
    simular(v, a.grade, a.inicio, 1.0f / 60);
    CHECK(v[0].posicao.x > antes + 0.2f);
}

TEST_CASE("destruído fica parado e só conta o tempo") {
    auto a = mundo::andarDeTexto({"#####", "#@a.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Destruido;
    auto antes = v[0].posicao;
    auto eventos = simular(v, a.grade, a.inicio, 2.0f);
    CHECK(eventos.dano == 0);
    CHECK(v[0].posicao == antes);
    CHECK(v[0].tempoNoEstado == doctest::Approx(2.0f).epsilon(0.02));
}

TEST_CASE("fila no corredor não empurra o primeiro através do jogador") {
    auto a = mundo::andarDeTexto({"###########", "#@.rrrr...#", "###########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simular(v, a.grade, a.inicio, 5.0f);
    for (const auto& i : v) {
        CHECK(i.posicao.x > a.inicio.x);  // ninguém passou para trás do jogador
        CHECK(glm::distance(i.posicao, a.inicio) >= 2 * jogo::RAIO_INIMIGO - 0.02f);
    }
}

TEST_CASE("inimigo contorna o elevador no meio da sala") {
    auto a = mundo::andarDeTexto({"#########", "#.......#", "#@..E..a#", "#.......#", "#########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simular(v, a.grade, a.inicio, 6.0f);
    CHECK(glm::distance(v[0].posicao, a.inicio) <= 1.0f);
}
