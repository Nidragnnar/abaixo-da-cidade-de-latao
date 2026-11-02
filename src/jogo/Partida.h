#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include "jogo/Comandos.h"
#include "jogo/Jogador.h"
#include "mundo/Consultas.h"
#include "mundo/Gerador.h"

namespace jogo {

constexpr float TEMPO_ABRIR_COMPORTA = 0.5f;  // segundos para a comporta subir
constexpr float ALCANCE_USAR = 1.5f;
constexpr float RAIO_PEGAR = 0.6f;
constexpr float DURACAO_MENSAGEM = 2.0f;

// Estado de uma partida: andar atual, jogador, comportas, chave, exploração e mensagens
class Partida {
public:
    Partida(uint32_t semente, int andarInicial = 1);
    // Para testes: o próximo andar sai de gerarAndar(semente, andar.numero + 1)
    Partida(mundo::Andar andar, uint32_t semente);

    void atualizar(const Comandos& c, float dt);

    const mundo::Andar& andar() const { return andar_; }
    const Jogador& jogador() const { return jogador_; }
    bool temChave() const { return temChave_; }
    // 0 fechada … 1 aberta
    float aberturaComporta(int x, int y) const;
    const mundo::Exploracao& exploracao() const { return exploracao_; }
    void revelarMapa() { exploracao_.marcarTudo(); }
    // Direção e posição iniciais (opções --angulo e --posicao, para capturas e cenários de teste)
    void definirAngulo(float angulo) { jogador_.angulo = angulo; }
    void definirPosicao(glm::vec2 posicao) { jogador_.posicao = posicao; }
    // Vazia quando não há mensagem
    std::string_view mensagem() const { return mensagem_; }
    int trocasDeAndar() const { return trocasDeAndar_; }
    uint32_t semente() const { return semente_; }

private:
    void usar();
    void descer();
    void avisar(std::string texto);
    int indice(int x, int y) const { return y * andar_.grade.largura() + x; }

    uint32_t semente_;
    mundo::Andar andar_;
    Jogador jogador_;
    mundo::Exploracao exploracao_;
    std::unordered_map<int, float> abrindo_;  // comportas subindo: índice da célula → abertura
    bool temChave_ = false;
    std::string mensagem_;
    float tempoMensagem_ = 0.0f;
    int trocasDeAndar_ = 0;
};

}
