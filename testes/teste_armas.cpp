#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string
#include <random>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "jogo/Armas.h"
#include "mundo/Gerador.h"

namespace {

jogo::Comandos comando(bool atacar, bool trocar = false, bool recarregar = false) {
    jogo::Comandos c;
    c.atacar = atacar;
    c.trocarArma = trocar;
    c.recarregar = recarregar;
    return c;
}

// Inventário com as armas pedidas equipadas
jogo::Inventario inventarioCom(jogo::TipoItem branca, jogo::TipoItem fogo) {
    jogo::Inventario inv;
    if (inv.armaBranca().tipo != branca) {
        inv.adicionar(jogo::criarItem(branca));
        inv.equipar(1);
        inv.remover(1);
    }
    if (inv.armaDeFogo().tipo != fogo) {
        inv.adicionar(jogo::criarItem(fogo));
        inv.equipar(1);
        inv.remover(1);
    }
    return inv;
}

// Um passo da arma ativa com o comando dado (olhando para +X)
jogo::ResultadoAtaque passo(jogo::Armas& a, jogo::Inventario& inv, const mundo::Andar& andar,
                            std::vector<jogo::Inimigo>& v, std::mt19937& rng, jogo::Comandos c = comando(true),
                            float inclinacao = 0.0f, int nivel = 1) {
    return jogo::atualizarArmas(a, inv, jogo::calcularAtributos(inv, nivel), c, andar.inicio, 0.0f, 0.0f, inclinacao,
                                andar.grade, v, rng, 1.0f / 60);
}

jogo::Armas deFogo() {
    jogo::Armas a;
    a.ativa = jogo::ArmaAtiva::Fogo;
    return a;
}

}

TEST_CASE("chave inglesa acerta à frente, empurra e interrompe") {
    auto a = mundo::andarDeTexto({"#######", "#@a...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Preparando;
    jogo::Inventario inv;
    jogo::Armas armas;
    std::mt19937 rng(1);
    auto r = passo(armas, inv, a, v, rng);
    CHECK(r.acertos == 1);
    CHECK(v[0].vida <= 16 - 4);
    CHECK(v[0].vida >= 16 - 6);
    CHECK(v[0].estado == jogo::EstadoInimigo::Perseguindo);
    CHECK(glm::length(v[0].empurrao) == doctest::Approx(0.3f));
    CHECK(r.barulho == doctest::Approx(4.0f));
}

TEST_CASE("chave inglesa não acerta atrás nem longe") {
    auto a = mundo::andarDeTexto({"########", "#a@...a#", "########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    jogo::Armas armas;
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng).acertos == 0);
}

TEST_CASE("defesa do colosso reduz o dano") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    jogo::Armas armas;
    std::mt19937 rng(1);
    passo(armas, inv, a, v, rng);
    CHECK(v[0].vida >= 40 - 4);  // 4–6 menos 2
    CHECK(v[0].vida <= 40 - 2);
}

TEST_CASE("segurar o botão respeita o intervalo") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    jogo::Armas armas;
    std::mt19937 rng(1);
    int golpes = 0;
    for (int i = 0; i < 60; ++i) golpes += passo(armas, inv, a, v, rng).acertos;
    CHECK(golpes == 3);  // 1 s ÷ 0,35 s → golpes em 0; 0,35; 0,70
}

TEST_CASE("revólver acerta o primeiro inimigo e não atravessa parede") {
    auto a = mundo::andarDeTexto({"###########", "#@...a#..a#", "###########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    auto armas = deFogo();
    std::mt19937 rng(1);
    auto r = passo(armas, inv, a, v, rng);
    CHECK(r.disparou);
    CHECK(inv.armaDeFogo().carga == 5);
    CHECK(v[0].vida <= 16 - 7);
    CHECK(v[1].vida == 16);
    CHECK(r.barulho == doctest::Approx(10.0f));
}

TEST_CASE("revólver vazio avisa e recarga move balas da reserva") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Inventario inv;
    inv.armaDeFogo().carga = 0;
    auto armas = deFogo();
    armas.municao.balas = 4;
    std::mt19937 rng(1);
    auto r = passo(armas, inv, a, v, rng);
    CHECK_FALSE(r.disparou);
    CHECK(r.mensagem == "Sem munição — R para recarregar");
    passo(armas, inv, a, v, rng, comando(false, false, true));
    for (int i = 0; i < 90; ++i) passo(armas, inv, a, v, rng, comando(false));
    CHECK(inv.armaDeFogo().carga == 4);
    CHECK(armas.municao.balas == 0);
}

TEST_CASE("sem bala nenhuma a mensagem é outra") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Inventario inv;
    inv.armaDeFogo().carga = 0;
    auto armas = deFogo();
    armas.municao.balas = 0;
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng).mensagem == "Sem munição");
}

