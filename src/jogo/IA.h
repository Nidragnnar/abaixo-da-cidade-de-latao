#pragma once
#include <optional>
#include <random>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Inimigos.h"
#include "jogo/Projeteis.h"
#include "mundo/Grade.h"

namespace jogo {

constexpr float ALCANCE_VISAO = 10.0f;
constexpr float RECALCULO_CAMINHO = 0.5f;  // segundos entre buscas em largura
constexpr float INICIO_GOLPE = 0.9f;       // distância em que o corpo a corpo começa a preparar
constexpr float ALCANCE_GOLPE = 1.1f;      // distância máxima para o golpe acertar no fim do preparo
constexpr float RAIO_INIMIGO = 0.25f;
constexpr float TEMPO_DESTROCOS = 10.0f;
constexpr float DISTANCIA_MIN_ATIRADOR = 3.0f, DISTANCIA_MAX_ATIRADOR = 6.0f;

// Voadores: sobem e descem até a altura do estado, sempre entre o chão e o teto da vizinhança
constexpr float VELOCIDADE_VERTICAL_VOO = 2.5f;  // blocos por segundo
constexpr float VELOCIDADE_MERGULHO = 4.0f;      // a vespa desce até o peito durante o preparo (0,3 s)
constexpr float FOLGA_VOO = 0.1f;                // distância mínima do chão e do teto
constexpr float ALTURA_VOO_VESPA = 1.2f;         // acima dos pés do jogador
constexpr float ALTURA_PEITO = 0.3f;             // o mergulho desce os pés da vespa até aqui (acima dos pés do jogador)
constexpr float DISTANCIA_MERGULHO = 1.5f;       // horizontal
constexpr float DESCIDA_DIRIGIVEL = 1.2f;        // pés do dirigível abaixo do teto

struct MundoIA {
    mundo::Grade& grade;
    glm::vec2 jogador;
    const std::vector<int>& campo;  // dos andarilhos: distanciasComAltura(..., ANDARILHO) a partir do jogador
    std::mt19937& rng;
    float raioJogador = 0.25f;  // inimigos não entram no jogador
    const std::vector<int>* campoSaltador = nullptr;  // campo dos ratos (sobem 1 bloco); sem ele, usam `campo`
    float pesJogador = 0.0f;
    const std::vector<int>* campoVoador = nullptr;  // distanciasComAltura(..., VOADOR); sem ele, usam `campo`
    // Chefes: limites das invocações e onde elas podem nascer
    int ratosVivos = 0, voadoresVivos = 0;
    std::optional<mundo::Sala> arena;
    bool lutaComChefe = false;  // o chefe só acorda quando a arena tranca
};

// Inimigo chamado por um chefe (a Partida o acrescenta ao andar)
struct Invocacao {
    TipoInimigo tipo;
    glm::vec2 posicao;
};

// Onda pelo chão (pancada do Capataz): fere quem estiver no chão dentro do raio
struct Onda {
    glm::vec2 centro;
    float raio;
    int dano;
    TipoInimigo autor;
};

// Voador que acabou de perceber o jogador (zumbido da vespa, motor do dirigível)
struct Alerta {
    TipoInimigo tipo;
    glm::vec2 posicao;
};

struct EventosIA {
    std::vector<Alerta> alertas;
    std::vector<Invocacao> invocacoes;
    std::vector<Onda> ondas;
    std::vector<Golpe> golpes;  // um por acerto, na ordem (a defesa vale para cada um)
    std::vector<glm::ivec2> comportasParaAbrir;
    std::vector<Projetil> tiros;
};

// Parado → Perseguindo para quem estiver no alcance do barulho, mesmo sem linha de visão
void acordarPorBarulho(std::vector<Inimigo>& v, glm::vec2 origem, float alcance);

EventosIA atualizarInimigos(std::vector<Inimigo>& v, const MundoIA& m, float dt);

// Para os chefes (jogo/Chefes.cpp): andar até o jogador como os de chão, e mover com colisão numa direção
void aproximarDoJogador(Inimigo& i, const MundoIA& m, EventosIA& e, bool ve, float dt);
// Devolve quanto andou de fato (menos que o pedido: bateu em algo)
float moverInimigo(Inimigo& i, const mundo::Grade& g, glm::vec2 deslocamento);

}
