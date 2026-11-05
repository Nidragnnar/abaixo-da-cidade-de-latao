#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string
#include <random>
#include <stdexcept>

#include "mundo/Gerador.h"
#include "mundo/Consultas.h"
#include "mundo/Terreno.h"

using mundo::A_PE;
using mundo::ANDARILHO;
using mundo::SALTADOR;

namespace {
// Corredor de 1 linha com as alturas dadas (e tetos opcionais)
mundo::Grade corredor(const std::string& alturas, const std::string& tetos = {}) {
    const std::string meio = "#@" + std::string(alturas.size() - 3, '.') + "#";
    const std::string borda(alturas.size(), '#');
    const std::string zeros(alturas.size(), '0');
    return mundo::andarDeTexto({borda, meio, borda}, 1, {zeros, alturas, zeros},
                               tetos.empty() ? std::vector<std::string>{} : std::vector<std::string>{zeros, tetos, zeros})
        .grade;
}
}

TEST_CASE("mapa sem camada de altura fica no nível 0 com teto 4") {
    const auto a = mundo::andarDeTexto({"####", "#@.#", "####"}, 1);
    CHECK(a.grade.chao(1, 1) == 0);
    CHECK(a.grade.teto(2, 1) == mundo::TETO_PADRAO);
    CHECK(a.grade.rampa(2, 1) == mundo::Rampa::Nenhuma);
    CHECK(a.grade.detalhe(2, 1) == mundo::Detalhe::Nenhum);
    CHECK(mundo::alturaDoChao(a.grade, {1.5f, 1.5f}) == doctest::Approx(0));
    CHECK(mundo::alturaDoTeto(a.grade, {1.5f, 1.5f}) == doctest::Approx(1.0f));
}

TEST_CASE("camada de alturas: dígitos e rampas") {
    const auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1, {"00000", "02l40", "00000"});
    CHECK(a.grade.chao(1, 1) == 2);
    CHECK(a.grade.rampa(2, 1) == mundo::Rampa::Leste);
    CHECK(a.grade.chao(3, 1) == 4);
    CHECK(a.grade.teto(1, 1) == 2 + mundo::TETO_PADRAO);  // padrão: chão + 4
    CHECK(mundo::alturaDoChao(a.grade, {2.0f, 1.5f}) == doctest::Approx(0.0f));
    CHECK(mundo::alturaDoChao(a.grade, {2.5f, 1.5f}) == doctest::Approx(0.5f));
    CHECK(mundo::alturaDoChao(a.grade, {2.99f, 1.5f}) == doctest::Approx(0.99f).epsilon(0.01));
    CHECK(mundo::alturaDoChao(a.grade, {1.5f, 1.5f}) == doctest::Approx(0.5f));
    // Maiúscula: rampa a partir de 1 bloco
    const auto b = mundo::andarDeTexto({"####", "#@.#", "####"}, 1, {"0000", "04L0", "0000"});
    CHECK(mundo::alturaDoChao(b.grade, {2.5f, 1.5f}) == doctest::Approx(1.5f));
    // Teto explícito
    const auto c = mundo::andarDeTexto({"####", "#@.#", "####"}, 1, {}, {"0000", "0090", "0000"});
    CHECK(c.grade.teto(2, 1) == 9);
}

TEST_CASE("camada com tamanho diferente é inválida") {
    CHECK_THROWS_AS(mundo::andarDeTexto({"###", "#@#", "###"}, 1, {"000", "00", "000"}), std::invalid_argument);
    CHECK_THROWS_AS(mundo::andarDeTexto({"###", "#@#", "###"}, 1, {"000", "0x0", "000"}), std::invalid_argument);
    CHECK_THROWS_AS(mundo::andarDeTexto({"###", "#@#", "###"}, 1, {"000", "000"}), std::invalid_argument);
}

