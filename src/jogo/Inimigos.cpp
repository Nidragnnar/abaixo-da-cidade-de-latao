#include "jogo/Inimigos.h"

#include <algorithm>
#include <cmath>
#include <random>

#include <glm/geometric.hpp>

#include "mundo/Aleatorio.h"

namespace jogo {

namespace {

//                                nome                  vida dano   veloc def xp preparo recup  distância altura voa  chefe  raio
constexpr DadosInimigo TABELA[] = {{"Rato mecânico",       6, 2, 3,   3.5f, 0, 3, 0.3f, 0.6f, false, 0.30f, false, false, 0.25f},
                                   {"Autômato operário",  16, 4, 6,   2.0f, 0, 8, 0.5f, 0.7f, false, 0.85f, false, false, 0.25f},
                                   {"Atirador da Guilda", 10, 3, 5,   2.0f, 0, 8, 0.4f, 1.1f, true,  0.85f, false, false, 0.25f},
                                   {"Colosso a vapor",    40, 8, 12,  1.2f, 2, 20, 0.9f, 1.0f, false, 1.00f, false, false, 0.25f},
                                   {"Vespa a corda",       6, 2, 4,   3.0f, 0, 5, 0.3f, 0.8f, false, 0.30f, true,  false, 0.25f},
                                   // Dirigível: sem preparo; a recuperação é o intervalo entre bombas
                                   {"Dirigível a vapor",  18, 5, 8,   1.0f, 1, 12, 0.0f, 2.5f, true,  0.60f, true,  false, 0.25f},
                                   // Chefes (Etapa 7): dano do ataque principal; os outros ataques ficam em Chefes.h
                                   {"Rainha dos Ratos",  140, 7, 10,  2.5f, 0, 60, 0.6f, 1.5f, false, 0.80f, false, true, 0.50f},
                                   {"Capataz a Vapor",   240, 4, 6,   1.0f, 3, 120, 0.5f, 0.5f, true, 1.40f, false, true, 0.60f},
                                   {"Grande Máquina",    420, 4, 6,   0.0f, 2, 250, 0.0f, 0.0f, true, 1.80f, false, true, 0.80f}};

constexpr float DISTANCIA_ENTRE_INIMIGOS = 0.6f;
constexpr int TENTATIVAS_POSICAO = 200;

// Pesos de sorteio por andar (rato, autômato, atirador, colosso, vespa, dirigível)
TipoInimigo sortearTipo(std::mt19937& rng, int andar) {
    int pesos[TOTAL_TIPOS_INIMIGO] = {60, 40, 0, 0, 0, 0, 0, 0, 0};  // chefes nunca no sorteio
    if (andar == 2) pesos[0] = 35, pesos[1] = 30, pesos[2] = 20, pesos[4] = 15;
    if (andar >= 3) pesos[0] = 25, pesos[1] = 25, pesos[2] = 20, pesos[3] = 12, pesos[4] = 10, pesos[5] = 8;
    int soma = 0;
    for (int p : pesos) soma += p;
    int r = mundo::sortear(rng, 0, soma - 1);
    for (int i = 0; i < TOTAL_TIPOS_INIMIGO; ++i) {
        if (r < pesos[i]) return static_cast<TipoInimigo>(i);
        r -= pesos[i];
    }
    return TipoInimigo::Rato;
}

}

TipoInimigo tipoDaMarca(char c) {
    switch (c) {
        case 'a': return TipoInimigo::Automato;
        case 'g': return TipoInimigo::Atirador;
        case 'c': return TipoInimigo::Colosso;
        case 'v': return TipoInimigo::Vespa;
        case 'd': return TipoInimigo::Dirigivel;
        case 'R': return TipoInimigo::RainhaRatos;
        case 'P': return TipoInimigo::Capataz;
        case 'M': return TipoInimigo::GrandeMaquina;
        default: return TipoInimigo::Rato;
    }
}

const DadosInimigo& dados(TipoInimigo t) { return TABELA[static_cast<int>(t)]; }

float raioDe(const Inimigo& i) { return dados(i.tipo).raio; }

TipoInimigo chefeDoAndar(int andar) {
    return andar <= 2 ? TipoInimigo::RainhaRatos : andar <= 4 ? TipoInimigo::Capataz : TipoInimigo::GrandeMaquina;
}

int escalar(int valor, int andar) {
    return std::max(1, static_cast<int>(std::lround(valor * (1.0 + 0.15 * (andar - 1)))));
}

Inimigo criarInimigo(TipoInimigo t, glm::vec2 posicao, int andar) {
    const DadosInimigo& d = dados(t);
    if (d.chefe) return Inimigo{t, posicao, d.vida, d.vida, d.danoMin, d.danoMax};  // valores finais, sem escala
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
        // Ratos vêm em bando de 2–4 e vespas em 1–2, todos na mesma sala (sem passar do total)
        const int quantos = tipo == TipoInimigo::Rato    ? std::min(mundo::sortear(rng, 2, 4), total - sorteados)
                            : tipo == TipoInimigo::Vespa ? std::min(mundo::sortear(rng, 1, 2), total - sorteados)
                                                         : 1;
        // Nunca na arena do chefe (sorteia outra sala; com só a inicial e a arena, ninguém)
        int indiceSala = mundo::sortear(rng, 1, static_cast<int>(andar.salas.size()) - 1);
        for (int t = 0; andar.arena && indiceSala == static_cast<int>(*andar.arena) && t < TENTATIVAS_POSICAO; ++t)
            indiceSala = mundo::sortear(rng, 1, static_cast<int>(andar.salas.size()) - 1);
        if (andar.arena && indiceSala == static_cast<int>(*andar.arena)) break;
        const mundo::Sala& sala = andar.salas[indiceSala];
        for (int i = 0; i < quantos; ++i, ++sorteados)
            if (auto p = posicaoLivre(sala)) v.push_back(criarInimigo(tipo, *p, andar.numero));
    }
    // O chefe espera no centro da arena
    if (andar.arena) v.push_back(criarInimigo(chefeDoAndar(andar.numero), glm::vec2(andar.salas[*andar.arena].centro()) + 0.5f, andar.numero));
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
