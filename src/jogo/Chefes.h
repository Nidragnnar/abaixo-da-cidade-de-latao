#pragma once
#include "jogo/IA.h"

namespace jogo {

// Rainha dos Ratos (andar 2)
constexpr float INVESTIDA_MIRA = 0.6f;        // parada mirando antes de correr
constexpr float INVESTIDA_VELOCIDADE = 7.0f;  // células por segundo
constexpr float INVESTIDA_DURACAO = 1.2f;     // corrida máxima
constexpr float INVESTIDA_ALCANCE = 6.0f;     // começa a mirar com o jogador a até tanto
constexpr float INVESTIDA_INTERVALO = 3.0f;   // entre o fim de uma investida e o começo da próxima
constexpr float TONTURA = 1.5f;               // depois de bater na parede (a recuperação da tabela)
constexpr float NINHADA_INTERVALO = 8.0f;
constexpr int NINHADA_TAMANHO = 3;
constexpr int MAXIMO_RATOS = 6;               // ratos vivos no andar

// Capataz a Vapor (andar 4)
constexpr float JATO_ALCANCE = 7.0f;
constexpr float JATO_INTERVALO = 3.0f;
constexpr float JATO_PREPARO = 0.5f;
constexpr float JATO_ANGULOS[] = {-20.0f, -10.0f, 0.0f, 10.0f, 20.0f};  // graus, em volta da linha até o jogador
constexpr float PANCADA_ALCANCE = 2.5f;       // também o raio da onda
constexpr float PANCADA_INTERVALO = 5.0f;
constexpr float PANCADA_PREPARO = 0.8f;
constexpr int PANCADA_DANO_MIN = 9, PANCADA_DANO_MAX = 13;

// Grande Máquina (andar 6)
constexpr int RAJADA_TIROS = 3;
constexpr float RAJADA_ESPACO = 0.2f;         // entre os tiros da rajada
constexpr float RAJADA_INTERVALO = 2.0f;
constexpr float RAJADA_INTERVALO_FASE2 = 1.2f;
constexpr float CHAMADO_INTERVALO = 12.0f;    // fase 2: 2 vespas ou 1 dirigível, alternando
constexpr int MAXIMO_VOADORES = 4;

// A onda da pancada atinge quem está no chão dentro do raio (pulando, escapa)
bool ondaAtinge(const Onda& o, glm::vec2 jogador, bool noChao);

// Um passo do chefe (estado, ataques e invocações); o voo e a gravidade ficam com atualizarInimigos
void atualizarChefe(Inimigo& i, const MundoIA& m, EventosIA& e, bool ve, float dt);

}
