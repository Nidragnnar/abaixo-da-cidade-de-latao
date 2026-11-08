#include "ui/Controle.h"

#include <algorithm>
#include <cmath>

#include <glm/common.hpp>

namespace ui {

namespace {

// Depois da zona morta: sinal × v², fino perto do centro e rápido nas pontas
float curva(float v) {
    const float z = aplicarZonaMorta(v);
    return z * std::abs(z);
}

bool desceu(const EstadoControle& antes, const EstadoControle& agora, BotaoControle b) {
    return agora.botao(b) && !antes.botao(b);
}

// −1 / +1 quando o eixo passa de 0,5 para um lado (uma vez por cruzamento)
int cruzou(float antes, float agora) {
    if (agora > 0.5f && antes <= 0.5f) return 1;
    if (agora < -0.5f && antes >= -0.5f) return -1;
    return 0;
}

}

float aplicarZonaMorta(float v) {
    const float a = std::abs(v);
    if (a < ZONA_MORTA) return 0.0f;
    return std::copysign(std::min(1.0f, (a - ZONA_MORTA) / (1.0f - ZONA_MORTA)), v);
}

MovimentoControle movimentoDoControle(const EstadoControle& e, float dt, int sensibilidade, bool inverter) {
    MovimentoControle m;
    if (!e.ligado) return m;
    m.frente = -aplicarZonaMorta(e.esqY);
    m.lado = aplicarZonaMorta(e.esqX);
    // Mesma escala da sensibilidade do mouse: 1,25 por ponto acima ou abaixo de 5
    const float giro = VELOCIDADE_GIRO * std::pow(1.25f, static_cast<float>(sensibilidade - 5)) * dt;
    m.giro = curva(e.dirX) * giro;
    m.inclinar = -curva(e.dirY) * giro * (inverter ? -1.0f : 1.0f);
    m.atacar = e.gatilhoDir > 0.5f;
    return m;
}

EventosControle eventosDoControle(const EstadoControle& antes, const EstadoControle& agora) {
    EventosControle v;
    if (!agora.ligado) return v;
    const auto d = [&](BotaoControle b) { return desceu(antes, agora, b); };
    v.pular = d(BotaoControle::A);
    v.usar = d(BotaoControle::X);
    v.recarregar = d(BotaoControle::B);
    v.trocarArma = d(BotaoControle::Y);
    v.inventario = d(BotaoControle::View);
    v.mapa = d(BotaoControle::LB);
    v.pausa = d(BotaoControle::Start);
    if (d(BotaoControle::Cima)) v.usarRapido = 1;
    else if (d(BotaoControle::Direita)) v.usarRapido = 2;
    else if (d(BotaoControle::Baixo)) v.usarRapido = 3;
    else if (d(BotaoControle::Esquerda)) v.usarRapido = 4;
    return v;
}

EntradaMenu menuDoControle(const EstadoControle& antes, const EstadoControle& agora) {
    EntradaMenu m;
    if (!agora.ligado) return m;
    const auto d = [&](BotaoControle b) { return desceu(antes, agora, b); };
    m.mover = cruzou(antes.esqY, agora.esqY);
    m.ajustar = cruzou(antes.esqX, agora.esqX);
    if (d(BotaoControle::Cima)) m.mover = -1;
    if (d(BotaoControle::Baixo)) m.mover = 1;
    if (d(BotaoControle::Esquerda)) m.ajustar = -1;
    if (d(BotaoControle::Direita)) m.ajustar = 1;
    m.confirmar = d(BotaoControle::A);
    m.voltar = d(BotaoControle::B) || d(BotaoControle::Start);
    return m;
}

glm::vec2 moverCursor(glm::vec2 cursor, const EstadoControle& e, float dt, glm::vec2 tamanhoJanela) {
    if (!e.ligado) return cursor;
    glm::vec2 v{curva(e.esqX), curva(e.esqY)};
    if (v == glm::vec2(0.0f)) v = {curva(e.dirX), curva(e.dirY)};
    if (v == glm::vec2(0.0f)) return cursor;
    cursor += v * VELOCIDADE_CURSOR * dt;
    return glm::clamp(cursor, glm::vec2(0.0f), glm::max(tamanhoJanela - 1.0f, glm::vec2(0.0f)));
}

std::string_view nomeNoControle(jogo::AcaoJogo a) {
    using jogo::AcaoJogo;
    switch (a) {
        case AcaoJogo::Pular: return "A";
        case AcaoJogo::Usar: return "X";
        case AcaoJogo::TrocarArma: return "Y";
        case AcaoJogo::Recarregar: return "B";
        case AcaoJogo::Inventario: return "View";
        case AcaoJogo::Mapa: return "LB";
        case AcaoJogo::Item1: return "Cima";
        case AcaoJogo::Item2: return "Direita";
        case AcaoJogo::Item3: return "Baixo";
        case AcaoJogo::Item4: return "Esquerda";
        case AcaoJogo::Atacar: return "RT";
        default: return "Analógico esquerdo";
    }
}

}
