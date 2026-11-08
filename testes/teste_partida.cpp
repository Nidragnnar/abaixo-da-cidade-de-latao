#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string_view
#include <string>
#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

#include "jogo/IA.h"
#include "jogo/Partida.h"
#include "jogo/Placar.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

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
    CHECK(p.inventario().armaDeFogo().carga == 5);
    for (int i = 0; i < 60; ++i) p.atualizar(tiro, 1.0f / 60);  // segura o gatilho: o rato cai com 1 ou 2 tiros
    CHECK(p.inimigosDestruidos() == 1);
    CHECK(p.pontuacao() == 100 * 1 + 10 * 1 + 0 + 50 * 1);
}

TEST_CASE("descer limpa inimigos e projéteis e mantém vida e armas") {
    jogo::Partida p(mundo::andarDeTexto({"#########", "#@E....g#", "#########"}, 1), 7);
    passos(p, 90);  // o atirador já atirou
    int vida = p.vida();
    int carga = p.inventario().armaDeFogo().carga;
    passos(p, 1, apertarE());
    CHECK(p.andar().numero == 2);
    CHECK(p.projeteis().empty());
    CHECK(p.inimigos().size() >= 8);  // população nova do andar 2
    CHECK(p.vida() == vida);
    CHECK(p.inventario().armaDeFogo().carga == carga);
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

TEST_CASE("jogador não atravessa inimigo") {
    jogo::Partida p(mundo::andarDeTexto({"#########", "#@..a...#", "#########"}, 1), 1);
    p.tornarInvencivel();
    passos(p, 120, {1, 0, 0});
    CHECK(p.jogador().posicao.x <= p.inimigos()[0].posicao.x - (jogo::RAIO_JOGADOR + jogo::RAIO_INIMIGO) + 0.02f);
}

TEST_CASE("defesa vale para cada golpe do mesmo passo") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#r@r#", "#####"}, 1), 3);
    passos(p, 25);  // ~0,42 s: só a primeira dupla de mordidas
    // cada mordida 2–3 menos defesa 1 = 1–2; somado 2–4 (com a regra antiga, 3–5)
    CHECK(p.vida() >= 30 - 4);
    CHECK(p.vida() <= 30 - 2);
}

TEST_CASE("E pega o item à frente") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.colocarNoChao({{2.5f, 1.5f}, jogo::criarItem(jogo::TipoItem::Sabre)});
    passos(p, 1, apertarE());
    CHECK(p.inventario().espaco(1)->tipo == jogo::TipoItem::Sabre);
    CHECK(p.chao().empty());
    CHECK(p.mensagem() == "Você pegou: Sabre de oficial");
}

TEST_CASE("cheio: E avisa e o item fica") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    for (int i = 1; i < 16; ++i) p.colocarNoChao({{1.6f, 1.5f}, jogo::criarItem(jogo::TipoItem::Martelo)});
    for (int i = 1; i < 16; ++i) passos(p, 1, apertarE());
    p.colocarNoChao({{2.5f, 1.5f}, jogo::criarItem(jogo::TipoItem::Sabre)});
    passos(p, 1, apertarE());
    CHECK(p.mensagem() == "Inventário cheio");
    CHECK(p.chao().size() == 1);
}

TEST_CASE("munição acima do máximo deixa o resto") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.colocarNoChao({{2.5f, 1.5f}, jogo::MunicaoNoChao{jogo::TipoMunicao::Balas, 40}});
    passos(p, 30, {1, 0, 0});
    CHECK(p.armas().municao.balas == 48);
    REQUIRE(p.chao().size() == 1);
    CHECK(std::get<jogo::MunicaoNoChao>(p.chao()[0].conteudo).quantidade == 4);
}

TEST_CASE("engrenagens somam na pontuação") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.colocarNoChao({{2.5f, 1.5f}, 17});
    passos(p, 30, {1, 0, 0});
    CHECK(p.engrenagens() == 17);
    CHECK(p.pontuacao() == 100 + 17 + 50);
}

TEST_CASE("XP que passa de dois níveis") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1), 1);
    p.ganharXp(20 + 40 + 5);  // nível 1→2 (20) e 2→3 (40), sobram 5
    CHECK(p.nivel() == 3);
    CHECK(p.xp() == 5);
    CHECK(p.xpParaProximo() == 60);
    CHECK(p.vidaMaxima() == 30 + 16);
    CHECK(p.vida() == p.vidaMaxima());
    CHECK(p.mensagem() == "Nível 3!");
}

