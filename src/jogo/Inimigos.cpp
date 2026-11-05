#include "jogo/Inimigos.h"

#include <algorithm>
#include <cmath>
#include <random>

#include <glm/geometric.hpp>

#include "mundo/Aleatorio.h"

namespace jogo {

namespace {

//                                nome                  vida dano   veloc def xp preparo recup  distância altura
constexpr DadosInimigo TABELA[] = {{"Rato mecânico",       6, 2, 3,   3.5f, 0, 3, 0.3f, 0.6f, false, 0.30f},
                                   {"Autômato operário",  16, 4, 6,   2.0f, 0, 8, 0.5f, 0.7f, false, 0.85f},
                                   {"Atirador da Guilda", 10, 3, 5,   2.0f, 0, 8, 0.4f, 1.1f, true,  0.85f},
                                   {"Colosso a vapor",    40, 8, 12,  1.2f, 2, 20, 0.9f, 1.0f, false, 1.00f}};

constexpr float DISTANCIA_ENTRE_INIMIGOS = 0.6f;
constexpr int TENTATIVAS_POSICAO = 200;

// Pesos de sorteio por andar (rato, autômato, atirador, colosso)
TipoInimigo sortearTipo(std::mt19937& rng, int andar) {
    int pesos[4] = {60, 40, 0, 0};
    if (andar == 2) pesos[0] = 40, pesos[1] = 35, pesos[2] = 25;
    if (andar >= 3) pesos[0] = 30, pesos[1] = 30, pesos[2] = 25, pesos[3] = 15;
    int r = mundo::sortear(rng, 0, pesos[0] + pesos[1] + pesos[2] + pesos[3] - 1);
    for (int i = 0; i < 4; ++i) {
        if (r < pesos[i]) return static_cast<TipoInimigo>(i);
        r -= pesos[i];
    }
    return TipoInimigo::Rato;
}

TipoInimigo tipoDaMarca(char c) {
    switch (c) {
        case 'a': return TipoInimigo::Automato;
        case 'g': return TipoInimigo::Atirador;
        case 'c': return TipoInimigo::Colosso;
        default: return TipoInimigo::Rato;
    }
}

}

const DadosInimigo& dados(TipoInimigo t) { return TABELA[static_cast<int>(t)]; }

int escalar(int valor, int andar) {
    return std::max(1, static_cast<int>(std::lround(valor * (1.0 + 0.15 * (andar - 1)))));
}

Inimigo criarInimigo(TipoInimigo t, glm::vec2 posicao, int andar) {
    const DadosInimigo& d = dados(t);
    const int vida = escalar(d.vida, andar);
    return Inimigo{t, posicao, vida, vida, escalar(d.danoMin, andar), escalar(d.danoMax, andar)};
}

std::vector<Inimigo> gerarInimigos(const mundo::Andar& andar, uint32_t semente) {
    std::vector<Inimigo> v;
    if (andar.salas.size() < 2) return v;
    std::mt19937 rng(mundo::sementeDoAndar(semente, andar.numero) ^ 0x51EDu);
    const int total = mundo::sortear(rng, 6 + 2 * andar.numero, 10 + 2 * andar.numero);

    // Célula de chão livre numa sala, longe dos outros inimigos
    auto posicaoLivre = [&](const mundo::Sala& s) -> std::optional<glm::vec2> {
        for (int t = 0; t < TENTATIVAS_POSICAO; ++t) {
            const int x = mundo::sortear(rng, s.x, s.x + s.largura - 1);
            const int y = mundo::sortear(rng, s.y, s.y + s.altura - 1);
            if (andar.grade.em(x, y) != mundo::Celula::Chao) continue;
            const glm::vec2 p{x + 0.5f, y + 0.5f};
            const bool perto = std::any_of(v.begin(), v.end(), [&](const Inimigo& o) {
                return glm::distance(o.posicao, p) < DISTANCIA_ENTRE_INIMIGOS;
            });
            if (!perto) return p;
        }
        return std::nullopt;
    };

    int sorteados = 0;
    while (sorteados < total) {
        const TipoInimigo tipo = sortearTipo(rng, andar.numero);
        // Ratos vêm em bando de 2–4, todos na mesma sala (sem passar do total)
        const int quantos = tipo == TipoInimigo::Rato ? std::min(mundo::sortear(rng, 2, 4), total - sorteados) : 1;
        const mundo::Sala& sala = andar.salas[mundo::sortear(rng, 1, static_cast<int>(andar.salas.size()) - 1)];
        for (int i = 0; i < quantos; ++i, ++sorteados)
            if (auto p = posicaoLivre(sala)) v.push_back(criarInimigo(tipo, *p, andar.numero));
    }
    for (Inimigo& i : v) i.corpo.pes = mundo::alturaDoChao(andar.grade, i.posicao);  // nascem no chão da célula
    return v;
}

std::vector<Inimigo> inimigosDasMarcas(const mundo::Andar& andar) {
    std::vector<Inimigo> v;
    for (const auto& [letra, posicao] : andar.marcas) {
        v.push_back(criarInimigo(tipoDaMarca(letra), posicao, andar.numero));
        v.back().corpo.pes = mundo::alturaDoChao(andar.grade, posicao);
    }
    return v;
}

}
