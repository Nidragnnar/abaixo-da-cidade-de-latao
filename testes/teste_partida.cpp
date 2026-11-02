#include <doctest/doctest.h>

#include <ostream>  // o doctest precisa para mostrar string_view

#include "jogo/IA.h"
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