TEST_CASE("destruir dá XP") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@r.#", "#####"}, 1), 1);
    jogo::Comandos golpe;
    golpe.atacar = true;
    for (int i = 0; i < 60; ++i) p.atualizar(golpe, 1.0f / 60);
    CHECK(p.inimigosDestruidos() == 1);
    CHECK(p.xp() == 3);
}

TEST_CASE("tônico rápido cura e não gasta com vida cheia") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1), 1);
    jogo::Comandos um;
    um.usarRapido = 1;
    passos(p, 1, um);
    CHECK(p.mensagem() == "Vida cheia");
    CHECK(p.inventario().espaco(0)->quantidade == 2);
    p.ferirParaTeste(20);
    passos(p, 1, um);
    CHECK(p.vida() == 25);
    CHECK(p.inventario().espaco(0)->quantidade == 1);
}

TEST_CASE("largar põe à frente e pegar de volta") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.largarEspaco(0);
    REQUIRE(p.chao().size() == 1);
    CHECK_FALSE(p.inventario().espaco(0).has_value());
    passos(p, 1, apertarE());
    CHECK(p.inventario().espaco(0)->quantidade == 2);
}

TEST_CASE("equipar armadura com vida extra muda a vida máxima") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.colocarNoChao({{2.5f, 1.5f}, jogo::Item{jogo::TipoItem::Casaco, jogo::Qualidade::Modificada,
                                              {{jogo::TipoBonus::VidaMaxima, 7}}}});
    passos(p, 1, apertarE());
    p.usarEspaco(1);
    CHECK(p.vidaMaxima() == 37);
    CHECK(p.inventario().espaco(1)->tipo == jogo::TipoItem::Casaco);  // o casaco comum antigo
    CHECK(p.inventario().espaco(1)->bonus.empty());
}

TEST_CASE("chão do andar gerado pela semente") {
    jogo::Partida p(5, 2);
    CHECK(p.chao().size() >= 4 + 3 + 3);
}

TEST_CASE("inventário cheio não impede o E de abrir a comporta") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@C..#", "######"}, 1), 1);
    for (int i = 1; i < 16; ++i) p.colocarNoChao({{1.6f, 1.5f}, jogo::criarItem(jogo::TipoItem::Martelo)});
    for (int i = 1; i < 16; ++i) passos(p, 1, apertarE());
    p.colocarNoChao({{1.85f, 1.5f}, jogo::criarItem(jogo::TipoItem::Sabre)});  // caído na frente da comporta
    passos(p, 1, apertarE());
    passos(p, 10);
    CHECK(p.aberturaComporta(2, 1) > 0.0f);
}

TEST_CASE("equipar outra arma de fogo durante a recarga não cria nem perde balas") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    jogo::Comandos troca;
    troca.trocarArma = true;
    passos(p, 1, troca);  // revólver na mão
    jogo::Comandos tiro;
    tiro.atacar = true;
    passos(p, 1, tiro);  // carga 5
    jogo::Comandos recarregar;
    recarregar.recarregar = true;
    passos(p, 1, recarregar);
    REQUIRE(p.armas().recarga > 0.0f);
    p.colocarNoChao({{2.0f, 1.5f}, jogo::criarItem(jogo::TipoItem::Revolver)});
    passos(p, 1, apertarE());
    p.usarEspaco(1);  // equipa o revólver do chão no meio da recarga
    CHECK(p.armas().recarga == 0.0f);
    CHECK(p.inventario().armaDeFogo().carga == 6);       // o novo veio carregado
    CHECK(p.inventario().espaco(1)->carga == 5);         // o antigo guardou a carga dele
    CHECK(p.armas().municao.balas == 12);                // a reserva não mudou
    passos(p, 120);
    CHECK(p.armas().municao.balas == 12);
}

namespace {
int contar(const jogo::Partida& p, jogo::Som s) {
    int n = 0;
    for (const auto& e : p.sons()) n += e.som == s ? 1 : 0;
    return n;
}
}

