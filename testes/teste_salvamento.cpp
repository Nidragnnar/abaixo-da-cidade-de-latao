#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <ostream>  // o doctest precisa para mostrar string
#include <random>
#include <string>
#include <variant>

#include "jogo/Partida.h"
#include "jogo/Salvamento.h"
#include "mundo/Gerador.h"

namespace {

void passos(jogo::Partida& p, int n, jogo::Comandos c = {}) {
    for (int i = 0; i < n; ++i) {
        p.atualizar(c, 1.0f / 60);
        c.usar = c.trocarArma = false;
    }
}

bool mesmoItem(const jogo::Item& a, const jogo::Item& b) {
    if (a.tipo != b.tipo || a.qualidade != b.qualidade || a.quantidade != b.quantidade || a.carga != b.carga ||
        a.bonus.size() != b.bonus.size())
        return false;
    for (size_t k = 0; k < a.bonus.size(); ++k)
        if (a.bonus[k].tipo != b.bonus[k].tipo || a.bonus[k].valor != b.bonus[k].valor) return false;
    return true;
}

// Compara tudo o que o salvamento promete guardar
void conferirIgual(const jogo::Partida& a, const jogo::Partida& b) {
    CHECK(a.semente() == b.semente());
    CHECK(a.andar().numero == b.andar().numero);
    CHECK(a.jogador().posicao == b.jogador().posicao);
    CHECK(a.jogador().angulo == b.jogador().angulo);
    CHECK(a.jogador().inclinacao == b.jogador().inclinacao);
    CHECK(a.jogador().corpo.pes == b.jogador().corpo.pes);
    CHECK(a.vida() == b.vida());
    CHECK(a.nivel() == b.nivel());
    CHECK(a.xp() == b.xp());
    CHECK(a.engrenagens() == b.engrenagens());
    CHECK(a.inimigosDestruidos() == b.inimigosDestruidos());
    CHECK(a.chefesDerrotados() == b.chefesDerrotados());
    CHECK(a.trocasDeAndar() == b.trocasDeAndar());
    CHECK(a.tempoDeJogo() == b.tempoDeJogo());
    CHECK(a.temChave() == b.temChave());
    CHECK(a.lutaComChefe() == b.lutaComChefe());
    CHECK(a.andar().chave.has_value() == b.andar().chave.has_value());
    // Inventário, equipamento e armas
    for (int k = 0; k < jogo::ESPACOS_INVENTARIO; ++k) {
        INFO("espaço " << k);
        REQUIRE(a.inventario().espaco(k).has_value() == b.inventario().espaco(k).has_value());
        if (a.inventario().espaco(k)) CHECK(mesmoItem(*a.inventario().espaco(k), *b.inventario().espaco(k)));
    }
    CHECK(mesmoItem(a.inventario().armaBranca(), b.inventario().armaBranca()));
    CHECK(mesmoItem(a.inventario().armaDeFogo(), b.inventario().armaDeFogo()));
    CHECK(mesmoItem(a.inventario().armadura(), b.inventario().armadura()));
    CHECK(a.armas().ativa == b.armas().ativa);
    for (auto t : {jogo::TipoMunicao::Balas, jogo::TipoMunicao::Cartuchos, jogo::TipoMunicao::Celulas})
        CHECK(a.armas().municao.de(t) == b.armas().municao.de(t));
    // Andar: grade e exploração
    const auto& ga = a.andar().grade;
    const auto& gb = b.andar().grade;
    for (int y = 0; y < ga.altura(); ++y)
        for (int x = 0; x < ga.largura(); ++x) {
            REQUIRE(ga.em(x, y) == gb.em(x, y));
            REQUIRE(a.exploracao().vista(x, y) == b.exploracao().vista(x, y));
        }
    // Inimigos e chão
    REQUIRE(a.inimigos().size() == b.inimigos().size());
    for (size_t k = 0; k < a.inimigos().size(); ++k) {
        const auto& i = a.inimigos()[k];
        const auto& j = b.inimigos()[k];
        CHECK(i.tipo == j.tipo);
        CHECK(i.posicao == j.posicao);
        CHECK(i.corpo.pes == j.corpo.pes);
        CHECK(i.vida == j.vida);
        CHECK(i.vidaMaxima == j.vidaMaxima);
        CHECK(i.recompensado == j.recompensado);
        const bool destruido = i.estado == jogo::EstadoInimigo::Destruido;
        CHECK((j.estado == jogo::EstadoInimigo::Destruido) == destruido);
    }
    REQUIRE(a.chao().size() == b.chao().size());
    for (size_t k = 0; k < a.chao().size(); ++k) {
        CHECK(a.chao()[k].posicao == b.chao()[k].posicao);
        CHECK(a.chao()[k].conteudo.index() == b.chao()[k].conteudo.index());
    }
}

}