TEST_CASE("passagem a pé: degrau de 1/4 sim, de 1/2 não; descida até 2 blocos") {
    const auto g = corredor("0012900");  // alturas 0 0 1 2 9 0 0
    CHECK(mundo::podePassar(g, {1, 1}, {2, 1}, A_PE));   // 0 → 1
    CHECK(mundo::podePassar(g, {2, 1}, {3, 1}, A_PE));   // 1 → 2
    CHECK_FALSE(mundo::podePassar(g, {3, 1}, {4, 1}, A_PE));  // 2 → 9
    CHECK_FALSE(mundo::podePassar(g, {4, 1}, {5, 1}, A_PE));  // 9 → 0: queda de 2,25 blocos
    CHECK(mundo::podePassar(g, {4, 1}, {5, 1}, ANDARILHO));
    const auto h = corredor("00800");
    CHECK(mundo::podePassar(h, {2, 1}, {3, 1}, A_PE));  // 8 → 0: 2 blocos, sem dano
    const auto s = corredor("00450");
    CHECK(mundo::podePassar(s, {1, 1}, {2, 1}, SALTADOR));       // 0 → 4
    CHECK_FALSE(mundo::podePassar(s, {1, 1}, {2, 1}, ANDARILHO));
    CHECK_FALSE(mundo::podePassar(s, {1, 1}, {3, 1}, SALTADOR));  // não vizinhas: nada
    const auto t = corredor("00000", "00200");  // teto 2 quartos na célula 2
    CHECK_FALSE(mundo::podePassar(t, {1, 1}, {2, 1}, SALTADOR));
}

TEST_CASE("rampa liga níveis de 1 bloco pela borda certa") {
    const auto a = mundo::andarDeTexto({"#####", "#@..#", "#.###", "#####"}, 1, {"00000", "00l40", "00000", "00000"});
    // 1,1 (chão 0) → 2,1 (rampa leste 0→4) → 3,1 (chão 4)
    CHECK(mundo::podePassar(a.grade, {1, 1}, {2, 1}, A_PE));
    CHECK(mundo::podePassar(a.grade, {2, 1}, {3, 1}, A_PE));
    CHECK(mundo::podePassar(a.grade, {3, 1}, {2, 1}, A_PE));
    CHECK(mundo::chaoNaBorda(a.grade, {2, 1}, {1, 0}) == doctest::Approx(1.0f));
    CHECK(mundo::chaoNaBorda(a.grade, {2, 1}, {-1, 0}) == doctest::Approx(0.0f));
    CHECK(mundo::chaoNaBorda(a.grade, {2, 1}, {0, 1}) == doctest::Approx(0.5f));  // lateral: altura média
    // Pela lateral (vindo de 2,2 que é parede aqui; usa 1,2 → 1,1 só para garantir que parede bloqueia)
    CHECK_FALSE(mundo::podePassar(a.grade, {1, 1}, {1, 0}, SALTADOR));
}

TEST_CASE("trancada e elevador seguem as regras") {
    const auto a = mundo::andarDeTexto({"#####", "#@TE#", "#####"}, 1);
    CHECK_FALSE(mundo::podePassar(a.grade, {1, 1}, {2, 1}, A_PE));
    mundo::RegrasPassagem abre = A_PE;
    abre.atravessaTrancada = true;
    CHECK(mundo::podePassar(a.grade, {1, 1}, {2, 1}, abre));
    CHECK_FALSE(mundo::podePassar(a.grade, {2, 1}, {3, 1}, abre));
    abre.atravessaElevador = true;
    CHECK(mundo::podePassar(a.grade, {2, 1}, {3, 1}, abre));
}

namespace {
int distancia(const mundo::Grade& g, glm::ivec2 alvo, glm::ivec2 de, const mundo::RegrasPassagem& r) {
    return mundo::distanciasComAltura(g, alvo, r)[static_cast<size_t>(de.y * g.largura() + de.x)];
}
}