TEST_CASE("recarregar cheio não faz nada; trocar de arma cancela a recarga") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Inventario inv;
    auto armas = deFogo();
    std::mt19937 rng(1);
    passo(armas, inv, a, v, rng, comando(false, false, true));
    CHECK(armas.recarga == 0.0f);
    inv.armaDeFogo().carga = 2;
    passo(armas, inv, a, v, rng, comando(false, false, true));
    CHECK(armas.recarga > 0.0f);
    passo(armas, inv, a, v, rng, comando(false, true));
    CHECK(armas.ativa == jogo::ArmaAtiva::Branca);
    CHECK(armas.recarga == 0.0f);
    CHECK(inv.armaDeFogo().carga == 2);
}

TEST_CASE("inimigo com vida zero é destruído") {
    auto a = mundo::andarDeTexto({"#####", "#@r.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].vida = 1;
    jogo::Inventario inv;
    jogo::Armas armas;
    std::mt19937 rng(1);
    auto r = passo(armas, inv, a, v, rng);
    CHECK(r.destruidos == 1);
    CHECK(v[0].estado == jogo::EstadoInimigo::Destruido);
}

TEST_CASE("tiro reto passa por cima do rato colado nos pés; olhando para baixo acerta") {
    auto a = mundo::andarDeTexto({"#######", "#@r...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    auto armas = deFogo();
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng, comando(true), 0.0f).acertos == 0);
    armas.intervalo = 0;
    CHECK(passo(armas, inv, a, v, rng, comando(true), glm::radians(-20.0f)).acertos == 1);
}

TEST_CASE("rato longe é acertado mirando reto (tolerância vertical)") {
    auto a = mundo::andarDeTexto({"##########", "#@......r#", "##########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    auto armas = deFogo();
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng).acertos == 1);
}

TEST_CASE("tiro para o teto não acerta o autômato à frente") {
    auto a = mundo::andarDeTexto({"#######", "#@..a.#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    auto armas = deFogo();
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng, comando(true), glm::radians(50.0f)).acertos == 0);
}

TEST_CASE("sabre alcança mais longe que a chave") {
    auto a = mundo::andarDeTexto({"########", "#@.a...#", "########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].posicao.x = 1.5f + 1.5f;  // autômato a 1,5 do jogador = 1,25 até a borda: o sabre (1,3) alcança, a chave (1,1) não
    auto inv = inventarioCom(jogo::TipoItem::Sabre, jogo::TipoItem::Revolver);
    jogo::Armas armas;
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng).acertos == 1);
    auto inv2 = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Revolver);
    jogo::Armas armas2;
    CHECK(passo(armas2, inv2, a, v, rng).acertos == 0);
}

TEST_CASE("martelo empurra 0,6") {
    auto a = mundo::andarDeTexto({"#######", "#@a...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    auto inv = inventarioCom(jogo::TipoItem::Martelo, jogo::TipoItem::Revolver);
    jogo::Armas armas;
    std::mt19937 rng(1);
    passo(armas, inv, a, v, rng);
    CHECK(glm::length(v[0].empurrao) == doctest::Approx(0.6f));
}

TEST_CASE("bacamarte: 7 chumbos, dano cai pela metade longe") {
    auto perto = mundo::andarDeTexto({"#######", "#@.c..#", "#######"}, 1);      // colosso a 2
    auto longe = mundo::andarDeTexto({"#########", "#@.....c#", "#########"}, 1);  // colosso a 6
    auto vp = jogo::inimigosDasMarcas(perto), vl = jogo::inimigosDasMarcas(longe);
    auto inv = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Bacamarte);
    auto inv2 = inv;
    std::mt19937 rng(5);
    auto a1 = deFogo(), a2 = deFogo();
    passo(a1, inv, perto, vp, rng);
    passo(a2, inv2, longe, vl, rng);
    const int danoPerto = 40 - vp[0].vida, danoLonge = 40 - vl[0].vida;
    CHECK(danoPerto >= 7);  // 7 × (2–4 − defesa 2, mínimo 1)
    CHECK(danoPerto > danoLonge);
    CHECK(inv.armaDeFogo().carga == 1);
}

TEST_CASE("Tesla gasta célula da reserva e não recarrega") {
    auto a = mundo::andarDeTexto({"####################", "#@.............a...#", "####################"}, 4);
    auto v = jogo::inimigosDasMarcas(a);
    auto inv = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Tesla);
    auto armas = deFogo();
    armas.municao.celulas = 2;
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng).acertos == 1);  // a 14 células: dentro do alcance 15
    CHECK(armas.municao.celulas == 1);
    armas.intervalo = 0;
    passo(armas, inv, a, v, rng, comando(false, false, true));
    CHECK(armas.recarga == 0.0f);
}