TEST_CASE("andar faz passos de pedra no esgoto") {
    jogo::Partida p(mundo::andarDeTexto({"################", "#@.............#", "################"}, 1), 1);
    REQUIRE(p.andar().zona == mundo::Zona::Esgoto);
    int pedra = 0, metal = 0;
    for (int i = 0; i < 60; ++i) {
        p.atualizar({1, 0, 0}, 1.0f / 60);
        pedra += contar(p, jogo::Som::PassoPedra);
        metal += contar(p, jogo::Som::PassoMetal);
    }
    CHECK(pedra >= 1);
    CHECK(metal == 0);
}

TEST_CASE("disparo do bacamarte toca o som certo e acerto toca uma vez") {
    jogo::Partida p(mundo::andarDeTexto({"##########", "#@..c....#", "##########"}, 1), 1);
    p.colocarNoChao({p.jogador().posicao + glm::vec2(0.5f, 0), jogo::criarItem(jogo::TipoItem::Bacamarte)});
    passos(p, 1, apertarE());
    REQUIRE(p.inventario().espaco(1).has_value());
    p.usarEspaco(1);
    REQUIRE(p.inventario().armaDeFogo().tipo == jogo::TipoItem::Bacamarte);
    REQUIRE(p.inventario().armaDeFogo().carga > 0);
    jogo::Comandos troca;
    troca.trocarArma = true;
    passos(p, 1, troca);
    jogo::Comandos tiro;
    tiro.atacar = true;
    p.atualizar(tiro, 1.0f / 60);
    CHECK(contar(p, jogo::Som::DisparoBacamarte) == 1);
    CHECK(contar(p, jogo::Som::DisparoRevolver) == 0);
    CHECK(contar(p, jogo::Som::Acerto) == 1);
}

TEST_CASE("comporta abrindo toca o som com origem na comporta") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@C.#", "#####"}, 1), 1);
    passos(p, 1, apertarE());
    REQUIRE(contar(p, jogo::Som::Comporta) == 1);
    const auto& e = p.sons().front().som == jogo::Som::Comporta ? p.sons().front() : p.sons().back();
    REQUIRE(e.origem.has_value());
    CHECK(e.origem->x == doctest::Approx(2.5f));
    CHECK(e.origem->y == doctest::Approx(1.5f));
    passos(p, 1, apertarE());  // já está abrindo: não toca de novo
    CHECK(contar(p, jogo::Som::Comporta) == 0);
}

TEST_CASE("descer e subir de nível tocam elevador e nível") {
    jogo::Partida p(mundo::andarDeTexto({"####", "#@E#", "####"}, 1), 99);
    passos(p, 1, apertarE());
    CHECK(contar(p, jogo::Som::Elevador) == 1);
    p.ganharXp(20 + 40);  // dois níveis: um som só
    CHECK(contar(p, jogo::Som::Nivel) == 1);
}

TEST_CASE("sons valem só para o passo em que aconteceram") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@C.#", "#####"}, 1), 1);
    passos(p, 1, apertarE());
    CHECK_FALSE(p.sons().empty());
    passos(p, 1);
    CHECK(p.sons().empty());
}

TEST_CASE("dano e pegar tocam uma vez por passo") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@....#", "#######"}, 1), 1);
    p.colocarNoChao({p.jogador().posicao, 30});
    p.colocarNoChao({p.jogador().posicao, 40});
    passos(p, 1);
    CHECK(contar(p, jogo::Som::Pegar) == 1);
}

namespace {
jogo::Comandos frente(bool pular = false) {
    jogo::Comandos c;
    c.frente = 1;
    c.pular = pular;
    return c;
}
}

TEST_CASE("queda de 2 blocos sem dano, de 3 com 10") {
    // Começa em cima de uma plataforma e anda para fora da borda
    auto cair = [](const std::string& alturas) {
        jogo::Partida p(mundo::andarDeTexto({"#######", "#@....#", "#######"}, 1, {"0000000", alturas, "0000000"},
                                            {"0000000", "0kkkkk0", "0000000"}),
                        1);
        for (int i = 0; i < 120; ++i) p.atualizar(frente(), 1.0f / 60);
        return p;
    };
    CHECK(cair("0880000").vida() == 30);
    const auto p3 = cair("0cc0000");
    CHECK(p3.vida() == 20);
    CHECK(p3.jogador().corpo.noChao);
}

