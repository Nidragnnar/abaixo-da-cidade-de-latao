#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string_view

#include "jogo/Partida.h"
#include "mundo/Gerador.h"

namespace {

void passos(jogo::Partida& p, int n, jogo::Comandos c = {}) {
    for (int i = 0; i < n; ++i) {
        p.atualizar(c, 1.0f / 60);
        c.usar = false;
        c.giro = 0;
    }
}

jogo::Comandos apertarE() {
    jogo::Comandos c;
    c.usar = true;
    return c;
}

}

TEST_CASE("E abre a comporta à frente em 0,5 s") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@C.#", "#####"}, 1), 1);
    passos(p, 15, apertarE());  // 0,25 s
    CHECK(p.aberturaComporta(2, 1) == doctest::Approx(0.5f).epsilon(0.05));
    CHECK(p.andar().grade.solida(2, 1));  // ainda fechada para passar
    passos(p, 16);
    CHECK(p.andar().grade.em(2, 1) == mundo::Celula::ComportaAberta);
    passos(p, 60, {1, 0, 0});
    CHECK(p.jogador().posicao.x > 3.0f);  // atravessou
}

TEST_CASE("comporta trancada sem chave não abre e avisa") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@T.#", "#####"}, 2), 1);
    passos(p, 60, apertarE());
    CHECK(p.andar().grade.em(2, 1) == mundo::Celula::ComportaTrancada);
    CHECK(p.mensagem() == "Comporta trancada: encontre a válvula-chave");
}

TEST_CASE("passar pela chave pega a válvula-chave") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@.K..#", "#######"}, 2), 1);
    passos(p, 60, {1, 0, 0});
    CHECK(p.temChave());
    CHECK_FALSE(p.andar().chave.has_value());
    CHECK(p.mensagem() == "Você pegou a válvula-chave");
}

TEST_CASE("com a chave, E destranca e abre") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@K.T.#", "#######"}, 2), 1);
    passos(p, 40, {1, 0, 0});  // anda até encostar
    passos(p, 1, apertarE());
    CHECK_FALSE(p.temChave());
    CHECK(p.mensagem() == "Comporta destrancada");
    passos(p, 40);
    CHECK(p.andar().grade.em(4, 1) == mundo::Celula::ComportaAberta);
}

TEST_CASE("E no elevador desce para o próximo andar") {
    jogo::Partida p(mundo::andarDeTexto({"####", "#@E#", "####"}, 1), 99);
    passos(p, 1, apertarE());
    CHECK(p.andar().numero == 2);
    CHECK(p.trocasDeAndar() == 1);
    CHECK(p.jogador().posicao == p.andar().inicio);
    CHECK(p.andar().grade.largura() == 64);  // veio do gerador
    CHECK(p.mensagem() == "Andar 2");
}

TEST_CASE("jogador não entra na célula do elevador (a gaiola cobriria a tela)") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@..E#", "######"}, 1), 1);
    passos(p, 120, {1, 0, 0});
    CHECK(p.jogador().posicao.x < 4.0f - 0.2f);
    passos(p, 1, apertarE());  // e ainda dá para usar de frente
    CHECK(p.andar().numero == 2);
}

TEST_CASE("usar duas vezes no mesmo passo de troca não desce dois andares") {
    jogo::Partida p(mundo::andarDeTexto({"####", "#@E#", "####"}, 1), 99);
    passos(p, 1, apertarE());
    passos(p, 1, apertarE());  // novo andar: o início fica longe do elevador
    CHECK(p.andar().numero == 2);
}

TEST_CASE("exploração marca as células ao redor depois de um passo") {
    jogo::Partida p(mundo::andarDeTexto({"####################", "#@.................#", "####################"}, 1), 1);
    passos(p, 1);
    CHECK(p.exploracao().vista(4, 1));
    CHECK_FALSE(p.exploracao().vista(15, 1));
}

TEST_CASE("mensagem some depois de 2 s") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@T.#", "#####"}, 2), 1);
    passos(p, 1, apertarE());
    passos(p, 121);
    CHECK(p.mensagem().empty());
}

TEST_CASE("partida pela semente começa no andar pedido") {
    jogo::Partida p(42, 3);
    CHECK(p.andar().numero == 3);
    CHECK(p.andar().zona == mundo::Zona::Fabrica);
    CHECK(p.jogador().posicao == p.andar().inicio);
}

TEST_CASE("partida pela semente tem inimigos") {
    jogo::Partida p(42, 2);
    CHECK(p.inimigos().size() >= 8);
    CHECK(p.vida() == 30);
}

TEST_CASE("dano recebido desconta a defesa") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@a.#", "#####"}, 1), 1);
    passos(p, 50);  // autômato chega a 0,9, prepara (0,5 s) e acerta 4–6 → 3–5; a recuperação (0,7 s) evita o 2º golpe
    CHECK(p.vida() <= 30 - 3);
    CHECK(p.vida() >= 30 - 5);
    CHECK(p.tempoDesdeDano() < 0.5f);
}

TEST_CASE("morte registra uma vez e congela a partida") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@cc..#", "#######"}, 5), 1);
    passos(p, 60 * 30);
    CHECK(p.morto());
    CHECK(p.causaDaMorte() == "Destruído por um Colosso a vapor");
    auto pos = p.jogador().posicao;
    passos(p, 60, {1, 0, 0});
    CHECK(p.jogador().posicao == pos);
    CHECK(p.vida() <= 0);
}

TEST_CASE("invencível não morre") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@cc..#", "#######"}, 5), 1);
    p.tornarInvencivel();
    passos(p, 60 * 10);
    CHECK_FALSE(p.morto());
}

TEST_CASE("disparo faz clarão e conta destruídos") {
    jogo::Partida p(mundo::andarDeTexto({"#########", "#@....r.#", "#########"}, 1), 1);
    jogo::Comandos troca;
    troca.trocarArma = true;
    passos(p, 1, troca);
    jogo::Comandos tiro;
    tiro.atacar = true;
    p.atualizar(tiro, 1.0f / 60);
    CHECK(p.clarao() > 0.0f);
    CHECK(p.armas().carga == 5);
    for (int i = 0; i < 60; ++i) p.atualizar(tiro, 1.0f / 60);  // segura o gatilho: o rato cai com 1 ou 2 tiros
    CHECK(p.inimigosDestruidos() == 1);
    CHECK(p.pontuacao() == 100 * 1 + 10 * 1 + 0 + 50 * 1);
}

TEST_CASE("descer limpa inimigos e projéteis e mantém vida e armas") {
    jogo::Partida p(mundo::andarDeTexto({"#########", "#@E....g#", "#########"}, 1), 7);
    passos(p, 90);  // o atirador já atirou
    int vida = p.vida();
    int carga = p.armas().carga;
    passos(p, 1, apertarE());
    CHECK(p.andar().numero == 2);
    CHECK(p.projeteis().empty());
    CHECK(p.inimigos().size() >= 8);  // população nova do andar 2
    CHECK(p.vida() == vida);
    CHECK(p.armas().carga == carga);
}

TEST_CASE("destroços somem depois de 10 s") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@r.#", "#####"}, 1), 1);
    jogo::Comandos golpe;
    golpe.atacar = true;
    for (int i = 0; i < 60; ++i) p.atualizar(golpe, 1.0f / 60);
    REQUIRE(p.inimigosDestruidos() == 1);
    passos(p, 60 * 11);
    CHECK(p.inimigos().empty());
}
