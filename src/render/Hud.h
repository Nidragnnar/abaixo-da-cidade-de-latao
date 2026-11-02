#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

#include "jogo/Armas.h"
#include "render/Primitivas2D.h"

namespace render {

struct ElementosHud {
    std::vector<VerticeCor> cores;  // barras e painéis; o flash vermelho vem por último
    std::vector<VerticeTex> texto;  // glifos da fonte
    std::vector<VerticeTex> arma;   // arma na mão (array das armas)
};

struct EstadoHud {
    int vida, vidaMaxima;
    jogo::ArmaAtiva arma;
    int carga, reserva;
    bool recarregando;
    float animacao;        // > 0 enquanto o golpe/tiro aparece
    float tempoDesdeDano;
    std::string_view mensagem;
    int andar;
    float tempo;           // relógio do jogo (reservado para animações do HUD)
};

// Camadas do array das armas na mão
enum QuadroArma : int { ChaveParada = 0, ChaveGolpe = 1, RevolverParado = 2, RevolverTiro = 3, RevolverRecarga = 4 };

// Barra de vida, munição, andar, mensagem, arma na mão e flash vermelho (seção 8 da spec, versão mínima)
ElementosHud construirHud(const EstadoHud& e, int telaL, int telaA);

struct DadosFim {
    int andar, nivel, inimigos, engrenagens, pontuacao;
    uint32_t semente;
    std::string_view causa;
    int posicaoPlacar;  // 0 se não entrou no placar
};

// Painel escuro com o resumo da partida (seção 7 da spec)
ElementosHud construirTelaFim(const DadosFim& d, int telaL, int telaA);

}