TEST_CASE("morte por queda tem causa própria e ignora a armadura") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@....#", "#######"}, 1, {"0000000", "0cc0000", "0000000"},
                                        {"0000000", "0kkkkk0", "0000000"}),
                    1);
    p.ferirParaTeste(25);  // sobra 5; a queda de 3 blocos tira 10 (defesa do casaco não conta)
    for (int i = 0; i < 120; ++i) p.atualizar(frente(), 1.0f / 60);
    CHECK(p.morto());
    CHECK(p.causaDaMorte() == "Caiu de muito alto");
}

TEST_CASE("pulo na quina sob teto baixo não prende") {
    // Parede ao norte, degrau de 1 bloco a leste, teto de 1,5 bloco
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@..#", "#...#", "#####"}, 1, {"00000", "00440", "00000", "00000"},
                                        {"00000", "06660", "06660", "00000"}),
                    1);
    p.definirAngulo(-0.6f);  // contra a quina (nordeste)
    for (int i = 0; i < 120; ++i) {
        p.atualizar(frente(i % 20 == 0), 1.0f / 60);
        const auto& j = p.jogador();
        REQUIRE(j.corpo.pes >= mundo::alturaDoChao(p.andar().grade, j.posicao) - 1e-3f);
        REQUIRE(j.corpo.pes + mundo::ALTURA_CORPO <= mundo::alturaDoTeto(p.andar().grade, j.posicao) + 1e-3f);
    }
    // Dá para sair andando para trás
    const glm::vec2 antes = p.jogador().posicao;
    jogo::Comandos tras;
    tras.frente = -1;
    for (int i = 0; i < 30; ++i) p.atualizar(tras, 1.0f / 60);
    CHECK(glm::distance(antes, p.jogador().posicao) > 0.5f);
}

TEST_CASE("descer no ar zera a queda") {
    jogo::Partida p(mundo::andarDeTexto({"####", "#@E#", "####"}, 1), 99);
    jogo::Comandos pulo;
    pulo.pular = true;
    p.atualizar(pulo, 1.0f / 60);
    for (int i = 0; i < 10; ++i) p.atualizar({}, 1.0f / 60);
    REQUIRE_FALSE(p.jogador().corpo.noChao);
    jogo::Comandos e;
    e.usar = true;
    p.atualizar(e, 1.0f / 60);
    REQUIRE(p.andar().numero == 2);
    CHECK(p.jogador().corpo.noChao);
    CHECK(p.jogador().corpo.velocidade == 0.0f);
    CHECK(p.jogador().corpo.pes == doctest::Approx(mundo::alturaDoChao(p.andar().grade, p.jogador().posicao)));
    for (int i = 0; i < 60; ++i) p.atualizar({}, 1.0f / 60);
    CHECK(p.vida() == 30);
}

TEST_CASE("item na plataforma só se pega lá em cima") {
    // Plataforma de 1 bloco nas colunas 3–4; o item está nela
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1, {"000000", "000440", "000000"},
                                        {"000000", "088880", "000000"}),
                    1);
    p.colocarNoChao({{3.5f, 1.5f}, jogo::criarItem(jogo::TipoItem::Sabre)});
    p.definirPosicao({2.5f, 1.5f});
    passos(p, 1, apertarE());
    CHECK_FALSE(p.inventario().espaco(1).has_value());  // de baixo, não pega
    p.definirPosicao({3.2f, 1.5f});
    passos(p, 2);  // os pés sobem para a plataforma
    REQUIRE(p.jogador().corpo.pes == doctest::Approx(1.0f));
    p.definirAngulo(0.0f);
    passos(p, 1, apertarE());
    CHECK(p.inventario().espaco(1).has_value());
}

TEST_CASE("item largado na plataforma fica no alto") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1, {"000000", "044400", "000000"},
                                        {"000000", "088880", "000000"}),
                    1);
    REQUIRE(p.jogador().corpo.pes == doctest::Approx(1.0f));
    p.largarEspaco(0);  // tônicos, à frente (ainda em cima da plataforma)
    REQUIRE(p.chao().size() == 1);
    CHECK(mundo::alturaDoChao(p.andar().grade, p.chao()[0].posicao) == doctest::Approx(1.0f));
    passos(p, 1, apertarE());  // e pega de volta lá de cima
    CHECK(p.chao().empty());
}

