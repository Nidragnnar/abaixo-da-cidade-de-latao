#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <ostream>  // o doctest precisa para mostrar string
#include <random>
#include <share.h>
#include <stdio.h>
#include <string>

#include "jogo/Placar.h"

namespace {

// Pasta temporária própria de cada caso, apagada no fim
struct PastaTemporaria {
    std::filesystem::path caminho;
    PastaTemporaria() {
        caminho = std::filesystem::temp_directory_path() / ("roguelike_teste_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(caminho);
    }
    ~PastaTemporaria() {
        std::error_code e;
        std::filesystem::remove_all(caminho, e);
    }
};

jogo::RegistroPlacar registro(int pontuacao) { return {pontuacao, 1, 1, 0, 0, 1u, "x"}; }

}

TEST_CASE("pontuação da spec") { CHECK(jogo::calcularPontuacao(3, 12, 40, 2) == 300 + 120 + 40 + 100); }

TEST_CASE("placar guarda os 10 melhores em ordem") {
    std::vector<jogo::RegistroPlacar> p;
    int pos = 0;
    for (int i = 1; i <= 12; ++i) p = jogo::inserirNoPlacar(p, registro(i * 100), &pos);
    REQUIRE(p.size() == 10);
    CHECK(p.front().pontuacao == 1200);
    CHECK(p.back().pontuacao == 300);
    p = jogo::inserirNoPlacar(p, registro(50), &pos);
    CHECK(pos == 0);
    p = jogo::inserirNoPlacar(p, registro(1000), &pos);
    CHECK(pos == 4);  // empate com o 1000 antigo (3º): o antigo fica na frente
}

TEST_CASE("gravar e ler de volta, com acentos e ponto e vírgula na causa") {
    PastaTemporaria pasta;
    std::vector<jogo::RegistroPlacar> p{{560, 3, 1, 12, 0, 4242u, "Destruído por um Colosso; a vapor"}};
    REQUIRE(jogo::gravarPlacar(pasta.caminho / "placar.txt", p));
    auto lido = jogo::lerPlacar(pasta.caminho / "placar.txt");
    REQUIRE(lido.registros.size() == 1);
    CHECK(lido.registros[0].pontuacao == 560);
    CHECK(lido.registros[0].semente == 4242u);
    CHECK(lido.registros[0].causa == "Destruído por um Colosso  a vapor");
}

TEST_CASE("placar ausente é vazio") {
    PastaTemporaria pasta;
    auto lido = jogo::lerPlacar(pasta.caminho / "nao_existe.txt");
    CHECK(lido.registros.empty());
    CHECK_FALSE(lido.estavaCorrompido);
}

TEST_CASE("placar corrompido vira .corrompido") {
    PastaTemporaria pasta;
    {
        std::ofstream f(pasta.caminho / "placar.txt");
        f << "isto não é um placar\n";
    }
    auto lido = jogo::lerPlacar(pasta.caminho / "placar.txt");
    CHECK(lido.estavaCorrompido);
    CHECK(lido.registros.empty());
    CHECK(std::filesystem::exists(pasta.caminho / "placar.txt.corrompido"));
    CHECK_FALSE(std::filesystem::exists(pasta.caminho / "placar.txt"));
}

TEST_CASE("gravar cria a pasta que falta") {
    PastaTemporaria pasta;
    CHECK(jogo::gravarPlacar(pasta.caminho / "sub" / "placar.txt", {registro(10)}));
    CHECK(jogo::lerPlacar(pasta.caminho / "sub" / "placar.txt").registros.size() == 1);
}

TEST_CASE("gravar em lugar impossível devolve falso sem lançar") {
    CHECK_FALSE(jogo::gravarPlacar("Z:\\<invalido>\\placar.txt", {}));
}

TEST_CASE("placar salvo com BOM pelo Bloco de Notas ainda é lido") {
    PastaTemporaria pasta;
    {
        std::ofstream f(pasta.caminho / "placar.txt", std::ios::binary);
        f << "\xEF\xBB\xBF" << "560;3;1;12;0;4242;Destruído por um Rato mecânico\n";
    }
    auto lido = jogo::lerPlacar(pasta.caminho / "placar.txt");
    CHECK_FALSE(lido.estavaCorrompido);
    REQUIRE(lido.registros.size() == 1);
    CHECK(lido.registros[0].pontuacao == 560);
}

TEST_CASE("placar bloqueado por outro programa não é tratado como vazio") {
    PastaTemporaria pasta;
    REQUIRE(jogo::gravarPlacar(pasta.caminho / "placar.txt", {registro(900)}));
    // Outro programa segura o arquivo sem compartilhar a leitura
    FILE* bloqueio = _wfsopen((pasta.caminho / "placar.txt").c_str(), L"r", _SH_DENYRW);
    REQUIRE(bloqueio != nullptr);
    auto lido = jogo::lerPlacar(pasta.caminho / "placar.txt");
    fclose(bloqueio);
    CHECK(lido.erroDeLeitura);
    CHECK_FALSE(lido.estavaCorrompido);
    CHECK(jogo::lerPlacar(pasta.caminho / "placar.txt").registros.size() == 1);  // nada se perdeu
}

TEST_CASE("corrompido informa se renomeou") {
    PastaTemporaria pasta;
    {
        std::ofstream f(pasta.caminho / "placar.txt");
        f << "lixo\n";
    }
    CHECK(jogo::lerPlacar(pasta.caminho / "placar.txt").renomeado);
}

TEST_CASE("pontuação ganha 500 por chefe") { CHECK(jogo::calcularPontuacao(3, 12, 40, 2, 2) == 300 + 120 + 40 + 100 + 1000); }

TEST_CASE("placar grava e lê a dificuldade") {
    PastaTemporaria pasta;
    std::vector<jogo::RegistroPlacar> p{{900, 4, 2, 9, 5, 7u, "Morto", jogo::Dificuldade::Pesadelo},
                                        {800, 3, 2, 9, 5, 7u, "Morto"}};
    REQUIRE(jogo::gravarPlacar(pasta.caminho / "placar.txt", p));
    auto lido = jogo::lerPlacar(pasta.caminho / "placar.txt");
    REQUIRE(lido.registros.size() == 2);
    CHECK(lido.registros[0].dificuldade == jogo::Dificuldade::Pesadelo);
    CHECK(lido.registros[1].dificuldade == jogo::Dificuldade::Normal);
    CHECK(lido.registros[0].causa == "Morto");
}

TEST_CASE("placar antigo vale Normal") {
    PastaTemporaria pasta;
    {
        std::ofstream f(pasta.caminho / "placar.txt");
        f << "500;2;1;3;10;7;Morto por Rato\n";
    }
    auto lido = jogo::lerPlacar(pasta.caminho / "placar.txt");
    CHECK_FALSE(lido.estavaCorrompido);
    REQUIRE(lido.registros.size() == 1);
    CHECK(lido.registros[0].dificuldade == jogo::Dificuldade::Normal);
    CHECK(lido.registros[0].causa == "Morto por Rato");
    CHECK(lido.registros[0].semente == 7u);
}

TEST_CASE("placar com letra desconhecida é corrompido") {
    PastaTemporaria pasta;
    {
        std::ofstream f(pasta.caminho / "placar.txt");
        f << "500;2;1;3;10;7;X;Morto por Rato\n";
    }
    CHECK(jogo::lerPlacar(pasta.caminho / "placar.txt").estavaCorrompido);
}
