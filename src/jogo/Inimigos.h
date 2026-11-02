#pragma once
#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

#include "mundo/Gerador.h"

namespace jogo {

enum class TipoInimigo { Rato, Automato, Atirador, Colosso };

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
};

// Tabela da seção 5 da spec
const DadosInimigo& dados(TipoInimigo t);

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
};

// Vida e dano já escalados para o andar
Inimigo criarInimigo(TipoInimigo t, glm::vec2 posicao, int andar);

// População do andar (determinística pela semente); nunca na sala inicial nem no elevador
std::vector<Inimigo> gerarInimigos(const mundo::Andar& andar, uint32_t semente);

// Mapas de teste: r Rato, a Autômato, g Atirador da Guilda, c Colosso
std::vector<Inimigo> inimigosDasMarcas(const mundo::Andar& andar);

}
