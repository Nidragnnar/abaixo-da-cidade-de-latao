#include "jogo/IA.h"

#include <cmath>

#include <glm/geometric.hpp>

#include "mundo/Aleatorio.h"
#include "mundo/Colisao.h"
#include "mundo/Consultas.h"
#include "mundo/Terreno.h"

namespace jogo {

namespace {

void mudar(Inimigo& i, EstadoInimigo e) {
    i.estado = e;
    i.tempoNoEstado = 0;
}

glm::ivec2 celulaDe(glm::vec2 p) { return {static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y))}; }

using mundo::MESMO_NIVEL;
constexpr float OLHOS_INIMIGO = 0.5f, CENTRO_JOGADOR = 0.35f;

bool mesmoNivel(float a, float b) { return std::abs(a - b) < MESMO_NIVEL; }

// Sobe degrau sozinho só com os pés no chão
float subidaDe(const Inimigo& i) { return i.corpo.noChao && i.corpo.velocidade <= 0.0f ? mundo::QUARTO : 0.0f; }

void mover(Inimigo& i, const mundo::Grade& g, glm::vec2 direcao, float dt) {
    if (glm::length(direcao) < 1e-5f) return;
    i.posicao = mundo::moverComColisao(g, i.posicao, glm::normalize(direcao) * dados(i.tipo).velocidade * dt, RAIO_INIMIGO,
                                       i.corpo.pes, subidaDe(i));
}

// Segue o campo de fluxo: vizinha com distância menor (aproximar) ou maior (recuar).
// Comporta fechada no caminho vira pedido de abertura e o inimigo espera.
void andarPeloCampo(Inimigo& i, const MundoIA& m, EventosIA& e, bool aproximar, float dt) {
    const mundo::Grade& g = m.grade;
    const glm::ivec2 c = celulaDe(i.posicao);
    const bool saltador = i.tipo == TipoInimigo::Rato && m.campoSaltador;
    const std::vector<int>& campo = saltador ? *m.campoSaltador : m.campo;
    auto distancia = [&](glm::ivec2 p) {
        if (p.x < 0 || p.y < 0 || p.x >= g.largura() || p.y >= g.altura()) return -1;
        return campo[static_cast<size_t>(p.y) * g.largura() + p.x];
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
        // Só vizinhas por onde dá para ir daqui (a plataforma ao lado pode estar perto no campo, mas alta demais)
        if (!mundo::podePassar(g, c, c + v, saltador ? mundo::SALTADOR : mundo::ANDARILHO)) continue;
        if (aproximar ? d < melhorD : d > melhorD) melhor = c + v, melhorD = d;
    }
    if (melhor == c) return;
    if (g.em(melhor.x, melhor.y) == mundo::Celula::Comporta) {
        e.comportasParaAbrir.push_back(melhor);
        return;
    }
    // O rato pula quando o próximo passo sobe mais que um degrau
    if (saltador && i.corpo.noChao && mundo::chaoNaBorda(g, melhor, c - melhor) > i.corpo.pes + mundo::QUARTO + 1e-3f)
        i.corpo.velocidade = mundo::VELOCIDADE_PULO;
    mover(i, g, glm::vec2(melhor) + 0.5f - i.posicao, dt);
}

// Aproxima em linha reta quando vê o jogador e nada bloqueia a passagem (o elevador é vazado); senão pelo campo
void aproximar(Inimigo& i, const MundoIA& m, EventosIA& e, bool ve, float dt) {
    if (ve && mesmoNivel(i.corpo.pes, m.pesJogador) && mundo::temPassagemLivre(m.grade, i.posicao, m.jogador))
        mover(i, m.grade, m.jogador - i.posicao, dt);
    else andarPeloCampo(i, m, e, true, dt);
}

