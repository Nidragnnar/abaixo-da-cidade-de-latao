#include "jogo/Partida.h"

#include <format>
#include <utility>

#include <glm/geometric.hpp>

namespace jogo {

Partida::Partida(uint32_t semente, int andarInicial) : Partida(mundo::gerarAndar(semente, andarInicial), semente) {}

Partida::Partida(mundo::Andar andar, uint32_t semente)
    : semente_(semente),
      andar_(std::move(andar)),
      jogador_{andar_.inicio, 0.0f},
      exploracao_(andar_.grade.largura(), andar_.grade.altura()) {}

float Partida::aberturaComporta(int x, int y) const {
    if (andar_.grade.em(x, y) == mundo::Celula::ComportaAberta) return 1.0f;
    const auto it = abrindo_.find(indice(x, y));
    return it == abrindo_.end() ? 0.0f : it->second;
}

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
    ++trocasDeAndar_;
    avisar(std::format("Andar {}", andar_.numero));
}

void Partida::atualizar(const Comandos& c, float dt) {
    atualizarJogador(jogador_, c, andar_.grade, dt);
    if (c.usar) usar();

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
}

}
