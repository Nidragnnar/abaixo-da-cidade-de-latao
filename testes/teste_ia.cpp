#include <doctest/doctest.h>

#include <cmath>
#include <random>
#include <string>

#include <glm/geometric.hpp>

#include "jogo/IA.h"
#include "mundo/Consultas.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

namespace {

int somaDano(const jogo::EventosIA& e) {
    int s = 0;
    for (const auto& g : e.golpes) s += g.dano;
    return s;
}


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
        total.golpes.insert(total.golpes.end(), e.golpes.begin(), e.golpes.end());
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
    CHECK(somaDano(eventos) >= 4);
    CHECK(somaDano(eventos) <= 6);
    REQUIRE_FALSE(eventos.golpes.empty());
    CHECK(eventos.golpes.back().autor == jogo::TipoInimigo::Automato);
}

TEST_CASE("golpe não acerta se o jogador se afastou durante o preparo") {
    auto a = mundo::andarDeTexto({"#########", "#@a.....#", "#########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 0.1f);  // começou a preparar
    REQUIRE(v[0].estado == jogo::EstadoInimigo::Preparando);
    auto eventos = simular(v, a.grade, {6.5f, 1.5f}, 0.5f);  // jogador longe no fim do preparo
    CHECK(eventos.golpes.empty());
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
    CHECK(eventos.golpes.empty());
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

TEST_CASE("dois golpes no mesmo passo chegam separados") {
    auto a = mundo::andarDeTexto({"#####", "#r@r#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    auto e = simular(v, a.grade, {2.5f, 1.5f}, 0.45f);  // aproximam, preparam juntos (0,3 s) e mordem; recuperação 0,6 s
    CHECK(e.golpes.size() == 2);
}

TEST_CASE("atirador encurralado atira de perto") {
    auto a = mundo::andarDeTexto({"########", "#g.@...#", "########"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    auto e = simular(v, a.grade, {3.5f, 1.5f}, 2.5f);
    CHECK(e.tiros.size() >= 1);
}

namespace {
// Como simular, mas com os dois campos com alturas e os pés do jogador
jogo::EventosIA simular3D(std::vector<jogo::Inimigo>& v, mundo::Grade& g, glm::vec2 jogador, float pes, float segundos) {
    std::mt19937 rng(3);
    jogo::EventosIA total;
    std::vector<int> andarilho, saltador;
    float desdeCampo = jogo::RECALCULO_CAMINHO;
    for (int i = 0; i < static_cast<int>(std::lround(segundos * 60)); ++i) {
        if (desdeCampo >= jogo::RECALCULO_CAMINHO) {
            andarilho = mundo::distanciasComAltura(g, glm::ivec2(jogador), mundo::ANDARILHO);
            saltador = mundo::distanciasComAltura(g, glm::ivec2(jogador), mundo::SALTADOR);
            desdeCampo = 0;
        }
        jogo::MundoIA m{g, jogador, andarilho, rng};
        m.campoSaltador = &saltador;
        m.pesJogador = pes;
        auto e = jogo::atualizarInimigos(v, m, 1.0f / 60);
        total.golpes.insert(total.golpes.end(), e.golpes.begin(), e.golpes.end());
        total.tiros.insert(total.tiros.end(), e.tiros.begin(), e.tiros.end());
        desdeCampo += 1.0f / 60;
    }
    return total;
}
// Plataforma de 1 bloco nas colunas 2–5 da linha 1 (com ou sem rampa na coluna 2), chão 0 na linha 2
mundo::Andar plataforma(bool comRampa, char inimigo) {
    return mundo::andarDeTexto({"#######", "#@....#", std::string("#....") + inimigo + "#", "#######"}, 1,
                               {"0000000", comRampa ? "00l4440" : "0044440", "0000000", "0000000"},
                               {"0000000", "0999990", "0999990", "0000000"});
}
}

TEST_CASE("autômato sobe pela rampa até o jogador na plataforma") {
    auto a = plataforma(true, 'a');
    auto v = jogo::inimigosDasMarcas(a);
    simular3D(v, a.grade, {4.5f, 1.5f}, 1.0f, 10.0f);
    CHECK(glm::distance(v[0].posicao, glm::vec2(4.5f, 1.5f)) <= 1.2f);
    CHECK(v[0].corpo.pes == doctest::Approx(1.0f));
}

TEST_CASE("rato pula 1 bloco; autômato não") {
    auto a = plataforma(false, 'r');
    auto ratos = jogo::inimigosDasMarcas(a);
    simular3D(ratos, a.grade, {4.5f, 1.5f}, 1.0f, 6.0f);
    CHECK(ratos[0].corpo.pes == doctest::Approx(1.0f));
    auto b = plataforma(false, 'a');
    auto automatos = jogo::inimigosDasMarcas(b);
    simular3D(automatos, b.grade, {4.5f, 1.5f}, 1.0f, 6.0f);
    CHECK(automatos[0].corpo.pes == doctest::Approx(0.0f));
}

TEST_CASE("corpo a corpo não alcança 1 bloco acima") {
    auto a = plataforma(false, 'a');
    auto v = jogo::inimigosDasMarcas(a);
    v[0].posicao = {4.5f, 2.3f};  // colado embaixo da borda
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    const auto e = simular3D(v, a.grade, {4.5f, 1.7f}, 1.0f, 5.0f);
    CHECK(e.golpes.empty());
}

TEST_CASE("empurrado da plataforma cai e persegue") {
    auto a = plataforma(false, 'a');
    auto v = jogo::inimigosDasMarcas(a);
    v[0].posicao = {4.5f, 1.5f};
    v[0].corpo.pes = 1.0f;
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    v[0].empurrao = {0.0f, 1.0f};  // para fora da borda, para o sul
    simular3D(v, a.grade, {1.5f, 2.5f}, 0.0f, 4.0f);
    CHECK(v[0].corpo.pes == doctest::Approx(0.0f));
    CHECK(glm::distance(v[0].posicao, glm::vec2(1.5f, 2.5f)) <= 1.2f);
}

TEST_CASE("borda protege de quem atira de cima") {
    // Atirador numa plataforma de 2 blocos (colunas 1–2); jogador colado embaixo da borda
    auto a = mundo::andarDeTexto({"#######", "#g@...#", "#######"}, 1, {"0000000", "0880000", "0000000"},
                                 {"0000000", "0kkkkk0", "0000000"});
    auto v = jogo::inimigosDasMarcas(a);
    REQUIRE(v[0].corpo.pes == doctest::Approx(2.0f));
    v[0].estado = jogo::EstadoInimigo::Perseguindo;
    const auto e = simular3D(v, a.grade, {3.3f, 1.5f}, 0.0f, 5.0f);
    CHECK(e.tiros.empty());
}

TEST_CASE("inimigos não levam dano de queda") {
    auto a = mundo::andarDeTexto({"######", "#a@..#", "######"}, 1, {"000000", "0c0000", "000000"},
                                 {"000000", "0kkkk0", "000000"});
    auto v = jogo::inimigosDasMarcas(a);
    const int vida = v[0].vida;
    v[0].empurrao = {1.0f, 0.0f};
    simular3D(v, a.grade, {4.5f, 1.5f}, 0.0f, 2.0f);
    CHECK(v[0].corpo.pes == doctest::Approx(0.0f));
    CHECK(v[0].vida == vida);
}

