#include "jogo/Armas.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "jogo/IA.h"
#include "jogo/Jogador.h"
#include "mundo/Aleatorio.h"
#include "mundo/Consultas.h"

namespace jogo {

namespace {

constexpr float FOLGA = 1e-4f;  // tolerância de arredondamento nos temporizadores

// Chave inglesa
constexpr int CHAVE_DANO_MIN = 4, CHAVE_DANO_MAX = 6;
constexpr float CHAVE_INTERVALO = 0.35f, CHAVE_ALCANCE = 1.1f, CHAVE_EMPURRAO = 0.3f, CHAVE_BARULHO = 4.0f;
constexpr float MEIO_ARCO_GRAUS = 30.0f;  // arco de 60° à frente

// Revólver
constexpr int REVOLVER_DANO_MIN = 7, REVOLVER_DANO_MAX = 10;
constexpr float REVOLVER_INTERVALO = 0.4f, REVOLVER_RECARGA = 1.5f, REVOLVER_BARULHO = 10.0f;
constexpr float REVOLVER_DISPERSAO_GRAUS = 2.0f;
constexpr float MIRA_VERTICAL_GRAUS = 4.0f;  // tolerância vertical (mira automática leve, menor que a de Doom)

// Aplica o dano já descontada a defesa do tipo; devolve se destruiu
bool ferir(Inimigo& i, int dano) {
    i.vida -= std::max(1, dano - dados(i.tipo).defesa);
    if (i.estado == EstadoInimigo::Parado) i.estado = EstadoInimigo::Perseguindo;
    if (i.vida > 0) return false;
    i.estado = EstadoInimigo::Destruido;
    i.tempoNoEstado = 0;
    return true;
}

void golpear(ResultadoAtaque& r, glm::vec2 p, float angulo, const mundo::Grade& g, std::vector<Inimigo>& v,
             std::mt19937& rng) {
    const glm::vec2 frente{std::cos(angulo), std::sin(angulo)};
    const float cosArco = std::cos(glm::radians(MEIO_ARCO_GRAUS));
    for (Inimigo& i : v) {
        if (i.estado == EstadoInimigo::Destruido) continue;
        const glm::vec2 ate = i.posicao - p;
        const float dist = glm::length(ate);
        if (dist - RAIO_INIMIGO > CHAVE_ALCANCE) continue;
        if (dist > 1e-4f && glm::dot(ate / dist, frente) < cosArco) continue;
        if (!mundo::temLinhaDeVisao(g, p, i.posicao)) continue;
        ++r.acertos;
        // O golpe empurra e interrompe o preparo do ataque
        i.empurrao = (dist > 1e-4f ? ate / dist : frente) * CHAVE_EMPURRAO;
        if (i.estado == EstadoInimigo::Preparando) {
            i.estado = EstadoInimigo::Perseguindo;
            i.tempoNoEstado = 0;
        }
        if (ferir(i, mundo::sortear(rng, CHAVE_DANO_MIN, CHAVE_DANO_MAX))) ++r.destruidos;
    }
    r.barulho = CHAVE_BARULHO;
}

// Tiro instantâneo: acerta o inimigo mais próximo cruzado pelo raio antes de uma parede
// A mira vertical acerta se passar entre o pé e o topo do inimigo, com uma tolerância de MIRA_VERTICAL_GRAUS
bool miraVerticalAcerta(float inclinacao, float distancia, float altura) {
    const float tolerancia = glm::radians(MIRA_VERTICAL_GRAUS);
    const float topo = std::atan2(altura - ALTURA_OLHOS, distancia);
    const float pe = std::atan2(-ALTURA_OLHOS, distancia);
    return inclinacao >= pe - tolerancia && inclinacao <= topo + tolerancia;
}

void disparar(ResultadoAtaque& r, glm::vec2 p, float angulo, float inclinacao, const mundo::Grade& g,
              std::vector<Inimigo>& v, std::mt19937& rng) {
    const float desvio = glm::radians(mundo::sortearReal(rng, -REVOLVER_DISPERSAO_GRAUS, REVOLVER_DISPERSAO_GRAUS));
    const glm::vec2 d{std::cos(angulo + desvio), std::sin(angulo + desvio)};
    Inimigo* alvo = nullptr;
    float melhorT = std::numeric_limits<float>::max();
    for (Inimigo& i : v) {
        if (i.estado == EstadoInimigo::Destruido) continue;
        // Interseção raio × círculo
        const glm::vec2 oc = p - i.posicao;
        const float b = glm::dot(oc, d);
        const float c = glm::dot(oc, oc) - RAIO_INIMIGO * RAIO_INIMIGO;
        const float delta = b * b - c;
        if (delta < 0) continue;
        float t = -b - std::sqrt(delta);
        if (t < 0) t = -b + std::sqrt(delta);  // jogador dentro do círculo
        if (t < 0 || t >= melhorT) continue;
        if (!miraVerticalAcerta(inclinacao, t, dados(i.tipo).altura)) continue;
        if (!mundo::temLinhaDeVisao(g, p, p + d * t)) continue;
        melhorT = t;
        alvo = &i;
    }
    if (alvo) {
        ++r.acertos;
        if (ferir(*alvo, mundo::sortear(rng, REVOLVER_DANO_MIN, REVOLVER_DANO_MAX))) ++r.destruidos;
    }
    r.disparou = true;
    r.barulho = REVOLVER_BARULHO;
}

}

ResultadoAtaque atualizarArmas(Armas& a, const Comandos& c, glm::vec2 posicao, float angulo, const mundo::Grade& g,
                               std::vector<Inimigo>& inimigos, std::mt19937& rng, float dt, float inclinacao) {
    ResultadoAtaque r;
    a.intervalo = std::max(0.0f, a.intervalo - dt);
    a.animacao = std::max(0.0f, a.animacao - dt);
    if (a.recarga > 0) {
        a.recarga -= dt;
        if (a.recarga <= FOLGA) {
            const int quantas = std::min(CARGA_REVOLVER - a.carga, a.reserva);
            a.carga += quantas;
            a.reserva -= quantas;
            a.recarga = 0;
        }
    }

    if (c.trocarArma) {
        a.ativa = a.ativa == ArmaAtiva::ChaveInglesa ? ArmaAtiva::Revolver : ArmaAtiva::ChaveInglesa;
        a.recarga = 0;  // trocar cancela a recarga
    }
    if (c.recarregar && a.ativa == ArmaAtiva::Revolver && a.recarga == 0 && a.carga < CARGA_REVOLVER && a.reserva > 0)
        a.recarga = REVOLVER_RECARGA;

    if (!c.atacar || a.intervalo > FOLGA || a.recarga > 0) return r;

    if (a.ativa == ArmaAtiva::ChaveInglesa) {
        golpear(r, posicao, angulo, g, inimigos, rng);
        a.intervalo = CHAVE_INTERVALO;
        a.animacao = DURACAO_ANIMACAO_ARMA;
    } else if (a.carga == 0) {
        r.mensagem = a.reserva > 0 ? "Sem munição — R para recarregar" : "Sem munição";
        a.intervalo = REVOLVER_INTERVALO;  // não repete o aviso a cada passo
    } else {
        --a.carga;
        disparar(r, posicao, angulo, inclinacao, g, inimigos, rng);
        a.intervalo = REVOLVER_INTERVALO;
        a.animacao = DURACAO_ANIMACAO_ARMA;
    }
    return r;
}

}
