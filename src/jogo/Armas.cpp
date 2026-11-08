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
constexpr float MEIO_ARCO_GRAUS = 30.0f;  // armas brancas: arco de 60° à frente
constexpr float BARULHO_GOLPE = 4.0f, BARULHO_DISPARO = 10.0f;
constexpr float MIRA_VERTICAL_GRAUS = 4.0f;  // tolerância vertical (mira automática leve, menor que a de Doom)
constexpr float DISTANCIA_QUEDA_CHUMBO = 4.0f;  // bacamarte: dano pela metade depois disso

// Aplica o dano já descontada a defesa do tipo; devolve se destruiu
bool ferir(Inimigo& i, int dano) {
    // Chefe antes da luta (arena aberta): não leva dano nem acorda; a luta começa quando a arena tranca
    if (dados(i.tipo).chefe && i.estado == EstadoInimigo::Parado) return false;
    i.vida -= std::max(1, dano - dados(i.tipo).defesa);
    if (i.estado == EstadoInimigo::Parado) i.estado = EstadoInimigo::Perseguindo;
    if (i.vida > 0) return false;
    i.estado = EstadoInimigo::Destruido;
    i.tempoNoEstado = 0;
    return true;
}

void registrarAcerto(ResultadoAtaque& r, Inimigo& i, int dano) {
    ++r.acertos;
    if (ferir(i, dano)) {
        ++r.destruidos;
    }
}

int sortearDano(const DadosArma& d, const Item& arma, const Atributos& atr, std::mt19937& rng) {
    return mundo::sortear(rng, d.danoMin, d.danoMax) + bonusPorChumbo(d, atr.danoBase + somaBonus(arma, TipoBonus::Dano));
}

float comBonusPorcento(float base, const Item& arma, TipoBonus t) {
    return base * (1.0f - somaBonus(arma, t) / 100.0f);
}

void golpear(ResultadoAtaque& r, const Item& arma, const Atributos& atr, glm::vec2 p, float pes, float angulo,
             const mundo::Grade& g, std::vector<Inimigo>& v, std::mt19937& rng) {
    const DadosArma& d = dadosArma(arma.tipo);
    const glm::vec2 frente{std::cos(angulo), std::sin(angulo)};
    const float cosArco = std::cos(glm::radians(MEIO_ARCO_GRAUS));
    for (Inimigo& i : v) {
        if (i.estado == EstadoInimigo::Destruido) continue;
        if (std::abs(i.corpo.pes - pes) >= mundo::MESMO_NIVEL) continue;  // um em cima da plataforma, outro embaixo
        const glm::vec2 ate = i.posicao - p;
        const float dist = glm::length(ate);
        if (dist - raioDe(i) > d.alcance) continue;
        if (dist > 1e-4f && glm::dot(ate / dist, frente) < cosArco) continue;
        if (!mundo::temLinhaDeVisao(g, p, i.posicao)) continue;
        // O golpe empurra e interrompe o preparo do ataque (o do chefe, não: senão golpes seguidos o travam)
        i.empurrao = (dist > 1e-4f ? ate / dist : frente) * d.empurrao;
        if (i.estado == EstadoInimigo::Preparando && !dados(i.tipo).chefe) {
            i.estado = EstadoInimigo::Perseguindo;
            i.tempoNoEstado = 0;
        }
        registrarAcerto(r, i, sortearDano(d, arma, atr, rng));
    }
    r.barulho = BARULHO_GOLPE;
}

// A mira vertical acerta se passar entre o pé e o topo do inimigo, com uma tolerância de MIRA_VERTICAL_GRAUS
// (pé e topo medidos a partir dos olhos)
bool miraVerticalAcerta(float inclinacao, float distancia, float pe, float topo) {
    const float tolerancia = glm::radians(MIRA_VERTICAL_GRAUS);
    const float anguloTopo = std::atan2(topo, distancia);
    const float anguloPe = std::atan2(pe, distancia);
    return inclinacao >= anguloPe - tolerancia && inclinacao <= anguloTopo + tolerancia;
}

// Um traço instantâneo: o inimigo vivo mais próximo cruzado antes de uma parede (e dentro do alcance, se houver)
Inimigo* tracar(glm::vec2 p, float olhos, glm::vec2 d, float inclinacao, float alcance, const mundo::Grade& g,
                std::vector<Inimigo>& v, float& distancia) {
    Inimigo* alvo = nullptr;
    float melhorT = std::numeric_limits<float>::max();
    for (Inimigo& i : v) {
        if (i.estado == EstadoInimigo::Destruido) continue;
        // Interseção raio × círculo
        const glm::vec2 oc = p - i.posicao;
        const float b = glm::dot(oc, d);
        const float c = glm::dot(oc, oc) - raioDe(i) * raioDe(i);
        const float delta = b * b - c;
        if (delta < 0) continue;
        float t = -b - std::sqrt(delta);
        if (t < 0) t = -b + std::sqrt(delta);  // jogador dentro do círculo
        if (t < 0 || t >= melhorT) continue;
        if (alcance > 0 && t > alcance) continue;
        const float pe = i.corpo.pes - olhos, topo = i.corpo.pes + dados(i.tipo).altura - olhos;
        if (!miraVerticalAcerta(inclinacao, t, pe, topo)) continue;
        // O raio (preso à altura do inimigo, pela tolerância) não pode bater em parede, chão, degrau ou teto antes
        const float y = std::clamp(olhos + std::tan(inclinacao) * t, i.corpo.pes, i.corpo.pes + dados(i.tipo).altura);
        const glm::vec2 ponto = p + d * t;
        if (!mundo::temLinhaDeVisao3D(g, {p.x, olhos, p.y}, {ponto.x, y, ponto.y})) continue;
        melhorT = t;
        alvo = &i;
    }
    distancia = melhorT;
    return alvo;
}