// Inimigos se afastam uns dos outros e nunca entram no jogador (quem vem atrás não empurra o da frente através dele)
void separar(std::vector<Inimigo>& v, const mundo::Grade& g, glm::vec2 jogador, float pesJogador, float raioJogador) {
    for (size_t a = 0; a < v.size(); ++a)
        for (size_t b = a + 1; b < v.size(); ++b) {
            if (v[a].estado == EstadoInimigo::Destruido || v[b].estado == EstadoInimigo::Destruido) continue;
            if (!mesmoNivel(v[a].corpo.pes, v[b].corpo.pes)) continue;  // um em cima da plataforma, outro embaixo
            glm::vec2 d = v[b].posicao - v[a].posicao;
            const float dist = glm::length(d);
            if (dist >= 2 * RAIO_INIMIGO) continue;
            d = dist > 1e-5f ? d / dist : glm::vec2(1.0f, 0.0f);
            const glm::vec2 metade = d * (2 * RAIO_INIMIGO - dist) * 0.5f;
            v[a].posicao = mundo::moverComColisao(g, v[a].posicao, -metade, RAIO_INIMIGO, v[a].corpo.pes, subidaDe(v[a]));
            v[b].posicao = mundo::moverComColisao(g, v[b].posicao, metade, RAIO_INIMIGO, v[b].corpo.pes, subidaDe(v[b]));
        }
    const float minimo = RAIO_INIMIGO + raioJogador;
    for (Inimigo& i : v) {
        if (i.estado == EstadoInimigo::Destruido || !mesmoNivel(i.corpo.pes, pesJogador)) continue;
        glm::vec2 d = i.posicao - jogador;
        const float dist = glm::length(d);
        if (dist >= minimo) continue;
        d = dist > 1e-5f ? d / dist : glm::vec2(1.0f, 0.0f);
        i.posicao = mundo::moverComColisao(g, i.posicao, d * (minimo - dist), RAIO_INIMIGO, i.corpo.pes, subidaDe(i));
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
            i.posicao = mundo::moverComColisao(m.grade, i.posicao, i.empurrao, RAIO_INIMIGO, i.corpo.pes, subidaDe(i));
            i.empurrao = glm::vec2(0.0f);
        }

        const float dist = glm::distance(i.posicao, m.jogador);
        const bool nivel = mesmoNivel(i.corpo.pes, m.pesJogador);
        const bool ve = dist <= ALCANCE_VISAO &&
                        mundo::temLinhaDeVisao3D(m.grade, {i.posicao.x, i.corpo.pes + OLHOS_INIMIGO, i.posicao.y},
                                                 {m.jogador.x, m.pesJogador + OLHOS_INIMIGO, m.jogador.y});  // olho no olho

        switch (i.estado) {
            case EstadoInimigo::Parado:
                if (ve) mudar(i, EstadoInimigo::Perseguindo);
                break;
            case EstadoInimigo::Perseguindo:
                if (!d.aDistancia) {
                    if (ve && nivel && i.corpo.noChao && dist <= INICIO_GOLPE) mudar(i, EstadoInimigo::Preparando);  // não no meio do pulo
                    else aproximar(i, m, e, ve, dt);
                } else if (ve && dist < DISTANCIA_MIN_ATIRADOR) {
                    const glm::vec2 antes = i.posicao;
                    mover(i, m.grade, i.posicao - m.jogador, dt);  // recua
                    // Encurralado (o recuo não saiu do lugar): atira assim mesmo
                    if (glm::distance(antes, i.posicao) < 1e-4f) mudar(i, EstadoInimigo::Preparando);
                } else if (ve && dist <= DISTANCIA_MAX_ATIRADOR) {
                    mudar(i, EstadoInimigo::Preparando);  // para e mira
                } else {
                    aproximar(i, m, e, ve, dt);
                }
                break;
            case EstadoInimigo::Preparando:
                if (i.tempoNoEstado < d.preparo) break;
                if (!d.aDistancia) {
                    if (dist <= ALCANCE_GOLPE && ve && nivel) {
                        e.golpes.push_back({mundo::sortear(m.rng, i.danoMin, i.danoMax), i.tipo});
                    }
                } else if (ve && dist > 1e-4f) {  // sem direção se estiver exatamente no jogador
                    // Sai da frente do atirador, na altura dos olhos, e voa reto para o meio do corpo do jogador
                    const glm::vec2 plano = glm::normalize(m.jogador - i.posicao);
                    const glm::vec2 boca = i.posicao + plano * (RAIO_INIMIGO + 0.05f);
                    const glm::vec3 origem{boca.x, i.corpo.pes + OLHOS_INIMIGO, boca.y};
                    const glm::vec3 alvo{m.jogador.x, m.pesJogador + CENTRO_JOGADOR, m.jogador.y};
                    e.tiros.push_back({origem, glm::normalize(alvo - origem) * VELOCIDADE_PROJETIL,
                                       mundo::sortear(m.rng, i.danoMin, i.danoMax), i.tipo});
                }
                mudar(i, EstadoInimigo::Recuperando);
                break;
            case EstadoInimigo::Recuperando:
                if (i.tempoNoEstado >= d.recuperacao) mudar(i, EstadoInimigo::Perseguindo);
                break;
            case EstadoInimigo::Destruido: break;
        }
        mundo::atualizarVertical(i.corpo, m.grade, i.posicao, RAIO_INIMIGO, dt);  // inimigos não levam dano de queda
    }
    separar(v, m.grade, m.jogador, m.pesJogador, m.raioJogador);
    return e;
}

}