TEST_CASE("sem voador, campo voador vazio") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@..r.#", "#######"}, 1), 1);
    passos(p, 60);
    CHECK(p.campoVoador().empty());
}

TEST_CASE("com voador, campo voador calculado") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@...v#", "#######"}, 2), 1);
    passos(p, 60);
    CHECK(p.campoVoador().size() == 7u * 3u);
}

TEST_CASE("explosão de bomba fere o jogador, toca e aparece") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@..#", "#####"}, 3, {}, {"00000", "0ggg0", "00000"}), 1);
    p.acrescentarInimigo(jogo::TipoInimigo::Dirigivel, p.jogador().posicao);  // em cima do jogador
    const int antes = p.vida();
    bool tocou = false;
    for (int i = 0; i < 120 && p.explosoes().empty(); ++i) {
        p.atualizar({}, 1.0f / 60);
        for (const auto& s : p.sons()) tocou |= s.som == jogo::Som::Explosao;
    }
    REQUIRE(p.explosoes().size() == 1);
    CHECK(tocou);
    CHECK(p.vida() < antes);
    passos(p, static_cast<int>(jogo::DURACAO_EXPLOSAO * 60) + 1);
    CHECK(p.explosoes().empty());
}

TEST_CASE("acrescentarInimigo põe o inimigo no chão da célula") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 2, {"000000", "000400", "000000"}), 1);
    p.acrescentarInimigo(jogo::TipoInimigo::Vespa, {3.5f, 1.5f});
    REQUIRE(p.inimigos().size() == 1);
    CHECK(p.inimigos()[0].tipo == jogo::TipoInimigo::Vespa);
    CHECK(p.inimigos()[0].corpo.pes == doctest::Approx(1.0f));
    CHECK(p.inimigos()[0].vida == jogo::escalar(6, 2));
}

TEST_CASE("voador no alto não empurra o jogador") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#.....#", "#..@..#", "#.....#", "#######"}, 3, {},
                                        {"0000000", "0ggggg0", "0ggggg0", "0ggggg0", "0000000"}),
                    1);
    p.tornarInvencivel();
    p.acrescentarInimigo(jogo::TipoInimigo::Dirigivel, {3.5f, 1.5f});
    passos(p, 120);  // sobe até perto do teto e vem para cima do jogador
    const glm::vec2 antes = p.jogador().posicao;
    passos(p, 120);
    CHECK(glm::distance(p.jogador().posicao, antes) < 0.01f);
}

namespace {
// Corredor do início até a arena (sala x 5–8, y 1–4) pela comporta em (4, 1); chefe no centro, elevador no canto
jogo::Partida partidaComArena(char chefe, int andar) {
    auto a = mundo::andarDeTexto({"##########", "#@..C....#", "#####....#", std::string("#####..") + chefe + ".#",
                                  "#####...E#", "##########"},
                                 andar);
    a.salas = {mundo::Sala{1, 1, 3, 1}, mundo::Sala{5, 1, 4, 4}};
    a.arena = 1;
    return jogo::Partida(std::move(a), 1);
}
}

TEST_CASE("entrar na arena tranca e acorda o chefe") {
    auto p = partidaComArena('R', 2);
    REQUIRE(p.chefe() != nullptr);
    CHECK_FALSE(p.lutaComChefe());
    p.definirPosicao({6.5f, 2.5f});
    passos(p, 1);
    CHECK(p.lutaComChefe());
    CHECK(p.andar().grade.em(4, 1) == mundo::Celula::ComportaTrancada);
    CHECK(p.chefe()->estado != jogo::EstadoInimigo::Parado);
}

TEST_CASE("trancar não prende o jogador dentro da comporta") {
    auto p = partidaComArena('R', 2);
    p.definirPosicao({5.15f, 1.5f});  // centro dentro da sala, corpo ainda na comporta
    passos(p, 1);
    CHECK_FALSE(p.lutaComChefe());
    CHECK(p.andar().grade.em(4, 1) == mundo::Celula::Comporta);
    p.definirPosicao({5.6f, 1.5f});
    passos(p, 1);
    CHECK(p.lutaComChefe());
}