void disparar(ResultadoAtaque& r, const Item& arma, const Atributos& atr, glm::vec2 p, float pes, float angulo, float inclinacao,
              const mundo::Grade& g, std::vector<Inimigo>& v, std::mt19937& rng) {
    const DadosArma& d = dadosArma(arma.tipo);
    for (int k = 0; k < d.chumbos; ++k) {
        const float desvio = glm::radians(mundo::sortearReal(rng, -d.abertura / 2, d.abertura / 2));
        const glm::vec2 direcao{std::cos(angulo + desvio), std::sin(angulo + desvio)};
        float distancia = 0;
        Inimigo* alvo = tracar(p, pes + ALTURA_OLHOS, direcao, inclinacao, d.alcance, g, v, distancia);
        if (!alvo) continue;
        int dano = sortearDano(d, arma, atr, rng);
        if (d.chumbos > 1 && distancia > DISTANCIA_QUEDA_CHUMBO) dano = std::max(1, dano / 2);
        registrarAcerto(r, *alvo, dano);
    }
    r.disparou = true;
    r.barulho = BARULHO_DISPARO;
}

}

int& Municao::de(TipoMunicao t) {
    switch (t) {
        case TipoMunicao::Cartuchos: return cartuchos;
        case TipoMunicao::Celulas: return celulas;
        default: return balas;
    }
}

int Municao::de(TipoMunicao t) const { return const_cast<Municao*>(this)->de(t); }

int maximoMunicao(TipoMunicao t) {
    switch (t) {
        case TipoMunicao::Cartuchos: return 20;
        case TipoMunicao::Celulas: return 12;
        default: return 48;
    }
}

int bonusPorChumbo(const DadosArma& d, int extra) { return d.chumbos > 1 ? (extra + 1) / 2 : extra; }

ResultadoAtaque atualizarArmas(Armas& a, Inventario& inv, const Atributos& atr, const Comandos& c, glm::vec2 posicao,
                               float pes, float angulo, float inclinacao, const mundo::Grade& g, std::vector<Inimigo>& inimigos,
                               std::mt19937& rng, float dt) {
    ResultadoAtaque r;
    Item& fogo = inv.armaDeFogo();
    const DadosArma& dFogo = dadosArma(fogo.tipo);
    int& reserva = a.municao.de(dFogo.municao);

    a.intervalo = std::max(0.0f, a.intervalo - dt);
    a.animacao = std::max(0.0f, a.animacao - dt);
    if (a.recarga > 0) {
        a.recarga -= dt;
        if (a.recarga <= FOLGA) {
            const int quantas = std::min(capacidade(fogo) - fogo.carga, reserva);
            fogo.carga += std::max(0, quantas);
            reserva -= std::max(0, quantas);
            a.recarga = 0;
        }
    }

    if (c.trocarArma) {
        a.ativa = a.ativa == ArmaAtiva::Branca ? ArmaAtiva::Fogo : ArmaAtiva::Branca;
        a.recarga = 0;  // trocar cancela a recarga
    }
    // Recarregar: só arma de fogo com carga (a Tesla gasta direto da reserva)
    if (c.recarregar && a.ativa == ArmaAtiva::Fogo && dFogo.carga > 0 && a.recarga == 0 &&
        fogo.carga < capacidade(fogo) && reserva > 0) {
        a.recarga = comBonusPorcento(dFogo.recarga, fogo, TipoBonus::Recarga);
        r.comecouRecarga = true;
    }

    if (!c.atacar || a.intervalo > FOLGA || a.recarga > 0) return r;

    if (a.ativa == ArmaAtiva::Branca) {
        const Item& branca = inv.armaBranca();
        golpear(r, branca, atr, posicao, pes, angulo, g, inimigos, rng);
        r.golpeou = true;
        a.intervalo = comBonusPorcento(dadosArma(branca.tipo).intervalo, branca, TipoBonus::Velocidade);
        a.animacao = DURACAO_ANIMACAO_ARMA;
        return r;
    }

    const float intervalo = comBonusPorcento(dFogo.intervalo, fogo, TipoBonus::Velocidade);
    const bool semRecarga = dFogo.carga == 0;
    if ((semRecarga && reserva == 0) || (!semRecarga && fogo.carga == 0)) {
        if (semRecarga) r.mensagem = "Sem células elétricas";
        else r.mensagem = reserva > 0 ? "Sem munição — {recarregar} para recarregar" : "Sem munição";
        r.vazia = true;
        a.intervalo = intervalo;  // não repete o aviso a cada passo
        return r;
    }
    if (semRecarga) --reserva;
    else --fogo.carga;
    disparar(r, fogo, atr, posicao, pes, angulo, inclinacao, g, inimigos, rng);
    a.intervalo = intervalo;
    a.animacao = DURACAO_ANIMACAO_ARMA;
    return r;
}

}
