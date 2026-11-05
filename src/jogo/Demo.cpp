#include "jogo/Demo.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <glm/geometric.hpp>

#include "jogo/Partida.h"
#include "mundo/Consultas.h"
#include "mundo/Terreno.h"

namespace jogo {

namespace {

constexpr float GIRO_MAXIMO = 0.08f;         // rad por passo
constexpr float ERRO_PARA_ANDAR = 0.5f;      // rad
constexpr float INTERVALO_USAR = 0.5f;
constexpr float CONFERIR_PRESO = 3.0f, MOVIMENTO_MINIMO = 0.3f, DURACAO_DESVIO = 1.0f;
constexpr float ALCANCE_ATAQUE = 1.5f, ARCO_ATAQUE = 0.52f;  // ~30°

// Diferença de ângulo em (−π, π]
float diferenca(float alvo, float atual) {
    float d = std::fmod(alvo - atual, 2.0f * std::numbers::pi_v<float>);
    if (d > std::numbers::pi_v<float>) d -= 2.0f * std::numbers::pi_v<float>;
    if (d <= -std::numbers::pi_v<float>) d += 2.0f * std::numbers::pi_v<float>;
    return d;
}

bool ehComportaFechada(mundo::Celula c) { return c == mundo::Celula::Comporta || c == mundo::Celula::ComportaTrancada; }

}

Comandos PilotoDemo::comandar(const Partida& p, float dt) {
    Comandos c;
    const mundo::Andar& andar = p.andar();
    const mundo::Grade& g = andar.grade;
    const glm::vec2 pos = p.jogador().posicao;
    const float angulo = p.jogador().angulo;
    const glm::ivec2 aqui(glm::floor(pos));
    desdeUsar_ += dt;

    // Alvo: a válvula enquanto estiver no chão, depois o elevador
    const glm::ivec2 alvo = andar.chave ? glm::ivec2(glm::floor(*andar.chave)) : andar.elevador;
    const bool atravessaTrancada = !andar.chave;
    if (alvo != alvoDoCampo_ || p.trocasDeAndar() != andarDoCampo_ || atravessaTrancada != trancaDoCampo_) {
        // A pé, sem pular: o mesmo caminho que o gerador garante
        mundo::RegrasPassagem regras = mundo::A_PE;
        regras.atravessaTrancada = atravessaTrancada;
        regras.atravessaElevador = true;
        regras_ = regras;
        campo_ = mundo::distanciasComAltura(g, alvo, regras);
        alvoDoCampo_ = alvo;
        andarDoCampo_ = p.trocasDeAndar();
        trancaDoCampo_ = atravessaTrancada;
    }

    // Para onde ir: o centro do vizinho mais perto do alvo (ou a própria válvula, já na célula dela)
    const auto distancia = [&](glm::ivec2 q) {
        if (q.x < 0 || q.y < 0 || q.x >= g.largura() || q.y >= g.altura()) return -1;
        return campo_[static_cast<size_t>(q.y * g.largura() + q.x)];
    };
    glm::vec2 destino = pos;
    glm::ivec2 proxima = aqui;
    const int dAqui = distancia(aqui);
    if (andar.chave && aqui == alvo) {
        destino = *andar.chave;
    } else if (dAqui > 0) {
        for (const glm::ivec2 passo : {glm::ivec2(1, 0), glm::ivec2(-1, 0), glm::ivec2(0, 1), glm::ivec2(0, -1)}) {
            const int d = distancia(aqui + passo);
            if (d < 0 || d >= dAqui || !mundo::podePassar(g, aqui, aqui + passo, regras_)) continue;
            if (proxima == aqui || d < distancia(proxima)) proxima = aqui + passo;
        }
        destino = glm::vec2(proxima) + 0.5f;
    }

    const glm::vec2 ate = destino - pos;
    const float erro = glm::length(ate) > 1e-3f ? diferenca(std::atan2(ate.y, ate.x), angulo) : 0.0f;
    c.giro = std::clamp(erro, -GIRO_MAXIMO, GIRO_MAXIMO);

    // Comporta ou elevador na próxima célula: virar e usar; senão, andar
    const mundo::Celula seguinte = g.em(proxima.x, proxima.y);
    const bool usarAFrente = proxima != aqui && (ehComportaFechada(seguinte) || seguinte == mundo::Celula::Elevador);
    if (usarAFrente) {
        if (std::abs(erro) < 0.3f && desdeUsar_ >= INTERVALO_USAR) {
            c.usar = true;
            desdeUsar_ = 0.0f;
        }
        c.frente = std::abs(erro) < ERRO_PARA_ANDAR && !ehComportaFechada(seguinte) && seguinte != mundo::Celula::Elevador
                       ? 1.0f
                       : 0.0f;
    } else if (std::abs(erro) < ERRO_PARA_ANDAR) {
        c.frente = 1.0f;
    }

    // Inimigo vivo colado e à frente: bater
    for (const Inimigo& i : p.inimigos()) {
        if (i.estado == EstadoInimigo::Destruido) continue;
        const glm::vec2 d = i.posicao - pos;
        if (glm::length(d) > ALCANCE_ATAQUE) continue;
        if (std::abs(i.corpo.pes - p.jogador().corpo.pes) >= mundo::MESMO_NIVEL) continue;  // voador no alto: não alcança
        if (std::abs(diferenca(std::atan2(d.y, d.x), angulo)) > ARCO_ATAQUE) continue;
        if (!mundo::temLinhaDeVisao(g, pos, i.posicao)) continue;
        c.atacar = true;
        break;
    }

    // Preso (quina, inimigo no caminho): anda de lado um pouco, alternando o lado
    relogioPreso_ += dt;
    if (relogioPreso_ >= CONFERIR_PRESO) {
        if (glm::distance(pos, posicaoConferida_) < MOVIMENTO_MINIMO && !usarAFrente) {
            desvio_ = DURACAO_DESVIO;
            lado_ = -lado_;
        }
        posicaoConferida_ = pos;
        relogioPreso_ = 0.0f;
    }
    if (desvio_ > 0.0f) {
        desvio_ -= dt;
        c.lado = lado_;
    }
    return c;
}

}