TEST_CASE("chefe destruído destranca e solta obra-prima e 30 engrenagens") {
    auto p = partidaComArena('R', 2);
    p.tornarInvencivel();
    p.definirPosicao({6.5f, 2.5f});
    passos(p, 1);
    p.destruirChefeParaTeste();
    passos(p, 1);
    CHECK_FALSE(p.lutaComChefe());
    CHECK(p.chefe() == nullptr);
    CHECK(p.chefesDerrotados() == 1);
    CHECK(p.andar().grade.em(4, 1) == mundo::Celula::Comporta);
    bool obraPrima = false, engrenagens = false;
    for (const auto& c : p.chao()) {
        if (const auto* i = std::get_if<jogo::Item>(&c.conteudo)) obraPrima |= i->qualidade == jogo::Qualidade::ObraPrima;
        if (const auto* n = std::get_if<int>(&c.conteudo)) engrenagens |= *n == 30;
    }
    CHECK(obraPrima);
    CHECK(engrenagens);
    CHECK_FALSE(p.venceu());  // a Rainha não termina o jogo
}

TEST_CASE("elevador com chefe vivo avisa 'Derrote o chefe primeiro'") {
    auto p = partidaComArena('R', 2);
    p.tornarInvencivel();
    p.definirPosicao({7.5f, 4.5f});
    passos(p, 1, apertarE());
    CHECK(p.mensagem() == "Derrote o chefe primeiro");
    CHECK(p.trocasDeAndar() == 0);
}

TEST_CASE("Grande Máquina destruída: vitória depois de 3 s, causa e +500") {
    auto p = partidaComArena('M', 6);
    p.tornarInvencivel();
    p.definirPosicao({5.6f, 1.5f});
    passos(p, 1);
    p.destruirChefeParaTeste();
    passos(p, 60);
    CHECK_FALSE(p.venceu());
    CHECK_FALSE(p.terminou());
    passos(p, 2 * 60 + 5);
    CHECK(p.venceu());
    CHECK(p.terminou());
    CHECK(p.causaDaMorte() == jogo::CAUSA_VITORIA);
    CHECK(p.pontuacao() ==
          jogo::calcularPontuacao(6, p.inimigosDestruidos(), p.engrenagens(), p.nivel(), 1));
    CHECK(p.tempoDeJogo() == doctest::Approx(1.0f / 60 * 186).epsilon(0.05));
}

TEST_CASE("morto não vence") {
    auto p = partidaComArena('M', 6);
    p.definirPosicao({5.6f, 1.5f});
    passos(p, 1);
    p.destruirChefeParaTeste();
    passos(p, 1);  // recompensa (o XP sobe de nível e cura) antes do ferimento
    p.ferirParaTeste(999);
    passos(p, 4 * 60);
    CHECK_FALSE(p.venceu());
    CHECK(p.causaDaMorte() != jogo::CAUSA_VITORIA);
}

TEST_CASE("ratos da ninhada entram no andar") {
    auto p = partidaComArena('R', 2);
    p.tornarInvencivel();
    p.definirPosicao({5.6f, 1.5f});
    passos(p, static_cast<int>(8.5f * 60));
    int ratos = 0;
    for (const auto& i : p.inimigos()) ratos += i.tipo == jogo::TipoInimigo::Rato;
    CHECK(ratos >= 1);
    CHECK(ratos <= 6);
}

TEST_CASE("definirVidaDoChefe muda só a vida do chefe vivo") {
    auto p = partidaComArena('M', 6);
    p.definirVidaDoChefe(15);
    REQUIRE(p.chefe() != nullptr);
    CHECK(p.chefe()->vida == 15);
    CHECK(p.chefe()->vidaMaxima == 420);
}

TEST_CASE("elevador do último andar não desce") {
    auto p = partidaComArena('M', 6);
    p.tornarInvencivel();
    p.definirPosicao({7.5f, 4.5f});  // de frente para o elevador, dentro da arena
    passos(p, 1);
    p.destruirChefeParaTeste();
    passos(p, 1);
    passos(p, 1, apertarE());
    CHECK(p.trocasDeAndar() == 0);
    CHECK(p.andar().numero == 6);
}

