#pragma once
#include <random>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Comandos.h"
#include "jogo/Inimigos.h"
#include "jogo/Inventario.h"
#include "jogo/Itens.h"
#include "mundo/Grade.h"

namespace jogo {

// Qual das duas armas equipadas está na mão
enum class ArmaAtiva { Branca, Fogo };

constexpr float DURACAO_ANIMACAO_ARMA = 0.2f;

// Munição de reserva por tipo (fora do inventário)
struct Municao {
    int balas = 12, cartuchos = 0, celulas = 0;
    int& de(TipoMunicao t);
    int de(TipoMunicao t) const;
};

int maximoMunicao(TipoMunicao t);  // balas 48, cartuchos 20, células 12

struct Armas {
    ArmaAtiva ativa = ArmaAtiva::Branca;
    Municao municao;
    float intervalo = 0;  // espera até o próximo ataque
    float recarga = 0;    // > 0 enquanto recarrega
    float animacao = 0;   // > 0 enquanto o golpe/tiro aparece na mão
};

struct ResultadoAtaque {
    int acertos = 0;
    int destruidos = 0;
    float barulho = 0;  // alcance do barulho (acorda inimigos parados)
    bool disparou = false;
    std::string mensagem;
};

// Troca, recarga e ataque da arma equipada ativa (seção 4 da spec), com dano base do nível e bônus do item;
// a inclinação do olhar mira as armas de fogo na vertical
ResultadoAtaque atualizarArmas(Armas& a, Inventario& inv, const Atributos& atr, const Comandos& c, glm::vec2 posicao,
                               float angulo, float inclinacao, const mundo::Grade& g, std::vector<Inimigo>& inimigos,
                               std::mt19937& rng, float dt);

}
