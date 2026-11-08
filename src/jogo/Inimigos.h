#pragma once
#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Dificuldade.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

namespace jogo {

enum class TipoInimigo { Rato, Automato, Atirador, Colosso, Vespa, Dirigivel, RainhaRatos, Capataz, GrandeMaquina };
constexpr int TOTAL_TIPOS_INIMIGO = 9;

// Preparando e Recuperando formam o "Atacando" da spec: preparo do golpe e recuperação depois dele
enum class EstadoInimigo { Parado, Perseguindo, Preparando, Recuperando, Destruido };

struct DadosInimigo {
    const char* nome;
    int vida;
    int danoMin, danoMax;
    float velocidade;  // células por segundo
    int defesa;
    int xp;
    float preparo, recuperacao;  // segundos
    bool aDistancia;
    float altura;  // do chão ao topo, em células (mira vertical e tamanho do sprite)
    bool voa;      // vespa e dirigível: sem gravidade enquanto vivos, por cima do terreno
    bool chefe;    // Rainha dos Ratos, Capataz e Grande Máquina: arena, barra de vida, sem escala por andar
    float raio;    // do corpo (colisão, golpe e tiro); 0,25 nos inimigos comuns
};

// Tabela da seção 5 da spec
const DadosInimigo& dados(TipoInimigo t);

struct Inimigo;
float raioDe(const Inimigo& i);

// valor × (1 + 0,15 × (andar − 1)), arredondado, mínimo 1
int escalar(int valor, int andar);

struct Inimigo {
    TipoInimigo tipo;
    glm::vec2 posicao;
    int vida;
    int vidaMaxima;
    int danoMin, danoMax;
    EstadoInimigo estado = EstadoInimigo::Parado;
    float tempoNoEstado = 0;
    glm::vec2 empurrao{0.0f};  // aplicado (e zerado) no próximo passo da IA
    bool recompensado = false;  // destruído e já contado (XP e o que caiu no chão)
    bool alertou = false;       // voador: o som do voo já avisou que ele vem
    // Chefes: direção fixada da investida e relógios dos ataques (desde o último uso)
    glm::vec2 direcao{0.0f};
    float recarga = 0, recarga2 = 0;
    int ataque = 0;    // chefe: qual ataque prepara (Capataz: 1 jato, 2 pancada) ou tiros que faltam (Grande Máquina)
    int chamados = 0;  // Grande Máquina: quantas vezes já chamou voadores (alterna vespas e dirigível)
    mundo::CorpoVertical corpo;  // altura dos pés (segue o terreno; sem dano de queda)
    float multiplicador = 1.0f;  // da dificuldade: vale também para danos fora da tabela (pancada do Capataz)
};

// Um acerto num alvo (dano bruto, antes da defesa) e quem o deu
struct Golpe {
    int dano;
    TipoInimigo autor;
};

// Vida e dano já escalados para o andar e multiplicados pela dificuldade
Inimigo criarInimigo(TipoInimigo t, glm::vec2 posicao, int andar, Dificuldade d = Dificuldade::Normal);

// População do andar (determinística pela semente); nunca na sala inicial nem no elevador
std::vector<Inimigo> gerarInimigos(const mundo::Andar& andar, uint32_t semente, Dificuldade d = Dificuldade::Normal);

// Mapas de teste: r Rato, a Autômato, g Atirador da Guilda, c Colosso, v Vespa, d Dirigível,
// R Rainha dos Ratos, P Capataz a Vapor, M Grande Máquina
std::vector<Inimigo> inimigosDasMarcas(const mundo::Andar& andar, Dificuldade d = Dificuldade::Normal);
TipoInimigo tipoDaMarca(char letra);  // letra desconhecida vira Rato

// Chefe de cada andar de chefe: 2 Rainha dos Ratos, 4 Capataz a Vapor, 6 (e outros) Grande Máquina
TipoInimigo chefeDoAndar(int andar);

}