TEST_CASE("arena tranca todas as aberturas do anel, não só as entradas") {
    // Anel aberto à direita (x 9) e embaixo (y 5): corredor colado na parede da arena
    auto a = mundo::andarDeTexto({"###########", "#@..C.....#", "#####.....#", "#####..R..#", "#####...E.#", "#####.....#",
                                  "###########"},
                                 2);
    a.salas = {mundo::Sala{1, 1, 3, 1}, mundo::Sala{5, 1, 4, 4}};
    a.arena = 1;
    jogo::Partida p(std::move(a), 1);
    p.definirPosicao({6.5f, 2.5f});
    passos(p, 1);
    REQUIRE(p.lutaComChefe());
    for (int y = 0; y <= 5; ++y)
        for (int x = 4; x <= 9; ++x) {
            const bool anel = x == 4 || x == 9 || y == 0 || y == 5;
            if (!anel) continue;
            INFO("célula " << x << ", " << y);
            CHECK(p.andar().grade.solida(x, y));
        }
}

TEST_CASE("depois da Grande Máquina cair, nada mais fere o jogador") {
    // Ratos colados no jogador dentro da arena: continuam mordendo nos 3 s até a vitória
    auto a = mundo::andarDeTexto({"##########", "#@..C....#", "#####rr..#", "#####..M.#", "#####...E#", "##########"}, 6);
    a.salas = {mundo::Sala{1, 1, 3, 1}, mundo::Sala{5, 1, 4, 4}};
    a.arena = 1;
    jogo::Partida p(std::move(a), 1);
    p.definirPosicao({5.6f, 1.5f});
    passos(p, 1);
    p.destruirChefeParaTeste();
    passos(p, 1);
    const int vida = p.vida();
    passos(p, static_cast<int>(jogo::ESPERA_VITORIA * 60) + 5);
    CHECK(p.vida() == vida);
    CHECK(p.venceu());
}

TEST_CASE("inimigo na abertura não é empurrado: ela tranca quando ele sai") {
    auto a = mundo::andarDeTexto({"##########", "#@..r....#", "#####....#", "#####..R.#", "#####...E#", "##########"}, 2);
    a.salas = {mundo::Sala{1, 1, 3, 1}, mundo::Sala{5, 1, 4, 4}};
    a.arena = 1;
    jogo::Partida p(std::move(a), 1);
    p.tornarInvencivel();
    const glm::vec2 antes = p.inimigos()[0].posicao;  // rato na abertura (4, 1)
    p.definirPosicao({6.5f, 2.5f});
    passos(p, 1);
    REQUIRE(p.lutaComChefe());
    CHECK(glm::distance(p.inimigos()[0].posicao, antes) < 0.1f);  // não saltou para fora da abertura
    CHECK_FALSE(p.andar().grade.solida(4, 1));                    // ainda aberta: o rato está nela
    passos(p, 120);  // o rato entra atrás do jogador
    CHECK(p.andar().grade.em(4, 1) == mundo::Celula::ComportaTrancada);
}

TEST_CASE("partida guarda a dificuldade e multiplica a pontuação") {
    jogo::Partida n(5, 2), d(5, 2, jogo::Dificuldade::Dificil);
    CHECK(n.dificuldade() == jogo::Dificuldade::Normal);
    CHECK(d.dificuldade() == jogo::Dificuldade::Dificil);
    REQUIRE(d.inimigos().size() == n.inimigos().size());
    CHECK(d.inimigos()[0].vidaMaxima > n.inimigos()[0].vidaMaxima);
    // mesma partida, mesmos números: pontuação × 1,5
    n.ganharXp(30);
    d.ganharXp(30);
    CHECK(d.pontuacao() == static_cast<int>(std::lround(n.pontuacao() * 1.5)));
}

TEST_CASE("descer mantém a dificuldade") {
    jogo::Partida p(mundo::andarDeTexto({"####", "#@E#", "####"}, 1), 99, jogo::Dificuldade::Pesadelo);
    passos(p, 1, apertarE());
    REQUIRE(p.andar().numero == 2);
    const jogo::Partida normal(99, 2);
    REQUIRE(p.inimigos().size() == normal.inimigos().size());
    CHECK(p.inimigos()[0].vidaMaxima == std::max(1, static_cast<int>(std::lround(normal.inimigos()[0].vidaMaxima * 1.8))));
    CHECK(p.dificuldade() == jogo::Dificuldade::Pesadelo);
}
