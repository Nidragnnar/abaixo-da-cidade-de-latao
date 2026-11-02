#include "jogo/Partida.h"

#include <format>
#include <utility>

#include <glm/geometric.hpp>

#include "jogo/IA.h"
#include "jogo/Placar.h"

namespace jogo {

Partida::Partida(uint32_t semente, int andarInicial) : Partida(mundo::gerarAndar(semente, andarInicial), semente) {
    inimigos_ = gerarInimigos(andar_, semente_);
}

Partida::Partida(mundo::Andar andar, uint32_t semente)
    : semente_(semente),
      andar_(std::move(andar)),
      jogador_{andar_.inicio, 0.0f},
      exploracao_(andar_.grade.largura(), andar_.grade.altura()),
      rng_(semente ^ 0xC0FFEEu),
      inimigos_(inimigosDasMarcas(andar_)) {}

float Partida::aberturaComporta(int x, int y) const {
    if (andar_.grade.em(x, y) == mundo::Celula::ComportaAberta) return 1.0f;
    const auto it = abrindo_.find(indice(x, y));
    return it == abrindo_.end() ? 0.0f : it->second;
}

int Partida::pontuacao() const { return calcularPontuacao(andar_.numero, destruidos_, engrenagens(), nivel()); }

void Partida::avisar(std::string texto) {
    mensagem_ = std::move(texto);
    tempoMensagem_ = DURACAO_MENSAGEM;
}

void Partida::usar() {
    const auto alvo = mundo::celulaAFrente(andar_.grade, jogador_.posicao, jogador_.angulo, ALCANCE_USAR);
    if (!alvo) return;
    const int i = indice(alvo->x, alvo->y);
    switch (andar_.grade.em(alvo->x, alvo->y)) {
        case mundo::Celula::Comporta:
            abrindo_.try_emplace(i, 0.0f);  // se já está abrindo, continua de onde está
            break;
        case mundo::Celula::ComportaTrancada:
            if (!temChave_) {
                avisar("Comporta trancada: encontre a válvula-chave");
                break;
            }
            temChave_ = false;
            andar_.grade.definir(alvo->x, alvo->y, mundo::Celula::Comporta);
            abrindo_.try_emplace(i, 0.0f);
            avisar("Comporta destrancada");
            break;
        case mundo::Celula::Elevador: descer(); break;
        default: break;
    }
}

void Partida::descer() {
    andar_ = mundo::gerarAndar(semente_, andar_.numero + 1);
    jogador_.posicao = andar_.inicio;  // mantém o ângulo
    exploracao_ = mundo::Exploracao(andar_.grade.largura(), andar_.grade.altura());
    abrindo_.clear();
    temChave_ = false;
    // Vida e armas continuam; inimigos e tiros são do andar
    inimigos_ = gerarInimigos(andar_, semente_);
    projeteis_.clear();
    desdeCampo_ = 1e9f;
    ++trocasDeAndar_;
    avisar(std::format("Andar {}", andar_.numero));
}

void Partida::receberDano(int bruto, TipoInimigo autor) {
    tempoDesdeDano_ = 0.0f;
    if (invencivel_) return;
    vida_ -= std::max(1, bruto - defesa());
    if (vida_ <= 0 && !morto_) {
        morto_ = true;
        causa_ = std::format("Destruído por um {}", dados(autor).nome);
    }
}

void Partida::atualizar(const Comandos& c, float dt) {
    if (morto_) return;
    tempoDesdeDano_ += dt;
    clarao_ = std::max(0.0f, clarao_ - dt);

    atualizarJogador(jogador_, c, andar_.grade, dt);
    if (c.usar) usar();

    // Armas; o barulho acorda os inimigos parados
    const ResultadoAtaque ataque =
        atualizarArmas(armas_, c, jogador_.posicao, jogador_.angulo, andar_.grade, inimigos_, rng_, dt, jogador_.inclinacao);
    destruidos_ += ataque.destruidos;
    if (ataque.disparou) clarao_ = DURACAO_CLARAO;
    if (ataque.barulho > 0) acordarPorBarulho(inimigos_, jogador_.posicao, ataque.barulho);
    if (!ataque.mensagem.empty()) avisar(ataque.mensagem);

    // Campo de fluxo até o jogador (a gaiola do elevador bloqueia a passagem durante a busca)
    desdeCampo_ += dt;
    if (desdeCampo_ >= RECALCULO_CAMINHO) {
        campo_ = mundo::distanciasNaGrade(andar_.grade, glm::ivec2(glm::floor(jogador_.posicao)), false, false);
        desdeCampo_ = 0.0f;
    }

    // Inimigos e seus tiros
    EventosIA eventos = atualizarInimigos(inimigos_, {andar_.grade, jogador_.posicao, campo_, rng_, RAIO_JOGADOR}, dt);
    for (const glm::ivec2& c2 : eventos.comportasParaAbrir) abrindo_.try_emplace(indice(c2.x, c2.y), 0.0f);
    projeteis_.insert(projeteis_.end(), eventos.tiros.begin(), eventos.tiros.end());
    if (eventos.dano > 0) receberDano(eventos.dano, *eventos.autor);
    const ResultadoProjeteis tiros = atualizarProjeteis(projeteis_, andar_.grade, jogador_.posicao, RAIO_JOGADOR, dt);
    if (tiros.dano > 0) receberDano(tiros.dano, *tiros.autor);

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

    if (andar_.chave && glm::distance(*andar_.chave, jogador_.posicao) < RAIO_PEGAR) {
        andar_.chave.reset();
        temChave_ = true;
        avisar("Você pegou a válvula-chave");
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