TEST_CASE("campo do andarilho contorna pela rampa; o do saltador sobe direto") {
    // Plataforma de 1 bloco (linha 1, colunas 3–5) com rampa em (2,1) subindo para leste
    const auto a = mundo::andarDeTexto({"#######", "#@....#", "#.....#", "#######"}, 1,
                                       {"0000000", "00l4440", "0000000", "0000000"},
                                       {"0000000", "0088880", "0000000", "0000000"});
    const glm::ivec2 alvo{5, 1}, embaixo{5, 2};
    const int andarilho = distancia(a.grade, alvo, embaixo, ANDARILHO);
    const int saltador = distancia(a.grade, alvo, embaixo, SALTADOR);
    CHECK(saltador == 1);       // sobe direto
    CHECK(andarilho == 9);  // não sobe 1 bloco nem entra na rampa pela lateral: (5,2)…(1,2)→(1,1)→rampa→(5,1)
    CHECK(andarilho > saltador);
    // E desce direto (descida livre)
    CHECK(distancia(a.grade, embaixo, alvo, ANDARILHO) == 1);
}

TEST_CASE("campo a pé não desce poço fundo") {
    // Poço de 3 blocos (célula 3,1) com rampa de saída? Aqui sem saída: a pé não entra pela borda
    const auto a = mundo::andarDeTexto({"######", "#@...#", "######"}, 1, {"0cc0cc", "0cc0cc", "0cc0cc"},
                                       {"000000", "0kk0k0", "000000"});
    CHECK(distancia(a.grade, {3, 1}, {2, 1}, ANDARILHO) == 1);  // cai 3 blocos: andarilho aceita
    CHECK(distancia(a.grade, {3, 1}, {2, 1}, A_PE) == -1);      // a pé, não
}

TEST_CASE("borda de plataforma esconde quem está atrás e abaixo") {
    // Plataforma de 1 bloco nas colunas 1–2; chão 0 nas colunas 3–6; tetos altos
    const auto a = mundo::andarDeTexto({"########", "#@.....#", "########"}, 1, {"00440000", "04400000", "00440000"},
                                       {"00000000", "0cccccc0", "00000000"});
    const glm::vec3 olho{1.5f, 1.5f, 1.5f};
    CHECK_FALSE(mundo::temLinhaDeVisao3D(a.grade, olho, {3.2f, 0.3f, 1.5f}));  // colado embaixo da borda
    CHECK(mundo::temLinhaDeVisao3D(a.grade, olho, {6.5f, 0.5f, 1.5f}));        // longe: vê por cima
}

TEST_CASE("teto baixo barra a visão de cima") {
    const auto a = mundo::andarDeTexto({"#######", "#@....#", "#######"}, 1, {}, {"0000000", "0cc6cc0", "0000000"});
    CHECK_FALSE(mundo::temLinhaDeVisao3D(a.grade, {1.5f, 2.0f, 1.5f}, {5.5f, 2.0f, 1.5f}));  // teto 1,5 no meio
    CHECK(mundo::temLinhaDeVisao3D(a.grade, {1.5f, 1.0f, 1.5f}, {5.5f, 1.0f, 1.5f}));
}

TEST_CASE("visão 3D em mapa plano igual à 2D") {
    auto a = mundo::gerarAndar(42, 1);
    for (int y = 0; y < 64; ++y)  // achata o relevo: só o desenho 2D importa aqui
        for (int x = 0; x < 64; ++x) {
            a.grade.definirAltura(x, y, 0, mundo::TETO_PADRAO);
            a.grade.definirRampa(x, y, mundo::Rampa::Nenhuma);
        }
    std::mt19937 rng(3);
    std::uniform_real_distribution<float> u(1.0f, 63.0f);
    int comparados = 0;
    for (int i = 0; i < 20000 && comparados < 200; ++i) {
        const glm::vec2 p{u(rng), u(rng)}, q{u(rng), u(rng)};
        if (a.grade.solida(static_cast<int>(p.x), static_cast<int>(p.y)) ||
            a.grade.solida(static_cast<int>(q.x), static_cast<int>(q.y)))
            continue;
        ++comparados;
        CHECK(mundo::temLinhaDeVisao3D(a.grade, {p.x, 0.5f, p.y}, {q.x, 0.5f, q.y}) ==
              mundo::temLinhaDeVisao(a.grade, p, q));
    }
    CHECK(comparados == 200);
}
