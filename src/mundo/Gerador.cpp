#include "mundo/Gerador.h"

#include <algorithm>
#include <cstdlib>
#include <random>
#include <set>
#include <utility>

#include "mundo/Consultas.h"

namespace mundo {

namespace {

constexpr int MIN_SALAS = 8, MAX_SALAS = 15;
constexpr int MIN_LADO = 4, MAX_LADO = 10;
constexpr int TENTATIVAS_SALA = 2000;
constexpr int MIN_EXTRAS = 2, MAX_EXTRAS = 4;
constexpr float CHANCE_COMPORTA = 0.5f;
constexpr float CHANCE_TRANCA = 0.4f;
constexpr int VARIACOES_TRANCA = 10;

// Mistura de bits (splitmix32) para espalhar sementes parecidas
uint32_t misturar(uint32_t s) {
    uint32_t z = s + 0x9E3779B9u;
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}

// Sorteio próprio (a distribuição da biblioteca padrão pode mudar entre compiladores)
int sortear(std::mt19937& rng, int min, int max) { return min + static_cast<int>(rng() % (max - min + 1)); }
float sortear01(std::mt19937& rng) { return static_cast<float>(rng() % 1000000) / 1000000.0f; }

int distanciaManhattan(glm::ivec2 a, glm::ivec2 b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }

// Salas com o anel de parede sem encostar no anel de outra sala
std::vector<Sala> sortearSalas(std::mt19937& rng) {
    const int alvo = sortear(rng, MIN_SALAS, MAX_SALAS);
    std::vector<Sala> salas;
    for (int t = 0; t < TENTATIVAS_SALA && static_cast<int>(salas.size()) < alvo; ++t) {
        const int l = sortear(rng, MIN_LADO, MAX_LADO), a = sortear(rng, MIN_LADO, MAX_LADO);
        // O anel (x-1 .. x+l) fica dentro de [1, 62]: a borda da grade é sempre parede
        const Sala s{sortear(rng, 2, LADO_ANDAR - 2 - l), sortear(rng, 2, LADO_ANDAR - 2 - a), l, a};
        const bool encosta = std::any_of(salas.begin(), salas.end(), [&](const Sala& o) {
            return s.x - 1 <= o.x + o.largura && o.x - 1 <= s.x + s.largura &&
                   s.y - 1 <= o.y + o.altura && o.y - 1 <= s.y + s.altura;
        });
        if (!encosta) salas.push_back(s);
    }
    return salas;
}

void cavar(Grade& g, Sala s) {
    for (int y = s.y; y < s.y + s.altura; ++y)
        for (int x = s.x; x < s.x + s.largura; ++x) g.definir(x, y, Celula::Chao);
}

// Corredor em L de 1 célula entre dois pontos
void cavarCorredor(Grade& g, glm::ivec2 a, glm::ivec2 b, bool primeiroX) {
    auto linhaX = [&](int y, int de, int ate) {
        for (int x = std::min(de, ate); x <= std::max(de, ate); ++x)
            if (g.em(x, y) == Celula::Parede) g.definir(x, y, Celula::Chao);
    };
    auto linhaY = [&](int x, int de, int ate) {
        for (int y = std::min(de, ate); y <= std::max(de, ate); ++y)
            if (g.em(x, y) == Celula::Parede) g.definir(x, y, Celula::Chao);
    };
    if (primeiroX) {
        linhaX(a.y, a.x, b.x);
        linhaY(b.x, a.y, b.y);
    } else {
        linhaY(a.x, a.y, b.y);
        linhaX(b.y, a.x, b.x);
    }
}

// Células do anel da sala onde um corredor passa entre duas paredes (lugar de comporta)
std::vector<glm::ivec2> entradas(const Grade& g, const Sala& s) {
    std::vector<glm::ivec2> e;
    auto aberta = [&](int x, int y) { return g.em(x, y) != Celula::Parede; };
    auto parede = [&](int x, int y) { return g.em(x, y) == Celula::Parede; };
    for (int x = s.x; x < s.x + s.largura; ++x)
        for (int y : {s.y - 1, s.y + s.altura})
            if (aberta(x, y) && parede(x - 1, y) && parede(x + 1, y) && aberta(x, y - 1) && aberta(x, y + 1))
                e.push_back({x, y});
    for (int y = s.y; y < s.y + s.altura; ++y)
        for (int x : {s.x - 1, s.x + s.largura})
            if (aberta(x, y) && parede(x, y - 1) && parede(x, y + 1) && aberta(x - 1, y) && aberta(x + 1, y))
                e.push_back({x, y});
    return e;
}

int indice(glm::ivec2 c) { return c.y * LADO_ANDAR + c.x; }

// Uma tentativa completa; sem tranca quando querTranca é falso ou quando ela não deu certo (devolve nullopt)
std::optional<Andar> tentar(uint32_t semente, int numero, bool querTranca) {
    std::mt19937 rng(semente);
    Andar andar{numero, zonaDoAndar(numero), Grade(LADO_ANDAR, LADO_ANDAR), sortearSalas(rng), {}, {}, {}};
    Grade& g = andar.grade;
    const auto& salas = andar.salas;
    for (const Sala& s : salas) cavar(g, s);

    // Árvore (Prim pelos centros) + corredores extras para ciclos
    std::set<std::pair<int, int>> ligadas;
    std::vector<bool> naArvore(salas.size(), false);
    naArvore[0] = true;
    for (size_t n = 1; n < salas.size(); ++n) {
        int melhorA = -1, melhorB = -1, melhorD = 1 << 30;
        for (size_t a = 0; a < salas.size(); ++a)
            if (naArvore[a])
                for (size_t b = 0; b < salas.size(); ++b)
                    if (!naArvore[b]) {
                        const int d = distanciaManhattan(salas[a].centro(), salas[b].centro());
                        if (d < melhorD) melhorD = d, melhorA = static_cast<int>(a), melhorB = static_cast<int>(b);
                    }
        naArvore[melhorB] = true;
        ligadas.insert({std::min(melhorA, melhorB), std::max(melhorA, melhorB)});
        cavarCorredor(g, salas[melhorA].centro(), salas[melhorB].centro(), rng() % 2 == 0);
    }
    const int extras = sortear(rng, MIN_EXTRAS, MAX_EXTRAS);
    for (int i = 0; i < extras; ++i) {
        const int a = sortear(rng, 0, static_cast<int>(salas.size()) - 1);
        int melhor = -1, melhorD = 1 << 30;
        for (int b = 0; b < static_cast<int>(salas.size()); ++b) {
            if (b == a || ligadas.count({std::min(a, b), std::max(a, b)})) continue;
            const int d = distanciaManhattan(salas[a].centro(), salas[b].centro());
            if (d < melhorD) melhorD = d, melhor = b;
        }
        if (melhor < 0) continue;
        ligadas.insert({std::min(a, melhor), std::max(a, melhor)});
        cavarCorredor(g, salas[a].centro(), salas[melhor].centro(), rng() % 2 == 0);
    }

    // Comportas em cerca de metade das entradas
    for (const Sala& s : salas)
        for (glm::ivec2 e : entradas(g, s))
            if (sortear01(rng) < CHANCE_COMPORTA) g.definir(e.x, e.y, Celula::Comporta);

    // Início na sala 0; elevador no centro da sala mais distante pelo caminho
    andar.inicio = glm::vec2(salas[0].centro()) + 0.5f;
    const auto dist = distanciasNaGrade(g, salas[0].centro(), true);
    size_t salaElevador = 0;
    for (size_t i = 1; i < salas.size(); ++i)
        if (dist[indice(salas[i].centro())] > dist[indice(salas[salaElevador].centro())]) salaElevador = i;
    andar.elevador = salas[salaElevador].centro();
    g.definir(andar.elevador.x, andar.elevador.y, Celula::Elevador);

    if (!querTranca) return andar;

    // Tranca na entrada única da sala do elevador; a chave numa sala alcançável sem ela
    const auto entradasElevador = entradas(g, salas[salaElevador]);
    if (entradasElevador.size() != 1) return std::nullopt;
    const glm::ivec2 tranca = entradasElevador[0];
    g.definir(tranca.x, tranca.y, Celula::ComportaTrancada);
    const auto semTranca = distanciasNaGrade(g, salas[0].centro(), false);
    if (semTranca[indice(andar.elevador)] >= 0) return std::nullopt;

    std::vector<size_t> candidatas;
    for (size_t i = 1; i < salas.size(); ++i)
        if (i != salaElevador && semTranca[indice(salas[i].centro())] >= 0) candidatas.push_back(i);
    const size_t salaChave =
        candidatas.empty() ? 0 : candidatas[sortear(rng, 0, static_cast<int>(candidatas.size()) - 1)];
    andar.chave = glm::vec2(salas[salaChave].centro()) + 0.5f;
    return andar;
}

}

uint32_t sementeDoAndar(uint32_t sementePartida, int numero) {
    return misturar(sementePartida ^ (static_cast<uint32_t>(numero) * 0x9E3779B9u));
}

Andar gerarAndar(uint32_t sementePartida, int numero) {
    const uint32_t base = sementeDoAndar(sementePartida, numero);
    std::mt19937 sorteioTranca(base ^ 0xA5A5A5A5u);
    const bool querTranca = numero >= 2 && sortear01(sorteioTranca) < CHANCE_TRANCA;
    if (querTranca)
        for (int v = 0; v < VARIACOES_TRANCA; ++v)
            if (auto a = tentar(base + static_cast<uint32_t>(v) * 7919u, numero, true)) return std::move(*a);
    // Sem tranca (ou nenhuma variação permitiu tranca)
    return std::move(*tentar(base, numero, false));
}

Andar andarDeTexto(const std::vector<std::string>& linhas, int numero) {
    MapaCarregado m = carregarDeTexto(linhas);
    return Andar{numero, zonaDoAndar(numero), std::move(m.grade), {}, m.inicio,
                 m.elevador.value_or(glm::ivec2(-1, -1)), m.chave};
}

}