TEST_CASE("Tesla não alcança além de 15") {
    auto a = mundo::andarDeTexto({"######################", "#@.................a.#", "######################"}, 4);
    auto v = jogo::inimigosDasMarcas(a);
    auto inv = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Tesla);
    auto armas = deFogo();
    armas.municao.celulas = 1;
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng).acertos == 0);  // a 18 células
}

TEST_CASE("Tesla sem células avisa") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 4);
    std::vector<jogo::Inimigo> v;
    auto inv = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Tesla);
    auto armas = deFogo();
    std::mt19937 rng(1);
    CHECK(passo(armas, inv, a, v, rng).mensagem == "Sem células elétricas");
}

TEST_CASE("bônus de velocidade e de carga") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    inv.adicionar({jogo::TipoItem::ChaveInglesa, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Velocidade, 20}}});
    inv.equipar(1);
    jogo::Armas armas;
    std::mt19937 rng(1);
    passo(armas, inv, a, v, rng);
    CHECK(armas.intervalo == doctest::Approx(0.35f * 0.8f).epsilon(0.02));
    jogo::Item rev{jogo::TipoItem::Revolver, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Carga, 2}}};
    CHECK(jogo::capacidade(rev) == 8);
}

TEST_CASE("dano base do nível soma ao golpe") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    jogo::Armas armas;
    std::mt19937 rng(1);
    passo(armas, inv, a, v, rng, comando(true), 0.0f, 5);
    CHECK(v[0].vida <= 40 - (4 + 4 - 2));  // 4–6 + 4 de base − defesa 2
}

TEST_CASE("máximos de munição") {
    CHECK(jogo::maximoMunicao(jogo::TipoMunicao::Balas) == 48);
    CHECK(jogo::maximoMunicao(jogo::TipoMunicao::Cartuchos) == 20);
    CHECK(jogo::maximoMunicao(jogo::TipoMunicao::Celulas) == 12);
}

TEST_CASE("resultado do ataque informa golpe, arma vazia e recarga") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Inventario inv;
    std::mt19937 rng(1);
    jogo::Armas branca;
    auto golpe = passo(branca, inv, a, v, rng);
    CHECK(golpe.golpeou);
    CHECK(golpe.acertos == 0);

    // Revólver vazio com o botão segurado por 1 s: o clique respeita o intervalo
    inv.armaDeFogo().carga = 0;
    auto armas = deFogo();
    armas.municao.balas = 12;
    int vazias = 0;
    for (int i = 0; i < 60; ++i) vazias += passo(armas, inv, a, v, rng).vazia ? 1 : 0;
    CHECK(vazias >= 1);
    CHECK(vazias <= 3);  // ceil(1 / 0,4)

    // R começa a recarga uma vez só
    CHECK(passo(armas, inv, a, v, rng, comando(false, false, true)).comecouRecarga);
    CHECK_FALSE(passo(armas, inv, a, v, rng, comando(false, false, true)).comecouRecarga);
    CHECK_FALSE(passo(armas, inv, a, v, rng, comando(false)).golpeou);
}

