#include "jogo/Chefes.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "mundo/Aleatorio.h"
#include "mundo/Terreno.h"

namespace jogo {

namespace {

void mudar(Inimigo& i, EstadoInimigo e) {
    i.estado = e;
    i.tempoNoEstado = 0;
}

// Células de chão livres em volta do chefe (anéis de 1 e 2 células), dentro da arena se houver: onde nascem os
// inimigos chamados
std::vector<glm::vec2> lugaresEmVolta(const Inimigo& i, const MundoIA& m, int quantos) {
    std::vector<glm::vec2> lugares;
    const glm::ivec2 c(glm::floor(i.posicao));
    for (int anel = 1; anel <= 2 && static_cast<int>(lugares.size()) < quantos; ++anel)
        for (int dy = -anel; dy <= anel && static_cast<int>(lugares.size()) < quantos; ++dy)
            for (int dx = -anel; dx <= anel && static_cast<int>(lugares.size()) < quantos; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != anel) continue;
                const glm::ivec2 p = c + glm::ivec2(dx, dy);
                if (m.grade.em(p.x, p.y) != mundo::Celula::Chao) continue;
                if (m.arena && !m.arena->contem(p.x, p.y)) continue;
                const glm::vec2 centro = glm::vec2(p) + 0.5f;
                if (glm::distance(centro, i.posicao) < raioDe(i) + 0.3f) continue;  // não nasce dentro do chefe
                lugares.push_back(centro);
            }
    return lugares;
}

bool encostou(const Inimigo& i, const MundoIA& m) {
    return glm::distance(i.posicao, m.jogador) <= raioDe(i) + m.raioJogador + 0.05f &&
           std::abs(i.corpo.pes - m.pesJogador) < mundo::MESMO_NIVEL;
}

// Rainha dos Ratos: persegue; mira parada e corre em linha reta (tonta se bate na parede); chama ratos
void atualizarRainha(Inimigo& i, const MundoIA& m, EventosIA& e, bool ve, float dt) {
    const DadosInimigo& d = dados(i.tipo);
    i.recarga += dt;
    i.recarga2 += dt;
    if (i.recarga2 >= NINHADA_INTERVALO) {
        i.recarga2 = 0;
        const int quantos = std::min(NINHADA_TAMANHO, MAXIMO_RATOS - m.ratosVivos);
        for (const glm::vec2 p : lugaresEmVolta(i, m, std::max(0, quantos))) e.invocacoes.push_back({TipoInimigo::Rato, p});
    }
    switch (i.estado) {
        case EstadoInimigo::Perseguindo:
            if (ve && glm::distance(i.posicao, m.jogador) <= INVESTIDA_ALCANCE && i.recarga >= INVESTIDA_INTERVALO) {
                mudar(i, EstadoInimigo::Preparando);
                i.direcao = glm::vec2(0.0f);
            } else {
                aproximarDoJogador(i, m, e, ve, dt);
            }
            break;
        case EstadoInimigo::Preparando: {
            if (i.tempoNoEstado < INVESTIDA_MIRA) break;  // mirando
            if (i.direcao == glm::vec2(0.0f)) {             // fixa a direção no fim da mira
                const glm::vec2 ate = m.jogador - i.posicao;
                i.direcao = glm::length(ate) > 1e-4f ? glm::normalize(ate) : glm::vec2(1.0f, 0.0f);
            }
            const float pedido = INVESTIDA_VELOCIDADE * dt;
            const float andou = moverInimigo(i, m.grade, i.direcao * pedido);
            if (encostou(i, m)) {
                e.golpes.push_back({mundo::sortear(m.rng, i.danoMin, i.danoMax), i.tipo});
                mudar(i, EstadoInimigo::Perseguindo);
                i.recarga = 0;
            } else if (andou < pedido * 0.5f) {
                mudar(i, EstadoInimigo::Recuperando);  // bateu: tonta
            } else if (i.tempoNoEstado >= INVESTIDA_MIRA + INVESTIDA_DURACAO) {
                mudar(i, EstadoInimigo::Perseguindo);
                i.recarga = 0;
            }
            break;
        }
        case EstadoInimigo::Recuperando:
            if (i.tempoNoEstado >= d.recuperacao) {
                mudar(i, EstadoInimigo::Perseguindo);
                i.recarga = 0;
            }
            break;
        default: break;
    }
}

// Tiro do chefe saindo da frente dele, na altura do peito, em direção ao centro do jogador, girado `graus` no plano
Projetil tiroDoChefe(const Inimigo& i, const MundoIA& m, float graus) {
    const glm::vec2 ate = m.jogador - i.posicao;
    const float distancia = glm::length(ate);
    const glm::vec2 base = distancia > 1e-4f ? ate / distancia : glm::vec2(1.0f, 0.0f);
    const float a = glm::radians(graus);
    const glm::vec2 plano{base.x * std::cos(a) - base.y * std::sin(a), base.x * std::sin(a) + base.y * std::cos(a)};
    const glm::vec2 boca = i.posicao + plano * (raioDe(i) + 0.05f);
    const glm::vec3 origem{boca.x, i.corpo.pes + dados(i.tipo).altura * 0.6f, boca.y};
    const float alvoY = m.pesJogador + mundo::ALTURA_CORPO / 2;
    const float subida = (alvoY - origem.y) / std::max(distancia - raioDe(i), 0.5f);
    const glm::vec3 direcao = glm::normalize(glm::vec3(plano.x, subida, plano.y));
    return {origem, direcao * VELOCIDADE_PROJETIL, mundo::sortear(m.rng, i.danoMin, i.danoMax), i.tipo};
}

// Capataz a Vapor: lento; perto, prepara a pancada (onda pelo chão); vendo de longe, solta o leque de vapor
void atualizarCapataz(Inimigo& i, const MundoIA& m, EventosIA& e, bool ve, float dt) {
    i.recarga += dt;
    i.recarga2 += dt;
    const float distancia = glm::distance(i.posicao, m.jogador);
    switch (i.estado) {
        case EstadoInimigo::Perseguindo:
            if (distancia <= PANCADA_ALCANCE && i.recarga2 >= PANCADA_INTERVALO) {
                mudar(i, EstadoInimigo::Preparando);
                i.ataque = 2;
            } else if (ve && distancia <= JATO_ALCANCE && i.recarga >= JATO_INTERVALO) {
                mudar(i, EstadoInimigo::Preparando);
                i.ataque = 1;
            } else {
                aproximarDoJogador(i, m, e, ve, dt);
            }
            break;
        case EstadoInimigo::Preparando:
            if (i.ataque == 1 && i.tempoNoEstado >= JATO_PREPARO) {
                for (const float graus : JATO_ANGULOS) e.tiros.push_back(tiroDoChefe(i, m, graus));
                i.recarga = 0;
                mudar(i, EstadoInimigo::Perseguindo);
            } else if (i.ataque == 2 && i.tempoNoEstado >= PANCADA_PREPARO) {
                e.ondas.push_back({i.posicao, PANCADA_ALCANCE, mundo::sortear(m.rng, PANCADA_DANO_MIN, PANCADA_DANO_MAX), i.tipo});
                i.recarga2 = 0;
                mudar(i, EstadoInimigo::Perseguindo);
            }
            break;
        default: break;
    }
}

// Grande Máquina: fixa; rajadas de 3 tiros; na fase 2 (metade da vida) rajadas mais seguidas e voadores chamados
void atualizarMaquina(Inimigo& i, const MundoIA& m, EventosIA& e, bool ve, float dt) {
    const bool fase2 = i.vida * 2 <= i.vidaMaxima;
    i.recarga += dt;
    if (fase2) {
        i.recarga2 += dt;
        if (i.recarga2 >= CHAMADO_INTERVALO) {
            i.recarga2 = 0;
            const bool vespas = i.chamados % 2 == 0;
            const int quantos = std::min(vespas ? 2 : 1, MAXIMO_VOADORES - m.voadoresVivos);
            const auto lugares = lugaresEmVolta(i, m, std::max(0, quantos));
            for (const glm::vec2 p : lugares) e.invocacoes.push_back({vespas ? TipoInimigo::Vespa : TipoInimigo::Dirigivel, p});
            if (!lugares.empty()) ++i.chamados;
        }
    }
    switch (i.estado) {
        case EstadoInimigo::Perseguindo:
            if (ve && i.recarga >= (fase2 ? RAJADA_INTERVALO_FASE2 : RAJADA_INTERVALO)) {
                mudar(i, EstadoInimigo::Preparando);
                i.ataque = RAJADA_TIROS;
            }
            break;
        case EstadoInimigo::Preparando:
            // Um tiro a cada RAJADA_ESPACO, o primeiro na hora
            while (i.ataque > 0 && i.tempoNoEstado + 1e-4f >= (RAJADA_TIROS - i.ataque) * RAJADA_ESPACO) {
                if (ve) e.tiros.push_back(tiroDoChefe(i, m, 0.0f));
                --i.ataque;
            }
            if (i.ataque == 0) {
                i.recarga = 0;
                mudar(i, EstadoInimigo::Perseguindo);
            }
            break;
        default: break;
    }
}

}

bool ondaAtinge(const Onda& o, glm::vec2 jogador, bool noChao) {
    return noChao && glm::distance(jogador, o.centro) < o.raio;
}

void atualizarChefe(Inimigo& i, const MundoIA& m, EventosIA& e, bool ve, float dt) {
    if (i.estado == EstadoInimigo::Parado) {
        if (ve && m.lutaComChefe) mudar(i, EstadoInimigo::Perseguindo);  // só depois que a arena tranca
        return;
    }
    switch (i.tipo) {
        case TipoInimigo::RainhaRatos: atualizarRainha(i, m, e, ve, dt); break;
        case TipoInimigo::Capataz: atualizarCapataz(i, m, e, ve, dt); break;
        case TipoInimigo::GrandeMaquina: atualizarMaquina(i, m, e, ve, dt); break;
        default: break;
    }
}

}
