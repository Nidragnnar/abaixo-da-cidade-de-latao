#pragma once
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "mundo/Grade.h"

namespace mundo {

constexpr float QUARTO = 0.25f;  // unidade de altura (as alturas da grade são inteiros de quartos)

// Altura (em blocos) do chão sob o ponto, com a rampa interpolada; do teto
float alturaDoChao(const Grade& g, glm::vec2 p);
float alturaDoTeto(const Grade& g, glm::vec2 p);

// Altura do chão da célula c na borda voltada para `direcao` (±1, 0 ou 0, ±1); na lateral de uma rampa, a média
float chaoNaBorda(const Grade& g, glm::ivec2 c, glm::ivec2 direcao);

// Quanto se pode subir e descer (em quartos) de uma célula para a vizinha, e o que se atravessa
struct RegrasPassagem {
    int subida;
    int descida;
    bool atravessaTrancada;
    bool atravessaElevador;
};
constexpr RegrasPassagem A_PE{1, 8, false, false};       // caminho obrigatório: sem pulo nem queda com dano
constexpr RegrasPassagem ANDARILHO{1, 99, false, false};  // autômatos, atiradores e colossos
constexpr RegrasPassagem SALTADOR{4, 99, false, false};   // ratos sobem 1 bloco pulando
constexpr RegrasPassagem VOADOR{99, 99, false, false};    // vespas e dirigíveis: a altura do chão não importa

// --- Física vertical (jogador e inimigos) ---
constexpr float GRAVIDADE = 24.0f;        // blocos/s²
constexpr float ALTURA_CORPO = 0.7f;
constexpr float VELOCIDADE_PULO = 7.27f;  // √(2 · 24 · 1,1): alcança ~1,1 bloco
constexpr float MESMO_NIVEL = 0.75f;      // diferença de pés para golpear, pegar, separar e empurrar

struct CorpoVertical {
    float pes = 0;         // altura dos pés
    float velocidade = 0;  // vertical, positiva para cima
    bool noChao = true;
    float maisAlto = 0;    // ponto mais alto desde que saiu do chão (para medir a queda)
};

// Chão sob uma caixa de meia-largura `raio`: o do centro e o dos cantos fora de rampas (fica de pé na beirada)
float chaoSobCorpo(const Grade& g, glm::vec2 posicao, float raio);

// Gravidade, chão (degrau e rampa grudam os pés), teto; devolve a queda em blocos ao pousar (0 se não pousou)
// (alturaCorpo: o teto barra a cabeça; os destroços do voador são mais baixos que o jogador)
float atualizarVertical(CorpoVertical& c, const Grade& g, glm::vec2 posicao, float raio, float dt,
                        float alturaCorpo = ALTURA_CORPO);

// Até 2 blocos, nada; depois 5 por meio bloco a mais (arredondando para baixo)
int danoDeQueda(float blocos);

// Da célula a para a vizinha b: b não é parede (nem trancada/elevador, conforme as regras); diferença de chão
// nas bordas comuns dentro de [−descida, subida]; espaço de pelo menos 3 quartos (corpo de 0,7) entre o chão e o teto de b
bool podePassar(const Grade& g, glm::ivec2 a, glm::ivec2 b, const RegrasPassagem& r);

// Distância (em células) de cada célula até `alvo` para quem anda com as regras r; −1 = inalcançável.
// Calculada a partir do alvo pelas arestas ao contrário (vai de b para a quando podePassar(a, b, r)).
std::vector<int> distanciasComAltura(const Grade& g, glm::ivec2 alvo, const RegrasPassagem& r);

// Segmento livre em 3D: barrado por célula sólida (fora a do destino) ou quando passa abaixo do chão ou acima
// do teto de alguma célula do caminho (testado na entrada e na saída de cada célula)
bool temLinhaDeVisao3D(const Grade& g, glm::vec3 de, glm::vec3 para);

}
