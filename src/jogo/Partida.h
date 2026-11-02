#pragma once
#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "jogo/Armas.h"
#include "jogo/Comandos.h"
#include "jogo/Inimigos.h"
#include "jogo/Jogador.h"
#include "jogo/Projeteis.h"
#include "mundo/Consultas.h"
#include "mundo/Gerador.h"

namespace jogo {

constexpr float TEMPO_ABRIR_COMPORTA = 0.5f;  // segundos para a comporta subir
constexpr float ALCANCE_USAR = 1.5f;
constexpr float RAIO_PEGAR = 0.6f;
constexpr float DURACAO_MENSAGEM = 2.0f;
constexpr int VIDA_INICIAL = 30;
constexpr int DEFESA_INICIAL = 1;      // casaco de couro (até o inventário da etapa 4)
constexpr float DURACAO_CLARAO = 0.05f;  // o disparo ilumina a cena

// Estado de uma partida: andar, jogador, armas, inimigos, projéteis, comportas, chave, exploração e mensagens
class Partida {
public:
    // Gera o andar e a população a partir da semente
    Partida(uint32_t semente, int andarInicial = 1);
    // Para testes: inimigos vêm das marcas do texto; o próximo andar sai de gerarAndar(semente, numero + 1)
    Partida(mundo::Andar andar, uint32_t semente);

    // Não faz nada depois da morte
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
    void definirInclinacao(float inclinacao) { jogador_.inclinacao = inclinacao; }
    void definirPosicao(glm::vec2 posicao) { jogador_.posicao = posicao; }
    // Opção --invencivel: o dano não reduz a vida (para observar a IA)
    void tornarInvencivel() { invencivel_ = true; }
    // Vazia quando não há mensagem
    std::string_view mensagem() const { return mensagem_; }
    int trocasDeAndar() const { return trocasDeAndar_; }
    uint32_t semente() const { return semente_; }

    int vida() const { return vida_; }
    int vidaMaxima() const { return vidaMaxima_; }
    int defesa() const { return DEFESA_INICIAL; }
    const std::vector<Inimigo>& inimigos() const { return inimigos_; }
    const std::vector<Projetil>& projeteis() const { return projeteis_; }
    const Armas& armas() const { return armas_; }
    bool morto() const { return morto_; }
    std::string_view causaDaMorte() const { return causa_; }
    int inimigosDestruidos() const { return destruidos_; }
    int nivel() const { return 1; }        // etapa 4
    int engrenagens() const { return 0; }  // etapa 4
    int pontuacao() const;
    float clarao() const { return clarao_; }
    float tempoDesdeDano() const { return tempoDesdeDano_; }

private:
    void usar();
    void descer();
    void avisar(std::string texto);
    void receberDano(int bruto, TipoInimigo autor);
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

    std::mt19937 rng_;  // dano e dispersão (a população usa a semente do andar)
    int vida_ = VIDA_INICIAL;
    int vidaMaxima_ = VIDA_INICIAL;
    Armas armas_;
    std::vector<Inimigo> inimigos_;
    std::vector<Projetil> projeteis_;
    std::vector<int> campo_;            // distâncias até o jogador, para a IA
    float desdeCampo_ = 1e9f;
    bool morto_ = false;
    bool invencivel_ = false;
    std::string causa_;
    int destruidos_ = 0;
    float clarao_ = 0.0f;
    float tempoDesdeDano_ = 1e9f;
};

}
