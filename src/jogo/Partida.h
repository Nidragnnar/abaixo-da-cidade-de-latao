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
#include "jogo/Inventario.h"
#include "jogo/Itens.h"
#include "jogo/Jogador.h"
#include "jogo/Projeteis.h"
#include "jogo/Sons.h"
#include "mundo/Consultas.h"
#include "mundo/Gerador.h"

namespace jogo {

constexpr float TEMPO_ABRIR_COMPORTA = 0.5f;  // segundos para a comporta subir
constexpr float ALCANCE_USAR = 1.5f;
constexpr float RAIO_PEGAR = 0.6f;
constexpr float DURACAO_MENSAGEM = 2.0f;
constexpr int VIDA_INICIAL = 30;
constexpr int XP_POR_NIVEL = 20;        // XP para o próximo nível = 20 × nível
constexpr int CURA_TONICO = 15;
constexpr float ALCANCE_PEGAR = 1.5f;   // pegar item com E
constexpr float LARGURA_PEGAR = 0.4f;   // quanto o item pode estar fora da linha do olhar
constexpr float DURACAO_CLARAO = 0.05f;  // o disparo ilumina a cena
constexpr float DISTANCIA_PASSO = 1.7f;  // um som de passo a cada tanto andado
constexpr float DURACAO_EXPLOSAO = 0.3f;  // quanto a explosão da bomba fica na tela
constexpr float ESPERA_VITORIA = 3.0f;    // da queda da Grande Máquina até a tela de vitória
constexpr int ENGRENAGENS_CHEFE = 30;
constexpr const char* CAUSA_VITORIA = "Venceu a Grande Máquina";

struct Explosao {
    glm::vec3 posicao;
    float tempo = 0;  // desde a explosão
};

struct Salvamento;

// Estado de uma partida: andar, jogador, armas, inimigos, projéteis, comportas, chave, exploração e mensagens
class Partida {
    friend struct Salvamento;  // grava e remonta o estado (jogo/Salvamento.h)
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
    // Opção --inimigo: põe mais um inimigo no andar atual, com os pés no chão da célula
    void acrescentarInimigo(TipoInimigo tipo, glm::vec2 posicao);
    // Opção --vida-chefe: o chefe vivo do andar começa com essa vida (cenário da vitória)
    void definirVidaDoChefe(int vida);
    // Opção --invencivel: o dano não reduz a vida (para observar a IA)
    void tornarInvencivel() { invencivel_ = true; }
    // Vazia quando não há mensagem
    std::string_view mensagem() const { return mensagem_; }
    int trocasDeAndar() const { return trocasDeAndar_; }
    uint32_t semente() const { return semente_; }

    int vida() const { return vida_; }
    int vidaMaxima() const { return atributos_.vidaMaxima; }
    int defesa() const { return atributos_.defesa; }
    const Inventario& inventario() const { return inventario_; }
    const Atributos& atributos() const { return atributos_; }
    const std::vector<Inimigo>& inimigos() const { return inimigos_; }
    const std::vector<Projetil>& projeteis() const { return projeteis_; }
    const std::vector<Explosao>& explosoes() const { return explosoes_; }
    // Vazio enquanto o andar não tem voador vivo (a busca só roda quando precisa)
    const std::vector<int>& campoVoador() const { return campoVoador_; }
    const std::vector<CoisaNoChao>& chao() const { return chao_; }
    const Armas& armas() const { return armas_; }
    bool morto() const { return morto_; }
    // Chefe vivo do andar (nulo se não há ou já caiu); a luta vai da entrada na arena até ele cair
    const Inimigo* chefe() const;
    bool lutaComChefe() const { return lutando_; }
    int chefesDerrotados() const { return chefesDerrotados_; }
    bool venceu() const { return venceu_; }
    bool terminou() const { return morto_ || venceu_; }  // tela de fim e placar
    float tempoDeJogo() const { return tempoDeJogo_; }
    std::string_view causaDaMorte() const { return causa_; }
    int inimigosDestruidos() const { return destruidos_; }
    int nivel() const { return nivel_; }
    int xp() const { return xp_; }
    int xpParaProximo() const { return XP_POR_NIVEL * nivel_; }
    int engrenagens() const { return engrenagens_; }
    int pontuacao() const;
    float clarao() const { return clarao_; }
    float tempoDesdeDano() const { return tempoDesdeDano_; }
    // Sons do último atualizar() (a lista é zerada no começo de cada passo)
    const std::vector<EventoSom>& sons() const { return sons_; }

    // Tela do inventário: equipa (arma/armadura) ou bebe (tônico); larga no chão à frente; troca dois espaços
    void usarEspaco(int i);
    void largarEspaco(int i);
    void trocarEspacos(int a, int b) { inventario_.trocar(a, b); }

    // Só para testes
    void colocarNoChao(CoisaNoChao c) { chao_.push_back(std::move(c)); }
    void ganharXp(int quanto);
    void ferirParaTeste(int quanto) { vida_ -= quanto; }
    void destruirChefeParaTeste();

private:
    void usar();
    bool pegarItemAFrente();  // falso se não pegou (nada à frente ou inventário cheio)
    void pegarAoPassar();     // munição e engrenagens
    void recompensarDestruidos();
    void beberTonico(int i);
    void recalcularAtributos();
    void descer();
    void avisar(std::string texto);
    void receberDano(int bruto, TipoInimigo autor);
    void cair(int dano);  // ignora a armadura
    // Coisa no chão nesse ponto está no nível dos pés do jogador (dá para pegar)
    bool noMesmoNivel(glm::vec2 ponto) const {
        return std::abs(mundo::alturaDoChao(andar_.grade, ponto) - jogador_.corpo.pes) < mundo::MESMO_NIVEL;
    }
    void abrirComporta(int x, int y);
    void conferirArena();  // começa a luta quando o jogador entra todo na arena
    void trancarArena(bool trancar);
    void trancarLivres();  // durante a luta: tranca as aberturas da arena sem inimigo dentro
    // Sem origem: no máximo um de cada por passo; com origem: todos
    void tocar(Som s, std::optional<glm::vec2> origem = std::nullopt);
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
    int nivel_ = 1;
    int xp_ = 0;
    int engrenagens_ = 0;
    Inventario inventario_;
    Atributos atributos_ = calcularAtributos(inventario_, 1);
    Armas armas_;
    std::vector<Inimigo> inimigos_;
    std::vector<Projetil> projeteis_;
    std::vector<CoisaNoChao> chao_;
    std::vector<Explosao> explosoes_;
    std::vector<int> campo_, campoSaltador_, campoVoador_;  // distâncias até o jogador (andarilhos, ratos, voadores)
    float desdeCampo_ = 1e9f;
    bool morto_ = false;
    bool invencivel_ = false;
    std::string causa_;
    int destruidos_ = 0;
    float clarao_ = 0.0f;
    float tempoDesdeDano_ = 1e9f;
    std::vector<EventoSom> sons_;
    float andadoDesdePasso_ = 0.0f;
    // Etapa 7: arena, chefes e vitória
    std::vector<glm::ivec2> comportasArena_;  // entradas da arena trancadas durante a luta
    bool lutando_ = false;
    int chefesDerrotados_ = 0;
    std::optional<float> desdeQuedaFinal_;  // contando desde a queda da Grande Máquina
    bool venceu_ = false;
    float tempoDeJogo_ = 0.0f;
};

}