TEST_CASE("ida e volta dá a mesma partida") {
    // Andar com válvula-chave: o jogador a pega
    std::optional<jogo::Partida> p;
    for (uint32_t s = 1; s <= 60 && !p; ++s) {
        jogo::Partida q(s, 3);
        if (q.andar().chave) p.emplace(std::move(q));
    }
    REQUIRE(p);
    p->tornarInvencivel();
    p->definirPosicao(*p->andar().chave);
    passos(*p, 2);
    REQUIRE(p->temChave());
    // Obra-prima na frente, pega com E; um tiro gasta uma bala do tambor
    std::mt19937 rng(9);
    p->definirAngulo(0.0f);
    p->colocarNoChao({p->jogador().posicao + glm::vec2(0.5f, 0.0f), jogo::gerarObraPrima(4, rng)});
    jogo::Comandos e;
    e.usar = true;
    passos(*p, 1, e);
    jogo::Comandos troca;
    troca.trocarArma = true;
    passos(*p, 1, troca);
    jogo::Comandos tiro;
    tiro.atacar = true;
    passos(*p, 1, tiro);
    p->ganharXp(30);
    p->ferirParaTeste(5);
    passos(*p, 30, jogo::Comandos{0.5f});  // anda um pouco (exploração)
    const auto texto = jogo::escreverPartida(*p);
    REQUIRE(texto);
    const auto lida = jogo::lerPartida(*texto);
    REQUIRE(lida);
    conferirIgual(*p, *lida);
}

TEST_CASE("arena trancada volta trancada") {
    jogo::Partida p(1, 2);  // arena da Rainha: sala (34, 51) 9×10
    p.tornarInvencivel();
    p.definirPosicao({38.5f, 53.5f});
    passos(p, 1);
    REQUIRE(p.lutaComChefe());
    const auto lida = jogo::lerPartida(*jogo::escreverPartida(p));
    REQUIRE(lida);
    CHECK(lida->lutaComChefe());
    REQUIRE(lida->chefe() != nullptr);
    CHECK(lida->chefe()->estado != jogo::EstadoInimigo::Parado);
    conferirIgual(p, *lida);
}

TEST_CASE("morto ou vencido não grava") {
    jogo::Partida p(5, 1);
    p.ferirParaTeste(999);
    // Morto de verdade: um rato encostado mata (ferirParaTeste não marca a morte)
    auto a = mundo::andarDeTexto({"#####", "#@cc#", "#####"}, 5);
    jogo::Partida morta(std::move(a), 1);
    for (int k = 0; k < 600 && !morta.morto(); ++k) morta.atualizar({}, 1.0f / 60);
    REQUIRE(morta.morto());
    CHECK_FALSE(jogo::escreverPartida(morta));
}

TEST_CASE("contagem da vitória não grava") {
    jogo::Partida p(1, 6);
    p.tornarInvencivel();
    p.destruirChefeParaTeste();
    passos(p, 1);  // a Grande Máquina caiu: a vitória está contando
    CHECK_FALSE(jogo::escreverPartida(p));
}

TEST_CASE("números sem depender do idioma") {
    jogo::Partida p(7, 1);
    p.definirPosicao({12.375f, 7.125f});
    p.definirAngulo(-1.5f);
    const auto lida = jogo::lerPartida(*jogo::escreverPartida(p));
    REQUIRE(lida);
    CHECK(lida->jogador().posicao == glm::vec2(12.375f, 7.125f));
    CHECK(lida->jogador().angulo == -1.5f);
}