TEST_CASE("bacamarte: bônus de dano entra pela metade em cada chumbo (arredondando para cima)") {
    CHECK(jogo::bonusPorChumbo(jogo::dadosArma(jogo::TipoItem::Bacamarte), 7) == 4);
    CHECK(jogo::bonusPorChumbo(jogo::dadosArma(jogo::TipoItem::Bacamarte), 1) == 1);
    CHECK(jogo::bonusPorChumbo(jogo::dadosArma(jogo::TipoItem::Bacamarte), 0) == 0);
    CHECK(jogo::bonusPorChumbo(jogo::dadosArma(jogo::TipoItem::Revolver), 7) == 7);  // arma de um projétil: inteiro

    // Colosso a 2 (defesa 2), bacamarte +4 no nível 1: cada chumbo faz 2–4 + 2 − 2 = 2–4
    auto perto = mundo::andarDeTexto({"#######", "#@.c..#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(perto);
    auto inv = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Bacamarte);
    inv.armaDeFogo().bonus = {{jogo::TipoBonus::Dano, 4}};
    std::mt19937 rng(5);
    auto a = deFogo();
    passo(a, inv, perto, v, rng);
    const int dano = 40 - v[0].vida;
    CHECK(dano >= 7);
    CHECK(dano <= 7 * 4);  // antes (+4 inteiro em cada chumbo) seriam 28–42
}

namespace {
// Passo da arma com os pés numa altura e a posição dada
jogo::ResultadoAtaque passoEm(jogo::Armas& a, jogo::Inventario& inv, const mundo::Andar& andar, std::vector<jogo::Inimigo>& v,
                              std::mt19937& rng, glm::vec2 pos, float pes, float inclinacao) {
    return jogo::atualizarArmas(a, inv, jogo::calcularAtributos(inv, 1), comando(true), pos, pes, 0.0f, inclinacao,
                                andar.grade, v, rng, 1.0f / 60);
}
}

TEST_CASE("tiro de cima acerta inimigo embaixo") {
    // Jogador numa plataforma de 2 blocos; autômato no chão 4 células à frente
    auto a = mundo::andarDeTexto({"########", "#@...a.#", "########"}, 1, {"00000000", "08800000", "00000000"},
                                 {"00000000", "0kkkkkk0", "00000000"});
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    auto armas = deFogo();
    std::mt19937 rng(1);
    // Da beira (de longe, a própria plataforma barra o tiro para baixo)
    const auto r = passoEm(armas, inv, a, v, rng, {2.7f, 1.5f}, 2.0f, -0.64f);  // ~37° para baixo
    CHECK(r.acertos == 1);
    // Mirando reto, passa por cima
    auto v2 = jogo::inimigosDasMarcas(a);
    auto armas2 = deFogo();
    CHECK(passoEm(armas2, inv, a, v2, rng, {2.7f, 1.5f}, 2.0f, 0.0f).acertos == 0);
}

TEST_CASE("tiro reto para na caixa do caminho") {
    auto a = mundo::andarDeTexto({"########", "#@....a#", "########"}, 1, {"00000000", "00040000", "00000000"},
                                 {"00000000", "08888880", "00000000"});
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    auto armas = deFogo();
    std::mt19937 rng(1);
    CHECK(passoEm(armas, inv, a, v, rng, {1.5f, 1.5f}, 0.0f, 0.0f).acertos == 0);
}

TEST_CASE("golpe não alcança quem está 1 bloco abaixo") {
    // Jogador numa plataforma de 1 bloco; rato colado embaixo, à frente
    auto a = mundo::andarDeTexto({"######", "#@r..#", "######"}, 1, {"000000", "040000", "000000"},
                                 {"000000", "088880", "000000"});
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    jogo::Armas branca;
    std::mt19937 rng(1);
    const auto r = passoEm(branca, inv, a, v, rng, {1.7f, 1.5f}, 1.0f, -0.5f);
    CHECK(r.golpeou);
    CHECK(r.acertos == 0);
    // No mesmo nível, acerta
    auto v2 = jogo::inimigosDasMarcas(a);
    jogo::Armas branca2;
    CHECK(passoEm(branca2, inv, a, v2, rng, {1.7f, 1.5f}, 0.0f, 0.0f).acertos == 1);
}

TEST_CASE("tiro acerta o chefe pela borda do corpo") {
    // Tiro reto passando 0,45 ao lado do centro: dentro do raio da Rainha (0,5), fora do rato (0,25)
    for (char letra : {'R', 'r'}) {
        auto a = mundo::andarDeTexto({"########", "#......#", std::string("#@...") + letra + ".#", "#......#", "########"}, 2);
        auto v = jogo::inimigosDasMarcas(a);
        jogo::Inventario inv;
        auto armas = deFogo();
        std::mt19937 rng(1);
        const auto r = passoEm(armas, inv, a, v, rng, {1.5f, 2.95f}, 0.0f, 0.0f);
        CHECK(r.acertos == (letra == 'R' ? 1 : 0));
    }
}

TEST_CASE("golpe não interrompe o preparo do chefe (só empurra)") {
    auto a = mundo::andarDeTexto({"#######", "#@P...#", "#######"}, 4);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].posicao = {2.6f, 1.5f};
    v[0].estado = jogo::EstadoInimigo::Preparando;
    jogo::Inventario inv;
    jogo::Armas armas;
    std::mt19937 rng(1);
    const auto r = passo(armas, inv, a, v, rng);
    CHECK(r.acertos == 1);
    CHECK(v[0].estado == jogo::EstadoInimigo::Preparando);
}
