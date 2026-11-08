#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <ostream>  // o doctest precisa para mostrar string
#include <random>
#include <share.h>
#include <stdio.h>
#include <string>

#include <iterator>

#include "jogo/Config.h"

namespace {

// Pasta temporária própria de cada caso, apagada no fim
struct PastaTemporaria {
    std::filesystem::path caminho;
    PastaTemporaria() {
        caminho = std::filesystem::temp_directory_path() / ("roguelike_config_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(caminho);
    }
    ~PastaTemporaria() {
        std::error_code e;
        std::filesystem::remove_all(caminho, e);
    }
    std::filesystem::path config() const { return caminho / "config.txt"; }
    void escrever(const std::string& texto) const {
        std::ofstream f(config(), std::ios::binary);
        f << texto;
    }
};

bool padrao(const jogo::Config& c) {
    const jogo::Config p;
    return c.sensibilidade == p.sensibilidade && c.volumeEfeitos == p.volumeEfeitos &&
           c.volumeMusica == p.volumeMusica && c.telaCheia == p.telaCheia && c.semente == p.semente;
}

}

TEST_CASE("config: ausente devolve o padrão sem erro") {
    PastaTemporaria pasta;
    auto l = jogo::lerConfig(pasta.config());
    CHECK(padrao(l.config));
    CHECK_FALSE(l.estavaCorrompido);
    CHECK_FALSE(l.erroDeLeitura);
    CHECK_FALSE(l.renomeado);
}

TEST_CASE("config: gravar e ler de volta") {
    PastaTemporaria pasta;
    jogo::Config c{9, 0, 10, true, 4294967295u};
    REQUIRE(jogo::gravarConfig(pasta.config(), c));
    auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(l.config.sensibilidade == 9);
    CHECK(l.config.volumeEfeitos == 0);
    CHECK(l.config.volumeMusica == 10);
    CHECK(l.config.telaCheia);
    CHECK(l.config.semente == 4294967295u);
}

TEST_CASE("config: semente vazia é aleatória") {
    PastaTemporaria pasta;
    REQUIRE(jogo::gravarConfig(pasta.config(), jogo::Config{}));
    auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.config.semente.has_value());
    CHECK_FALSE(l.estavaCorrompido);
}

TEST_CASE("config: chave desconhecida é ignorada e chave ausente fica no padrão") {
    PastaTemporaria pasta;
    pasta.escrever("# comentário\n\nvolume_musica=2\r\ncor=azul\n");
    auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(l.config.volumeMusica == 2);
    CHECK(l.config.sensibilidade == 5);
}

TEST_CASE("config: valor fora da faixa corrompe") {
    PastaTemporaria pasta;
    pasta.escrever("volume_musica=2\nsensibilidade=11\n");
    auto l = jogo::lerConfig(pasta.config());
    CHECK(l.estavaCorrompido);
    CHECK(l.renomeado);
    CHECK(padrao(l.config));
    CHECK(std::filesystem::exists(pasta.caminho / "config.txt.corrompido"));
    CHECK_FALSE(std::filesystem::exists(pasta.config()));
}

TEST_CASE("config: linha sem igual, número inválido e tela_cheia estranha corrompem") {
    for (const char* texto : {"sensibilidade 5\n", "volume_efeitos=8a\n", "tela_cheia=talvez\n", "semente=-1\n",
                              "semente=4294967296\n"}) {
        PastaTemporaria pasta;
        pasta.escrever(texto);
        INFO(texto);
        auto l = jogo::lerConfig(pasta.config());
        CHECK(l.estavaCorrompido);
        CHECK(padrao(l.config));
    }
}

TEST_CASE("config: BOM no começo é aceito") {
    PastaTemporaria pasta;
    pasta.escrever("\xEF\xBB\xBFvolume_efeitos=3\n");
    auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(l.config.volumeEfeitos == 3);
}

TEST_CASE("config: arquivo bloqueado não é sobrescrito") {
    PastaTemporaria pasta;
    REQUIRE(jogo::gravarConfig(pasta.config(), jogo::Config{3, 3, 3, false, 7u}));
    FILE* bloqueio = _wfsopen(pasta.config().c_str(), L"r", _SH_DENYRW);
    REQUIRE(bloqueio != nullptr);
    auto l = jogo::lerConfig(pasta.config());
    fclose(bloqueio);
    CHECK(l.erroDeLeitura);
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(jogo::lerConfig(pasta.config()).config.semente == 7u);  // nada se perdeu
}

TEST_CASE("sensibilidade em radianos por pixel") {
    CHECK(jogo::radianosPorPixel(5) == doctest::Approx(0.0025f));
    CHECK(jogo::radianosPorPixel(1) == doctest::Approx(0.001024f));
    CHECK(jogo::radianosPorPixel(10) == doctest::Approx(0.0076294f).epsilon(0.001));
}

