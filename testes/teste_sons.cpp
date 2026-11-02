#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string
#include <filesystem>
#include <set>
#include <string>

#include "jogo/Sons.h"

TEST_CASE("cada som tem um nome de arquivo único") {
    std::set<std::string> nomes;
    for (int i = 0; i < static_cast<int>(jogo::Som::TOTAL); ++i) {
        const std::string nome(jogo::nomeDoSom(static_cast<jogo::Som>(i)));
        INFO(i);
        CHECK_FALSE(nome.empty());
        CHECK(nomes.insert(nome).second);
    }
    CHECK(jogo::nomeDoSom(jogo::Som::PassoPedra) == "passo_pedra");
    CHECK(jogo::nomeDoSom(jogo::Som::MenuConfirmar) == "menu_confirmar");
}

TEST_CASE("mixagem: perto é volume cheio, longe é silêncio") {
    CHECK(jogo::mixar({jogo::Som::Acerto, glm::vec2(1, 0)}, {0, 0}, 0).volume == doctest::Approx(1));
    CHECK(jogo::mixar({jogo::Som::Acerto, glm::vec2(9, 0)}, {0, 0}, 0).volume == doctest::Approx(0.5));
    CHECK(jogo::mixar({jogo::Som::Acerto, glm::vec2(20, 0)}, {0, 0}, 0).volume == doctest::Approx(0));
    const auto semOrigem = jogo::mixar({jogo::Som::Dano, std::nullopt}, {0, 0}, 0);
    CHECK(semOrigem.volume == doctest::Approx(1));
    CHECK(semOrigem.pan == doctest::Approx(0));
    // em cima do ouvinte: sem lado
    CHECK(jogo::mixar({jogo::Som::Acerto, glm::vec2(0, 0)}, {0, 0}, 0).pan == doctest::Approx(0));
}

TEST_CASE("mixagem: som à direita sai na direita") {
    // ângulo 0 olha para +X; a direita é +Z
    CHECK(jogo::mixar({jogo::Som::Comporta, glm::vec2(0, 3)}, {0, 0}, 0).pan == doctest::Approx(0.8));
    CHECK(jogo::mixar({jogo::Som::Comporta, glm::vec2(0, -3)}, {0, 0}, 0).pan == doctest::Approx(-0.8));
    CHECK(jogo::mixar({jogo::Som::Comporta, glm::vec2(3, 0)}, {0, 0}, 0).pan == doctest::Approx(0));
    // olhando para +Z (90°), a direita é −X
    CHECK(jogo::mixar({jogo::Som::Comporta, glm::vec2(-3, 0)}, {0, 0}, 1.5707963f).pan == doctest::Approx(0.8));
}

TEST_CASE("ganhos estéreo") {
    CHECK(jogo::ganhosEstereo(0).esquerda == 255);
    CHECK(jogo::ganhosEstereo(0).direita == 255);
    CHECK(jogo::ganhosEstereo(0.8f).esquerda == 51);
    CHECK(jogo::ganhosEstereo(0.8f).direita == 255);
    CHECK(jogo::ganhosEstereo(-0.8f).esquerda == 255);
    CHECK(jogo::ganhosEstereo(-0.8f).direita == 51);
}

TEST_CASE("cada som tem arquivo em assets/sons") {
    const std::filesystem::path pasta = std::filesystem::path(PASTA_ASSETS) / "sons";
    for (int i = 0; i < static_cast<int>(jogo::Som::TOTAL); ++i) {
        const std::string nome(jogo::nomeDoSom(static_cast<jogo::Som>(i)));
        INFO(nome);
        CHECK((std::filesystem::exists(pasta / (nome + ".wav")) || std::filesystem::exists(pasta / (nome + ".ogg"))));
    }
    CHECK((std::filesystem::exists(pasta / "musica.wav") || std::filesystem::exists(pasta / "musica.ogg")));
}
