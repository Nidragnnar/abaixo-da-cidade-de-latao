#include "jogo/IA.h"

#include <cmath>

#include <glm/geometric.hpp>

#include "mundo/Aleatorio.h"
#include "mundo/Colisao.h"
#include "mundo/Consultas.h"

namespace jogo {

namespace {

void mudar(Inimigo& i, EstadoInimigo e) {
    i.estado = e;
    i.tempoNoEstado = 0;
}

glm::ivec2 celulaDe(glm::vec2 p) { return {static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y))}; }

void mover(Inimigo& i, const mundo::Grade& g, glm::vec2 direcao, float dt) {
    if (glm::length(direcao) < 1e-5f) return;
    i.posicao = mundo::moverComColisao(g, i.posicao, glm::normalize(direcao) * dados(i.tipo).velocidade * dt, RAIO_INIMIGO);
}

// Segue o campo de fluxo: vizinha com distância menor (aproximar) ou maior (recuar).
// Comporta fechada no caminho vira pedido de abertura e o inimigo espera.
void andarPeloCampo(Inimigo& i, const MundoIA& m, EventosIA& e, bool aproximar, float dt) {
    const mundo::Grade& g = m.grade;
    const glm::ivec2 c = celulaDe(i.posicao);
    auto distancia = [&](glm::ivec2 p) {
        if (p.x < 0 || p.y < 0 || p.x >= g.largura() || p.y >= g.altura()) return -1;
        return m.campo[static_cast<size_t>(p.y) * g.largura() + p.x];
    };
    const int aqui = distancia(c);
    if (aqui < 0) return;  // jogador fora de alcance (ex.: atrás da tranca)
    if (aproximar && aqui == 0) {
        mover(i, g, m.jogador - i.posicao, dt);
        return;
    }
    constexpr glm::ivec2 VIZINHOS[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    glm::ivec2 melhor = c;
    int melhorD = aqui;
    for (const auto& v : VIZINHOS) {
        const int d = distancia(c + v);
        if (d < 0) continue;
        if (aproximar ? d < melhorD : d > melhorD) melhor = c + v, melhorD = d;
    }
    if (melhor == c) return;
    if (g.em(melhor.x, melhor.y) == mundo::Celula::Comporta) {
        e.comportasParaAbrir.push_back(melhor);
        return;
    }
    mover(i, g, glm::vec2(melhor) + 0.5f - i.posicao, dt);
}

// Aproxima em linha reta quando vê o jogador e nada bloqueia a passagem (o elevador é vazado); senão pelo campo
void aproximar(Inimigo& i, const MundoIA& m, EventosIA& e, bool ve, float dt) {
    if (ve && mundo::temPassagemLivre(m.grade, i.posicao, m.jogador)) mover(i, m.grade, m.jogador - i.posicao, dt);
    else andarPeloCampo(i, m, e, true, dt);
}

// Inimigos se afastam uns dos outros e nunca entram no jogador (quem vem atrás não empurra o da frente através dele)
void separar(std::vector<Inimigo>& v, const mundo::Grade& g, glm::vec2 jogador, float raioJogador) {
    for (size_t a = 0; a < v.size(); ++a)
        for (size_t b = a + 1; b < v.size(); ++b) {
            if (v[a].estado == EstadoInimigo::Destruido || v[b].estado == EstadoInimigo::Destruido) continue;
            glm::vec2 d = v[b].posicao - v[a].posicao;
            const float dist = glm::length(d);
            if (dist >= 2 * RAIO_INIMIGO) continue;
            d = dist > 1e-5f ? d / dist : glm::vec2(1.0f, 0.0f);
            const glm::vec2 metade = d * (2 * RAIO_INIMIGO - dist) * 0.5f;
            v[a].posicao = mundo::moverComColisao(g, v[a].posicao, -metade, RAIO_INIMIGO);
            v[b].posicao = mundo::moverComColisao(g, v[b].posicao, metade, RAIO_INIMIGO);
        }
    const float minimo = RAIO_INIMIGO + raioJogador;
    for (Inimigo& i : v) {
        if (i.estado == EstadoInimigo::Destruido) continue;
        glm::vec2 d = i.posicao - jogador;
        const float dist = glm::length(d);
        if (dist >= minimo) continue;
        d = dist > 1e-5f ? d / dist : glm::vec2(1.0f, 0.0f);
        i.posicao = mundo::moverComColisao(g, i.posicao, d * (minimo - dist), RAIO_INIMIGO);
    }
}

}

void acordarPorBarulho(std::vector<Inimigo>& v, glm::vec2 origem, float alcance) {
    for (Inimigo& i : v)
        if (i.estado == EstadoInimigo::Parado && glm::distance(i.posicao, origem) <= alcance)
            mudar(i, EstadoInimigo::Perseguindo);
}

EventosIA atualizarInimigos(std::vector<Inimigo>& v, const MundoIA& m, float dt) {
    EventosIA e;
    for (Inimigo& i : v) {
        i.tempoNoEstado += dt;
        if (i.estado == EstadoInimigo::Destruido) continue;
        const DadosInimigo& d = dados(i.tipo);

        if (i.empurrao != glm::vec2(0.0f)) {
            i.posicao = mundo::moverComColisao(m.grade, i.posicao, i.empurrao, RAIO_INIMIGO);
            i.empurrao = glm::vec2(0.0f);
        }

        const float dist = glm::distance(i.posicao, m.jogador);
        const bool ve = dist <= ALCANCE_VISAO && mundo::temLinhaDeVisao(m.grade, i.posicao, m.jogador);

        switch (i.estado) {
            case EstadoInimigo::Parado:
                if (ve) mudar(i, EstadoInimigo::Perseguindo);
                break;
            case EstadoInimigo::Perseguindo:
                if (!d.aDistancia) {
                    if (ve && dist <= INICIO_GOLPE) mudar(i, EstadoInimigo::Preparando);
                    else aproximar(i, m, e, ve, dt);
                } else if (ve && dist < DISTANCIA_MIN_ATIRADOR) {
                    mover(i, m.grade, i.posicao - m.jogador, dt);  // recua
                } else if (ve && dist <= DISTANCIA_MAX_ATIRADOR) {
                    mudar(i, EstadoInimigo::Preparando);  // para e mira
                } else {
                    aproximar(i, m, e, ve, dt);
                }
                break;
            case EstadoInimigo::Preparando:
                if (i.tempoNoEstado < d.preparo) break;
                if (!d.aDistancia) {
                    if (dist <= ALCANCE_GOLPE && ve) {
                        e.dano += mundo::sortear(m.rng, i.danoMin, i.danoMax);
                        e.autor = i.tipo;
                    }
                } else if (ve && dist > 1e-4f) {  // sem direção se estiver exatamente no jogador
                    const glm::vec2 direcao = glm::normalize(m.jogador - i.posicao);
                    e.tiros.push_back({i.posicao + direcao * (RAIO_INIMIGO + 0.05f), direcao * VELOCIDADE_PROJETIL,
                                       mundo::sortear(m.rng, i.danoMin, i.danoMax), i.tipo});
                }
                mudar(i, EstadoInimigo::Recuperando);
                break;
            case EstadoInimigo::Recuperando:
                if (i.tempoNoEstado >= d.recuperacao) mudar(i, EstadoInimigo::Perseguindo);
                break;
            case EstadoInimigo::Destruido: break;
        }
    }
    separar(v, m.grade, m.jogador, m.raioJogador);
    return e;
}

}
