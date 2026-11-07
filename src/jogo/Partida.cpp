#include "jogo/Partida.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>

#include <glm/geometric.hpp>

#include "jogo/Chefes.h"
#include "jogo/IA.h"
#include "jogo/Placar.h"
#include "mundo/Colisao.h"

namespace jogo {

namespace {

const char* nomeMunicao(TipoMunicao t, int quantidade) {
    switch (t) {
        case TipoMunicao::Cartuchos: return quantidade == 1 ? "cartucho" : "cartuchos";
        case TipoMunicao::Celulas: return quantidade == 1 ? "célula elétrica" : "células elétricas";
        default: return quantidade == 1 ? "bala" : "balas";
    }
}

}

Partida::Partida(uint32_t semente, int andarInicial) : Partida(mundo::gerarAndar(semente, andarInicial), semente) {
    inimigos_ = gerarInimigos(andar_, semente_);
    chao_ = gerarChao(andar_, semente_);
}

Partida::Partida(mundo::Andar andar, uint32_t semente)
    : semente_(semente),
      andar_(std::move(andar)),
      jogador_{andar_.inicio, 0.0f},
      exploracao_(andar_.grade.largura(), andar_.grade.altura()),
      rng_(semente ^ 0xC0FFEEu),
      inimigos_(inimigosDasMarcas(andar_)) {
    jogador_.corpo.pes = mundo::alturaDoChao(andar_.grade, andar_.inicio);
}

float Partida::aberturaComporta(int x, int y) const {
    if (andar_.grade.em(x, y) == mundo::Celula::ComportaAberta) return 1.0f;
    const auto it = abrindo_.find(indice(x, y));
    return it == abrindo_.end() ? 0.0f : it->second;
}

int Partida::pontuacao() const {
    return calcularPontuacao(andar_.numero, destruidos_, engrenagens_, nivel_, chefesDerrotados_);
}

const Inimigo* Partida::chefe() const {
    for (const Inimigo& i : inimigos_)
        if (dados(i.tipo).chefe && i.estado != EstadoInimigo::Destruido) return &i;
    return nullptr;
}

void Partida::definirVidaDoChefe(int vida) {
    for (Inimigo& i : inimigos_)
        if (dados(i.tipo).chefe && i.estado != EstadoInimigo::Destruido) i.vida = std::clamp(vida, 1, i.vidaMaxima);
}

void Partida::destruirChefeParaTeste() {
    for (Inimigo& i : inimigos_)
        if (dados(i.tipo).chefe && i.estado != EstadoInimigo::Destruido) {
            i.vida = 0;
            i.estado = EstadoInimigo::Destruido;
            i.tempoNoEstado = 0;
        }
}

// Tranca toda abertura no anel de parede da arena (entradas, corredores colados nela, aberturas largas; o que não era
// comporta ganha uma) ou as devolve como comportas comuns fechadas
void Partida::trancarArena(bool trancar) {
    if (trancar) {
        const mundo::Sala& s = andar_.salas[*andar_.arena];
        comportasArena_.clear();
        for (int y = s.y - 1; y <= s.y + s.altura; ++y)
            for (int x = s.x - 1; x <= s.x + s.largura; ++x) {
                const bool anel = x == s.x - 1 || x == s.x + s.largura || y == s.y - 1 || y == s.y + s.altura;
                const mundo::Celula c = andar_.grade.em(x, y);
                if (anel && c != mundo::Celula::Parede && c != mundo::Celula::Elevador) comportasArena_.push_back({x, y});
            }
    }
    lutando_ = trancar;
    if (trancar) {
        trancarLivres();
        return;
    }
    for (const glm::ivec2 c : comportasArena_) {
        abrindo_.erase(indice(c.x, c.y));
        andar_.grade.definir(c.x, c.y, mundo::Celula::Comporta);
    }
    comportasArena_.clear();
}

// Tranca as aberturas da arena que não têm inimigo dentro (quem estiver numa delas não é empurrado: ela tranca
// quando ele sair). Chamada a cada passo durante a luta.
void Partida::trancarLivres() {
    for (const glm::ivec2 c : comportasArena_) {
        if (andar_.grade.em(c.x, c.y) == mundo::Celula::ComportaTrancada) continue;
        const bool ocupada = std::ranges::any_of(inimigos_, [&](const Inimigo& i) {
            if (i.estado == EstadoInimigo::Destruido) return false;
            const float r = raioDe(i);
            return i.posicao.x + r > c.x && i.posicao.x - r < c.x + 1 && i.posicao.y + r > c.y && i.posicao.y - r < c.y + 1;
        });
        if (ocupada) continue;
        abrindo_.erase(indice(c.x, c.y));
        andar_.grade.definir(c.x, c.y, mundo::Celula::ComportaTrancada);
    }
}

void Partida::conferirArena() {
    if (lutando_ || !andar_.arena || *andar_.arena >= andar_.salas.size()) return;
    const Inimigo* c = chefe();
    if (!c) return;
    // O corpo inteiro dentro da sala: trancar não prende o jogador na comporta
    const mundo::Sala& s = andar_.salas[*andar_.arena];
    const glm::vec2 p = jogador_.posicao;
    const float r = RAIO_JOGADOR + 0.01f;
    if (p.x - r < s.x || p.y - r < s.y || p.x + r > s.x + s.largura || p.y + r > s.y + s.altura) return;
    trancarArena(true);
    for (Inimigo& i : inimigos_)
        if (&i == c && i.estado == EstadoInimigo::Parado) {
            i.estado = EstadoInimigo::Perseguindo;
            i.tempoNoEstado = 0;
        }
    avisar(std::format("{}!", dados(c->tipo).nome));
}

void Partida::avisar(std::string texto) {
    mensagem_ = std::move(texto);
    tempoMensagem_ = DURACAO_MENSAGEM;
}

void Partida::tocar(Som s, std::optional<glm::vec2> origem) {
    if (!origem && std::ranges::any_of(sons_, [&](const EventoSom& e) { return e.som == s && !e.origem; })) return;
    sons_.push_back({s, origem});
}

void Partida::acrescentarInimigo(TipoInimigo tipo, glm::vec2 posicao) {
    inimigos_.push_back(criarInimigo(tipo, posicao, andar_.numero));
    inimigos_.back().corpo.pes = mundo::alturaDoChao(andar_.grade, posicao);
    desdeCampo_ = 1e9f;  // recalcula os campos (o voador precisa do seu) no próximo passo
}

void Partida::abrirComporta(int x, int y) {
    // Se já está abrindo, continua de onde está (e não toca de novo)
    if (abrindo_.try_emplace(indice(x, y), 0.0f).second) tocar(Som::Comporta, glm::vec2(x + 0.5f, y + 0.5f));
}

void Partida::recalcularAtributos() {
    atributos_ = calcularAtributos(inventario_, nivel_);
    vida_ = std::min(vida_, atributos_.vidaMaxima);
}

void Partida::ganharXp(int quanto) {
    xp_ += quanto;
    bool subiu = false;
    while (xp_ >= xpParaProximo()) {
        xp_ -= xpParaProximo();
        ++nivel_;
        subiu = true;
    }
    if (!subiu) return;
    recalcularAtributos();
    vida_ = atributos_.vidaMaxima;  // subir de nível enche a vida
    avisar(std::format("Nível {}!", nivel_));
    tocar(Som::Nivel);
}

void Partida::beberTonico(int i) {
    if (vida_ >= atributos_.vidaMaxima) {
        avisar("Vida cheia");
        return;
    }
    if (!inventario_.consumirTonico(i)) return;
    vida_ = std::min(atributos_.vidaMaxima, vida_ + CURA_TONICO);
    avisar(std::format("Tônico restaurador: +{} de vida", CURA_TONICO));
}

void Partida::usarEspaco(int i) {
    if (i < 0 || i >= ESPACOS_INVENTARIO || !inventario_.espaco(i)) return;
    const TipoItem tipo = inventario_.espaco(i)->tipo;
    if (tipo == TipoItem::Tonico) {
        beberTonico(i);
        return;
    }
    if (classe(tipo) == ClasseItem::ArmaDeFogo) armas_.recarga = 0;  // trocar de arma cancela a recarga
    if (!inventario_.equipar(i)) return;
    recalcularAtributos();
    const Item& novo = classe(tipo) == ClasseItem::ArmaBranca   ? inventario_.armaBranca()
                       : classe(tipo) == ClasseItem::ArmaDeFogo ? inventario_.armaDeFogo()
                                                                : inventario_.armadura();
    avisar("Equipado: " + nomeItem(novo));
}

void Partida::largarEspaco(int i) {
    auto item = inventario_.remover(i);
    if (!item) return;
    // À frente do jogador, sem atravessar parede
    const glm::vec2 frente{std::cos(jogador_.angulo), std::sin(jogador_.angulo)};
    const glm::vec2 lugar = mundo::moverComColisao(andar_.grade, jogador_.posicao, frente * 0.6f, 0.1f, jogador_.corpo.pes);
    chao_.push_back({lugar, std::move(*item)});
}

bool Partida::pegarItemAFrente() {
    const glm::vec2 frente{std::cos(jogador_.angulo), std::sin(jogador_.angulo)};
    int melhor = -1;
    float melhorDistancia = ALCANCE_PEGAR;
    for (size_t k = 0; k < chao_.size(); ++k) {
        if (!std::holds_alternative<Item>(chao_[k].conteudo) || !noMesmoNivel(chao_[k].posicao)) continue;
        const glm::vec2 ate = chao_[k].posicao - jogador_.posicao;
        const float aoLongo = glm::dot(ate, frente);
        const float lateral = std::abs(ate.x * frente.y - ate.y * frente.x);
        if (aoLongo < -0.2f || aoLongo > melhorDistancia || lateral > LARGURA_PEGAR) continue;
        if (!mundo::temLinhaDeVisao(andar_.grade, jogador_.posicao, chao_[k].posicao)) continue;
        melhor = static_cast<int>(k);
        melhorDistancia = aoLongo;
    }
    if (melhor < 0) return false;
    Item& item = std::get<Item>(chao_[static_cast<size_t>(melhor)].conteudo);
    const std::string nome = nomeItem(item);
    if (!inventario_.adicionar(item)) {
        avisar("Inventário cheio");
        return false;  // o E segue para a comporta ou o elevador à frente
    }
    chao_.erase(chao_.begin() + melhor);
    avisar("Você pegou: " + nome);
    tocar(Som::Pegar);
    return true;
}

void Partida::pegarAoPassar() {
    std::erase_if(chao_, [&](CoisaNoChao& c) {
        if (glm::distance(c.posicao, jogador_.posicao) >= RAIO_PEGAR || !noMesmoNivel(c.posicao)) return false;
        if (auto* m = std::get_if<MunicaoNoChao>(&c.conteudo)) {
            int& reserva = armas_.municao.de(m->tipo);
            const int cabe = std::min(m->quantidade, maximoMunicao(m->tipo) - reserva);
            if (cabe <= 0) return false;  // no máximo: a caixa fica
            reserva += cabe;
            m->quantidade -= cabe;
            avisar(std::format("+{} {}", cabe, nomeMunicao(m->tipo, cabe)));
            tocar(Som::Pegar);
            return m->quantidade == 0;
        }
        if (auto* e = std::get_if<int>(&c.conteudo)) {
            engrenagens_ += *e;
            avisar(std::format("+{} engrenagens", *e));
            tocar(Som::Pegar);
            return true;
        }
        return false;  // itens pedem E
    });
}

void Partida::recompensarDestruidos() {
    for (Inimigo& i : inimigos_) {
        if (i.estado != EstadoInimigo::Destruido || i.recompensado) continue;
        i.recompensado = true;
        ++destruidos_;
        tocar(Som::InimigoDestruido);
        for (auto& c : soltarDoInimigo(i.tipo, i.posicao, andar_.numero, rng_)) chao_.push_back(std::move(c));
        ganharXp(dados(i.tipo).xp);
        if (dados(i.tipo).chefe) {
            // Chefe: obra-prima e engrenagens a mais; a arena destranca e a Grande Máquina começa a contar a vitória
            ++chefesDerrotados_;
            chao_.push_back({i.posicao + glm::vec2(0.3f, 0.0f), gerarObraPrima(andar_.numero, rng_)});
            chao_.push_back({i.posicao - glm::vec2(0.3f, 0.0f), ENGRENAGENS_CHEFE});
            if (lutando_) trancarArena(false);
            if (i.tipo == TipoInimigo::GrandeMaquina) desdeQuedaFinal_ = 0.0f;
            avisar(std::format("{} derrotad{}!", dados(i.tipo).nome, i.tipo == TipoInimigo::Capataz ? "o" : "a"));
        }
    }
}

void Partida::usar() {
    if (pegarItemAFrente()) return;
    const auto alvo = mundo::celulaAFrente(andar_.grade, jogador_.posicao, jogador_.angulo, ALCANCE_USAR);
    if (!alvo) return;
    switch (andar_.grade.em(alvo->x, alvo->y)) {
        case mundo::Celula::Comporta:
            abrirComporta(alvo->x, alvo->y);
            break;
        case mundo::Celula::ComportaTrancada:
            if (!temChave_) {
                avisar("Comporta trancada: encontre a válvula-chave");
                break;
            }
            temChave_ = false;
            andar_.grade.definir(alvo->x, alvo->y, mundo::Celula::Comporta);
            abrirComporta(alvo->x, alvo->y);
            avisar("Comporta destrancada");
            break;
        case mundo::Celula::Elevador:
            if (chefe()) avisar("Derrote o chefe primeiro");
            else if (andar_.numero < mundo::ULTIMO_ANDAR) descer();  // no último, a vitória vem sozinha
            break;
        default: break;
    }
}

void Partida::descer() {
    andar_ = mundo::gerarAndar(semente_, andar_.numero + 1);
    jogador_.posicao = andar_.inicio;  // mantém o ângulo
    jogador_.corpo = {};               // pés no chão do início, sem queda herdada do pulo
    jogador_.corpo.pes = mundo::alturaDoChao(andar_.grade, andar_.inicio);
    exploracao_ = mundo::Exploracao(andar_.grade.largura(), andar_.grade.altura());
    abrindo_.clear();
    temChave_ = false;
    // Vida, armas, inventário e nível continuam; inimigos, tiros e chão são do andar
    inimigos_ = gerarInimigos(andar_, semente_);
    projeteis_.clear();
    explosoes_.clear();
    campoVoador_.clear();
    chao_ = gerarChao(andar_, semente_);
    desdeCampo_ = 1e9f;
    ++trocasDeAndar_;
    avisar(std::format("Andar {}", andar_.numero));
    tocar(Som::Elevador);
}

void Partida::receberDano(int bruto, TipoInimigo autor) {
    tempoDesdeDano_ = 0.0f;
    tocar(Som::Dano);
    if (invencivel_ || desdeQuedaFinal_) return;  // depois da Grande Máquina cair, nada mais fere
    vida_ -= std::max(1, bruto - defesa());
    if (vida_ <= 0 && !morto_) {
        morto_ = true;
        // "por um Rato mecânico", "por uma Vespa a corda", "pela Rainha dos Ratos", "pelo Capataz a Vapor"
        const bool chefe = dados(autor).chefe;
        const bool feminino = autor == TipoInimigo::Vespa || autor == TipoInimigo::RainhaRatos || autor == TipoInimigo::GrandeMaquina;
        causa_ = std::format("Destruído {} {}", chefe ? (feminino ? "pela" : "pelo") : (feminino ? "por uma" : "por um"),
                             dados(autor).nome);
    }
}

void Partida::cair(int dano) {
    tempoDesdeDano_ = 0.0f;
    tocar(Som::Dano);
    if (invencivel_ || desdeQuedaFinal_) return;  // depois da Grande Máquina cair, nada mais fere
    vida_ -= dano;
    if (vida_ <= 0 && !morto_) {
        morto_ = true;
        causa_ = "Caiu de muito alto";
    }
}

void Partida::atualizar(const Comandos& c, float dt) {
    sons_.clear();
    if (morto_ || venceu_) return;
    tempoDeJogo_ += dt;
    tempoDesdeDano_ += dt;
    // A Grande Máquina caiu: vitória depois da espera (se o jogador continuar vivo)
    if (desdeQuedaFinal_ && vida_ > 0 && (*desdeQuedaFinal_ += dt) >= ESPERA_VITORIA) {
        venceu_ = true;
        causa_ = CAUSA_VITORIA;
        return;
    }
    clarao_ = std::max(0.0f, clarao_ - dt);

    const glm::vec2 antes = jogador_.posicao;
    if (const int danoQueda = mundo::danoDeQueda(atualizarJogador(jogador_, c, andar_.grade, dt))) cair(danoQueda);
    // O jogador não atravessa inimigos vivos: é empurrado para fora de quem estiver colado nele
    for (const Inimigo& i : inimigos_) {
        if (i.estado == EstadoInimigo::Destruido) continue;
        // Só empurra quem divide a altura com o corpo do jogador (o voador lá em cima passa por cima)
        if (i.corpo.pes >= jogador_.corpo.pes + mundo::ALTURA_CORPO ||
            i.corpo.pes + dados(i.tipo).altura <= jogador_.corpo.pes)
            continue;
        glm::vec2 d = jogador_.posicao - i.posicao;
        const float dist = glm::length(d);
        const float minimo = RAIO_JOGADOR + raioDe(i);
        if (dist >= minimo) continue;
        d = dist > 1e-5f ? d / dist : glm::vec2(-1.0f, 0.0f);
        jogador_.posicao = mundo::moverComColisao(andar_.grade, jogador_.posicao, d * (minimo - dist), RAIO_JOGADOR,
                                                  jogador_.corpo.pes);
    }
    // Passos: metal na fábrica, pedra no esgoto e nas minas
    andadoDesdePasso_ += glm::distance(antes, jogador_.posicao);
    if (andadoDesdePasso_ >= DISTANCIA_PASSO) {
        andadoDesdePasso_ -= DISTANCIA_PASSO;
        tocar(andar_.zona == mundo::Zona::Fabrica ? Som::PassoMetal : Som::PassoPedra);
    }
    conferirArena();
    if (lutando_) trancarLivres();
    if (c.usar) usar();
    if (c.usarRapido >= 1 && c.usarRapido <= 4) usarEspaco(c.usarRapido - 1);
    pegarAoPassar();

    // Armas; o barulho acorda os inimigos parados
    const ResultadoAtaque ataque =
        atualizarArmas(armas_, inventario_, atributos_, c, jogador_.posicao, jogador_.corpo.pes, jogador_.angulo, jogador_.inclinacao,
                       andar_.grade, inimigos_, rng_, dt);
    if (ataque.disparou) {
        clarao_ = DURACAO_CLARAO;
        const TipoItem arma = inventario_.armaDeFogo().tipo;
        tocar(arma == TipoItem::Bacamarte ? Som::DisparoBacamarte
              : arma == TipoItem::Tesla   ? Som::DisparoTesla
                                          : Som::DisparoRevolver);
    }
    if (ataque.golpeou && ataque.acertos == 0) tocar(Som::GolpeAr);
    if (ataque.acertos > 0) tocar(Som::Acerto);
    if (ataque.comecouRecarga) tocar(Som::Recarga);
    if (ataque.vazia) tocar(Som::ArmaVazia);
    if (ataque.barulho > 0) acordarPorBarulho(inimigos_, jogador_.posicao, ataque.barulho);
    if (!ataque.mensagem.empty()) avisar(ataque.mensagem);
    recompensarDestruidos();

    // Campo de fluxo até o jogador (a gaiola do elevador bloqueia a passagem durante a busca)
    desdeCampo_ += dt;
    if (desdeCampo_ >= RECALCULO_CAMINHO) {
        const glm::ivec2 celula(glm::floor(jogador_.posicao));
        campo_ = mundo::distanciasComAltura(andar_.grade, celula, mundo::ANDARILHO);
        campoSaltador_ = mundo::distanciasComAltura(andar_.grade, celula, mundo::SALTADOR);
        const bool temVoador = std::any_of(inimigos_.begin(), inimigos_.end(), [](const Inimigo& i) {
            return dados(i.tipo).voa && i.estado != EstadoInimigo::Destruido;
        });
        if (temVoador) campoVoador_ = mundo::distanciasComAltura(andar_.grade, celula, mundo::VOADOR);
        else campoVoador_.clear();
        desdeCampo_ = 0.0f;
    }

    // Inimigos e seus tiros (bombas caem sem som; o som é o da explosão)
    MundoIA mundoIA{andar_.grade, jogador_.posicao, campo_, rng_, RAIO_JOGADOR, &campoSaltador_, jogador_.corpo.pes};
    if (!campoVoador_.empty()) mundoIA.campoVoador = &campoVoador_;
    if (andar_.arena && *andar_.arena < andar_.salas.size()) mundoIA.arena = andar_.salas[*andar_.arena];
    mundoIA.lutaComChefe = lutando_;
    for (const Inimigo& i : inimigos_) {
        if (i.estado == EstadoInimigo::Destruido) continue;
        mundoIA.ratosVivos += i.tipo == TipoInimigo::Rato;
        mundoIA.voadoresVivos += dados(i.tipo).voa;
    }
    EventosIA eventos = atualizarInimigos(inimigos_, mundoIA, dt);
    for (const glm::ivec2& c2 : eventos.comportasParaAbrir) abrirComporta(c2.x, c2.y);
    // Chamados pelo chefe: já entram perseguindo
    for (const Invocacao& c2 : eventos.invocacoes) {
        acrescentarInimigo(c2.tipo, c2.posicao);
        inimigos_.back().estado = EstadoInimigo::Perseguindo;
    }
    // Onda pelo chão: quem está pulando escapa
    for (const Onda& o : eventos.ondas)
        if (ondaAtinge(o, jogador_.posicao, jogador_.corpo.noChao)) receberDano(o.dano, o.autor);
    for (const Alerta& a : eventos.alertas) tocar(a.tipo == TipoInimigo::Vespa ? Som::Zumbido : Som::Motor, a.posicao);
    for (const Projetil& t : eventos.tiros)
        if (t.raioExplosao == 0) tocar(Som::TiroInimigo, glm::vec2(t.posicao.x, t.posicao.z));
    projeteis_.insert(projeteis_.end(), eventos.tiros.begin(), eventos.tiros.end());
    for (const Golpe& g : eventos.golpes) receberDano(g.dano, g.autor);
    const glm::vec3 centro{jogador_.posicao.x, jogador_.corpo.pes + mundo::ALTURA_CORPO / 2, jogador_.posicao.y};
    const ResultadoProjeteis tiros = atualizarProjeteis(projeteis_, andar_.grade, centro, RAIO_JOGADOR, dt);
    for (const Golpe& g : tiros.golpes) receberDano(g.dano, g.autor);
    // Explosões: som, clarão e o desenho por um instante
    for (Explosao& x : explosoes_) x.tempo += dt;
    std::erase_if(explosoes_, [](const Explosao& x) { return x.tempo >= DURACAO_EXPLOSAO; });
    for (const glm::vec3& p : tiros.explosoes) {
        explosoes_.push_back({p});
        clarao_ = DURACAO_CLARAO;
        tocar(Som::Explosao, glm::vec2(p.x, p.z));
    }

    // Comportas sobem; ao terminar viram passagem
    for (auto it = abrindo_.begin(); it != abrindo_.end();) {
        it->second += dt / TEMPO_ABRIR_COMPORTA;
        if (it->second >= 1.0f) {
            const int largura = andar_.grade.largura();
            andar_.grade.definir(it->first % largura, it->first / largura, mundo::Celula::ComportaAberta);
            it = abrindo_.erase(it);
        } else {
            ++it;
        }
    }

    if (andar_.chave && glm::distance(*andar_.chave, jogador_.posicao) < RAIO_PEGAR && noMesmoNivel(*andar_.chave)) {
        andar_.chave.reset();
        temChave_ = true;
        avisar("Você pegou a válvula-chave");
        tocar(Som::Pegar);
    }

    mundo::explorarAoRedor(exploracao_, andar_.grade, jogador_.posicao, mundo::RAIO_EXPLORACAO);

    if (tempoMensagem_ > 0.0f) {
        tempoMensagem_ -= dt;
        if (tempoMensagem_ <= 0.0f) mensagem_.clear();
    }

    // Destroços somem depois de um tempo
    std::erase_if(inimigos_, [](const Inimigo& i) {
        return i.estado == EstadoInimigo::Destruido && i.tempoNoEstado > TEMPO_DESTROCOS;
    });
}

}