TEST_CASE("config: espaços em volta da chave e do valor são aceitos") {
    PastaTemporaria pasta;
    pasta.escrever("sensibilidade = 7\nsemente= 5 \n  tela_cheia =sim\n");
    auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(l.config.sensibilidade == 7);
    CHECK(l.config.semente == 5u);
    CHECK(l.config.telaCheia);
}

TEST_CASE("gravação que falha não deixa o .tmp para trás") {
    PastaTemporaria pasta;
    std::filesystem::create_directories(pasta.config());  // uma pasta no lugar do arquivo: a troca falha
    CHECK_FALSE(jogo::gravarConfig(pasta.config(), jogo::Config{}));
    std::filesystem::path tmp = pasta.config();
    tmp += ".tmp";
    CHECK_FALSE(std::filesystem::exists(tmp));
}

TEST_CASE("config: dificuldade vai e volta") {
    PastaTemporaria pasta;
    CHECK(jogo::lerConfig(pasta.config()).config.dificuldade == jogo::Dificuldade::Normal);
    jogo::Config c;
    c.dificuldade = jogo::Dificuldade::Pesadelo;
    REQUIRE(jogo::gravarConfig(pasta.config(), c));
    const auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(l.config.dificuldade == jogo::Dificuldade::Pesadelo);
    pasta.escrever("sensibilidade=7\n");  // ausente: Normal
    CHECK(jogo::lerConfig(pasta.config()).config.dificuldade == jogo::Dificuldade::Normal);
}

TEST_CASE("config: dificuldade inválida corrompe") {
    PastaTemporaria pasta;
    pasta.escrever("dificuldade=medio\n");
    CHECK(jogo::lerConfig(pasta.config()).estavaCorrompido);
}

TEST_CASE("config: controle vai e volta") {
    PastaTemporaria pasta;
    jogo::Config c;
    CHECK(c.sensibilidadeControle == 5);
    CHECK_FALSE(c.inverterVertical);
    c.sensibilidadeControle = 8;
    c.inverterVertical = true;
    REQUIRE(jogo::gravarConfig(pasta.config(), c));
    const auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(l.config.sensibilidadeControle == 8);
    CHECK(l.config.inverterVertical);
    pasta.escrever("sensibilidade_controle=11\n");
    CHECK(jogo::lerConfig(pasta.config()).estavaCorrompido);
    pasta.escrever("inverter_vertical=2\n");
    CHECK(jogo::lerConfig(pasta.config()).estavaCorrompido);
}

TEST_CASE("config: teclas vão e voltam") {
    PastaTemporaria pasta;
    jogo::Config c;
    CHECK(c.teclas.tecla == jogo::Atalhos::padrao().tecla);
    c.teclas.definir(jogo::AcaoJogo::Pular, jogo::BOTAO_MOUSE + 4);
    REQUIRE(jogo::gravarConfig(pasta.config(), c));
    const auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(l.config.teclas.tecla == c.teclas.tecla);
    std::ifstream f(pasta.config());
    const std::string texto((std::istreambuf_iterator<char>(f)), {});
    CHECK(texto.find("tecla_pular=mouse_lateral1\n") != std::string::npos);
    CHECK(texto.find("tecla_atacar=mouse_esq\n") != std::string::npos);
}

TEST_CASE("config: tecla inválida volta ao padrão só dela") {
    PastaTemporaria pasta;
    pasta.escrever("sensibilidade=7\ntecla_pular=banana\ntecla_usar=f\ntecla_mapa=esc\ntecla_voar=x\n");
    const auto l = jogo::lerConfig(pasta.config());
    CHECK_FALSE(l.estavaCorrompido);
    CHECK(l.config.sensibilidade == 7);
    CHECK(l.config.teclas.de(jogo::AcaoJogo::Pular) == 44);
    CHECK(l.config.teclas.de(jogo::AcaoJogo::Usar) == 9);  // F
    CHECK(l.config.teclas.de(jogo::AcaoJogo::Mapa) == 43);
}

TEST_CASE("config: duas ações na mesma tecla") {
    PastaTemporaria pasta;
    pasta.escrever("tecla_usar=espaco\n");  // a do arquivo fica; a de fábrica que repete sai
    auto l = jogo::lerConfig(pasta.config());
    CHECK(l.config.teclas.de(jogo::AcaoJogo::Usar) == 44);
    CHECK(l.config.teclas.de(jogo::AcaoJogo::Pular) != 44);
    CHECK(jogo::teclaAceita(l.config.teclas.de(jogo::AcaoJogo::Pular)));
    pasta.escrever("tecla_pular=espaco\ntecla_usar=espaco\n");  // as duas no arquivo: a segunda volta ao padrão
    l = jogo::lerConfig(pasta.config());
    CHECK(l.config.teclas.de(jogo::AcaoJogo::Pular) == 44);
    CHECK(l.config.teclas.de(jogo::AcaoJogo::Usar) == 8);
}
