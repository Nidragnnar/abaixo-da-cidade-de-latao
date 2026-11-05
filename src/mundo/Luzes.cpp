#include "mundo/Luzes.h"

#include <algorithm>
#include <cstdlib>
#include <numbers>
#include <random>

#include <glm/geometric.hpp>

#include "mundo/Aleatorio.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

namespace mundo {

namespace {

constexpr int AREA_SALA_GRANDE = 30;
constexpr float COBERTURA_CORREDOR = 4.5f;  // toda célula de corredor fica a no máximo isto de uma luz
constexpr float EMPURRAO_PAREDE = 0.45f;     // do centro da célula até quase encostar na parede
constexpr float DISTANCIA_NA_SALA = 3.0f;    // luzes da mesma sala não ficam coladas

glm::vec3 corDaZona(Zona z) {
    switch (z) {
        case Zona::Fabrica: return {1.0f, 0.6f, 0.3f};   // fornalha
        case Zona::Minas: return {1.0f, 0.92f, 0.75f};   // lampião
        default: return {1.0f, 0.85f, 0.5f};             // esgoto
    }
}

// Lugar de lampião: célula aberta e o lado em que há parede
struct Encaixe {
    glm::ivec2 celula;
    glm::ivec2 lado;
};

std::vector<Encaixe> encaixes(const Grade& g, glm::ivec2 c) {
    std::vector<Encaixe> e;
    if (g.em(c.x, c.y) != Celula::Chao) return e;  // em comporta a placa atravessaria o lampião
    for (const glm::ivec2 d : {glm::ivec2(-1, 0), glm::ivec2(1, 0), glm::ivec2(0, -1), glm::ivec2(0, 1)})
        if (g.em(c.x + d.x, c.y + d.y) == Celula::Parede) e.push_back({c, d});
    return e;
}

}

std::vector<Luz> gerarLuzes(const Andar& a, uint32_t semente) {
    std::mt19937 rng(semente ^ 0x11C3u);
    const Grade& g = a.grade;
    const glm::vec3 cor = corDaZona(a.zona);
    std::vector<Luz> luzes;
    auto acender = [&](const Encaixe& e) {
        const glm::vec2 centro = glm::vec2(e.celula) + 0.5f + glm::vec2(e.lado) * EMPURRAO_PAREDE;
        luzes.push_back({{centro.x, alturaDoChao(g, centro) + ALTURA_LUZ, centro.y}, cor, ALCANCE_LUZ,
                         sortearReal(rng, 0.0f, 2.0f * std::numbers::pi_v<float>)});
    };
    auto distanciaAte = [&](glm::vec2 p) {
        float d = 1e9f;
        for (const Luz& l : luzes) d = std::min(d, glm::distance(p, glm::vec2(l.posicao.x, l.posicao.z)));
        return d;
    };

    // Salas: lustres no teto, sobre o centro (dois ao longo do lado comprido nas salas grandes)
    for (const Sala& s : a.salas) {
        const glm::vec2 centro{s.x + s.largura / 2.0f, s.y + s.altura / 2.0f};
        std::vector<glm::vec2> pontos{centro};
        if (s.largura * s.altura >= AREA_SALA_GRANDE) {
            const glm::vec2 terco = s.largura >= s.altura ? glm::vec2(s.largura / 6.0f, 0.0f) : glm::vec2(0.0f, s.altura / 6.0f);
            pontos = {centro - terco, centro + terco};
        }
        for (const glm::vec2 p : pontos)
            luzes.push_back({{p.x, alturaDoTeto(g, p) - DESCIDA_LUSTRE, p.y}, cor, ALCANCE_LUSTRE,
                             sortearReal(rng, 0.0f, 2.0f * std::numbers::pi_v<float>), TipoLuz::Lustre});
    }

    // Salas: lampiões nas paredes de dentro, afastados uns dos outros
    for (const Sala& s : a.salas) {
        std::vector<Encaixe> candidatos;
        for (int y = s.y; y < s.y + s.altura; ++y)
            for (int x = s.x; x < s.x + s.largura; ++x)
                for (const Encaixe& e : encaixes(g, {x, y})) candidatos.push_back(e);
        if (candidatos.empty()) continue;
        const int quantas = s.largura * s.altura < AREA_SALA_GRANDE ? 1 : sortear(rng, 2, 3);
        std::shuffle(candidatos.begin(), candidatos.end(), rng);
        const size_t antes = luzes.size();
        for (const Encaixe& e : candidatos) {
            if (static_cast<int>(luzes.size() - antes) == quantas) break;
            bool longe = true;
            for (size_t i = antes; i < luzes.size(); ++i)
                longe &= glm::distance(glm::vec2(e.celula), glm::vec2(luzes[i].posicao.x, luzes[i].posicao.z)) >=
                         DISTANCIA_NA_SALA;
            if (longe) acender(e);
        }
    }

    // Corredores (e o que mais estiver fora das salas): acende onde ainda está escuro
    for (int y = 0; y < g.altura(); ++y)
        for (int x = 0; x < g.largura(); ++x) {
            if (g.em(x, y) == Celula::Parede) continue;
            if (std::any_of(a.salas.begin(), a.salas.end(), [&](const Sala& s) { return s.contem(x, y); })) continue;
            if (distanciaAte(glm::vec2(x + 0.5f, y + 0.5f)) <= COBERTURA_CORREDOR) continue;
            // Aqui mesmo ou, se não dá (comporta, cruzamento sem parede), na célula com parede mais perto, até 2 de distância
            for (int raio = 0; raio <= 2; ++raio) {
                std::vector<Encaixe> e;
                for (int dy = -raio; dy <= raio; ++dy)
                    for (int dx = -raio; dx <= raio; ++dx)
                        if (std::max(std::abs(dx), std::abs(dy)) == raio)
                            for (const Encaixe& en : encaixes(g, {x + dx, y + dy})) e.push_back(en);
                if (e.empty()) continue;
                acender(e[static_cast<size_t>(sortear(rng, 0, static_cast<int>(e.size()) - 1))]);
                break;
            }
        }
    return luzes;
}

}