TEST_CASE("arquivo estragado não trava") {
    CHECK_FALSE(jogo::lerPartida(""));
    CHECK_FALSE(jogo::lerPartida("versao 2\npartida 1 1\nfim\n"));
    CHECK_FALSE(jogo::lerPartida("lixo\n"));
    jogo::Partida p(3, 2);
    const std::string bom = *jogo::escreverPartida(p);
    CHECK(jogo::lerPartida(bom));
    CHECK_FALSE(jogo::lerPartida(bom.substr(0, bom.size() / 2)));             // cortado no meio
    CHECK_FALSE(jogo::lerPartida(bom + "inimigo 99 x y z\n"));                 // depois do fim
    std::string trocado = bom;
    trocado.replace(trocado.find("jogador"), 7, "jogadxr");
    CHECK_FALSE(jogo::lerPartida(trocado));
    CHECK_FALSE(jogo::lerPartida(std::string(1 << 20, 'x')));                  // 1 MB de lixo
}

namespace {
// Pasta temporária só deste teste (nunca a pasta de dados real)
struct PastaTemporaria {
    std::filesystem::path caminho;
    PastaTemporaria() {
        caminho = std::filesystem::temp_directory_path() / ("roguelike_teste_sav_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(caminho);
    }
    ~PastaTemporaria() {
        std::error_code e;
        std::filesystem::remove_all(caminho, e);
    }
};
}

TEST_CASE("gravar e continuar apaga o arquivo") {
    PastaTemporaria pasta;
    const auto arquivo = pasta.caminho / "partida.sav";
    jogo::Partida p(11, 2);
    p.definirPosicao(p.jogador().posicao + glm::vec2(0.25f, 0.0f));
    REQUIRE(jogo::gravarPartida(arquivo, p));
    CHECK(jogo::existePartidaSalva(arquivo));
    auto [resultado, lida] = jogo::continuarPartida(arquivo);
    CHECK(resultado == jogo::ResultadoCarga::Carregou);
    REQUIRE(lida);
    CHECK(lida->jogador().posicao == p.jogador().posicao);
    CHECK_FALSE(jogo::existePartidaSalva(arquivo));
}

TEST_CASE("continuar sem arquivo dá Nenhum") {
    PastaTemporaria pasta;
    auto [resultado, lida] = jogo::continuarPartida(pasta.caminho / "partida.sav");
    CHECK(resultado == jogo::ResultadoCarga::Nenhum);
    CHECK_FALSE(lida);
}

TEST_CASE("arquivo estragado vira .corrompido e dá Estragado") {
    PastaTemporaria pasta;
    const auto arquivo = pasta.caminho / "partida.sav";
    { std::ofstream(arquivo) << "versao 1\nlixo\n"; }
    auto [resultado, lida] = jogo::continuarPartida(arquivo);
    CHECK(resultado == jogo::ResultadoCarga::Estragado);
    CHECK_FALSE(lida);
    CHECK_FALSE(std::filesystem::exists(arquivo));
    CHECK(std::filesystem::exists(pasta.caminho / "partida.sav.corrompido"));
}

TEST_CASE("gravar morto não cria arquivo") {
    PastaTemporaria pasta;
    const auto arquivo = pasta.caminho / "partida.sav";
    auto a = mundo::andarDeTexto({"#####", "#@cc#", "#####"}, 5);
    jogo::Partida morta(std::move(a), 1);
    for (int k = 0; k < 600 && !morta.morto(); ++k) morta.atualizar({}, 1.0f / 60);
    REQUIRE(morta.morto());
    CHECK_FALSE(jogo::gravarPartida(arquivo, morta));
    CHECK_FALSE(std::filesystem::exists(arquivo));
}

TEST_CASE("cenários de teste nunca mexem no salvamento do jogador") {
    // jogo normal (menu): pode
    CHECK(jogo::podeUsarSalvamento(false, false, false, false));
    // demo e captura (automático), invencível: nunca
    CHECK_FALSE(jogo::podeUsarSalvamento(true, false, false, true));
    CHECK_FALSE(jogo::podeUsarSalvamento(false, true, false, false));
    // cenário que abre direto na partida (--semente, --andar, --posicao...): só com pasta de dados de teste
    CHECK_FALSE(jogo::podeUsarSalvamento(false, false, true, false));
    CHECK(jogo::podeUsarSalvamento(false, false, true, true));
}
