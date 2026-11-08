#pragma once
#include <array>
#include <cstdint>
#include <string_view>

#include <glm/vec2.hpp>

#include "jogo/Atalhos.h"
#include "ui/Menus.h"

namespace ui {

// Controle de videogame (esquema fixo da spec, seção 4): traduz o estado já lido do SDL em comandos, eventos do
// jogo, navegação dos menus e cursor do inventário (puro, sem SDL)
enum class BotaoControle : uint8_t { A, B, X, Y, View, Start, LB, RB, Cima, Baixo, Esquerda, Direita };
constexpr int TOTAL_BOTOES_CONTROLE = 12;

struct EstadoControle {
    bool ligado = false;
    float esqX = 0, esqY = 0, dirX = 0, dirY = 0;  // −1…1, y para baixo positivo (como o SDL)
    float gatilhoDir = 0;                           // 0…1
    std::array<bool, TOTAL_BOTOES_CONTROLE> botoes{};
    bool botao(BotaoControle b) const { return botoes[static_cast<size_t>(b)]; }
};

constexpr float ZONA_MORTA = 0.2f;       // analógico gasto (drift) abaixo disso não mexe
constexpr float VELOCIDADE_GIRO = 3.0f;  // rad/s com o analógico direito no fim e sensibilidade 5
constexpr float VELOCIDADE_CURSOR = 700.0f;  // px/s do cursor do inventário

// |v| < 0,2 → 0; o resto vai de 0 a 1
float aplicarZonaMorta(float v);

struct MovimentoControle {
    float frente = 0, lado = 0, giro = 0, inclinar = 0;  // giro e inclinar em radianos neste quadro
    bool atacar = false;                                 // RT passou da metade
};
MovimentoControle movimentoDoControle(const EstadoControle& e, float dt, int sensibilidade, bool inverter);

// Botões que desceram entre os dois estados (A pular, X usar, B recarregar, Y trocar, View inventário, LB mapa,
// Start pausa, cruz 1 cima / 2 direita / 3 baixo / 4 esquerda)
struct EventosControle {
    bool pular = false, usar = false, recarregar = false, trocarArma = false, inventario = false, mapa = false,
         pausa = false;
    int usarRapido = 0;
};
EventosControle eventosDoControle(const EstadoControle& antes, const EstadoControle& agora);

// Cruz ou analógico esquerdo (uma vez por cruzamento de 0,5) movem e ajustam; A confirma; B ou Start voltam
EntradaMenu menuDoControle(const EstadoControle& antes, const EstadoControle& agora);

// Cursor do inventário: analógico esquerdo (ou o direito, com o esquerdo parado), preso dentro da janela
glm::vec2 moverCursor(glm::vec2 cursor, const EstadoControle& e, float dt, glm::vec2 tamanhoJanela);

// Para os textos que citam teclas quando o jogador está no controle ("B para recarregar")
std::string_view nomeNoControle(jogo::AcaoJogo a);

}
