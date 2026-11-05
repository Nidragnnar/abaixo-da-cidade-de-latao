#include "mundo/Relevo.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <random>
#include <vector>

#include <glm/common.hpp>

#include "mundo/Aleatorio.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

namespace mundo {

namespace {

constexpr int BLOCO = 4;            // quartos
constexpr int TETO_CORREDOR = 6;    // quartos acima do chão
constexpr int TETO_MAXIMO = 20, CHAO_MAXIMO = 12;  // em quartos (5 e 3 blocos)
constexpr int TETO_ARENA = 16;     // acima do chão da arena (limitado ao TETO_MAXIMO)
constexpr int ESPACO_MINIMO = 5;    // teto − chão (1,25 bloco)
constexpr int SEM_SALA = -1;
constexpr float CHANCE_SEGREDO = 0.35f;  // plataforma sem rampa: só pulando, com item em cima
constexpr float CHANCE_PILHA = 0.2f;

int indice(int x, int y) { return y * LADO_ANDAR + x; }
bool aberta(const Grade& g, int x, int y) { return g.em(x, y) != Celula::Parede; }
constexpr glm::ivec2 VIZINHOS[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

// Distância em células (sem olhar alturas) até o conjunto de fontes, só por células abertas
std::vector<int> distanciasPlanas(const Grade& g, const std::vector<glm::ivec2>& fontes) {
    std::vector<int> d(LADO_ANDAR * LADO_ANDAR, -1);
    std::deque<glm::ivec2> fila;
    for (const glm::ivec2 f : fontes) {
        d[indice(f.x, f.y)] = 0;
        fila.push_back(f);
    }
    while (!fila.empty()) {
        const glm::ivec2 c = fila.front();
        fila.pop_front();
        for (const glm::ivec2 v : VIZINHOS) {
            const glm::ivec2 n = c + v;
            if (!aberta(g, n.x, n.y) || d[indice(n.x, n.y)] >= 0) continue;
            d[indice(n.x, n.y)] = d[indice(c.x, c.y)] + 1;
            fila.push_back(n);
        }
    }
    return d;
}

// Platô de cada sala: o interior, as aberturas do anel e a célula logo depois de cada abertura (para a comporta
// ficar no plano); quem chega primeiro fica com a célula (a sala `primeira` antes das outras)
std::vector<int> donosDoPlato(const Grade& g, const std::vector<Sala>& salas, size_t primeira) {
    std::vector<int> dono(LADO_ANDAR * LADO_ANDAR, SEM_SALA);
    for (size_t r = 0; r < salas.size(); ++r)
        for (int y = salas[r].y; y < salas[r].y + salas[r].altura; ++y)
            for (int x = salas[r].x; x < salas[r].x + salas[r].largura; ++x) dono[indice(x, y)] = static_cast<int>(r);
    auto tomar = [&](int x, int y, int r) {
        if (aberta(g, x, y) && dono[indice(x, y)] == SEM_SALA) dono[indice(x, y)] = r;
    };
    // A sala do elevador escolhe primeiro: a comporta trancada da entrada dela não pode ficar torta
    for (size_t k = 0; k < salas.size(); ++k) {
        const size_t r = k == 0 ? primeira : (k == primeira ? 0 : k);
        const Sala& s = salas[r];
        const int ri = static_cast<int>(r);
        for (int x = s.x; x < s.x + s.largura; ++x) {
            if (aberta(g, x, s.y - 1)) tomar(x, s.y - 1, ri), tomar(x, s.y - 2, ri);
            if (aberta(g, x, s.y + s.altura)) tomar(x, s.y + s.altura, ri), tomar(x, s.y + s.altura + 1, ri);
        }
        for (int y = s.y; y < s.y + s.altura; ++y) {
            if (aberta(g, s.x - 1, y)) tomar(s.x - 1, y, ri), tomar(s.x - 2, y, ri);
            if (aberta(g, s.x + s.largura, y)) tomar(s.x + s.largura, y, ri), tomar(s.x + s.largura + 1, y, ri);
        }
    }
    return dono;
}

// Do início se chega a pé à chave e ao elevador, e de todo lugar aberto se volta a pé ao início
bool garantiaAPe(const Andar& a) {
    const Grade& g = a.grade;
    RegrasPassagem tudo = A_PE;
    tudo.atravessaTrancada = tudo.atravessaElevador = true;
    const glm::ivec2 inicio(glm::floor(a.inicio));
    const auto volta = distanciasComAltura(g, inicio, tudo);
    for (int y = 0; y < LADO_ANDAR; ++y)
        for (int x = 0; x < LADO_ANDAR; ++x)
            if (aberta(g, x, y) && g.em(x, y) != Celula::Elevador && volta[indice(x, y)] < 0) return false;
    if (distanciasComAltura(g, a.elevador, tudo)[indice(inicio.x, inicio.y)] < 0) return false;
    if (a.chave && distanciasComAltura(g, glm::ivec2(glm::floor(*a.chave)), A_PE)[indice(inicio.x, inicio.y)] < 0)
        return false;
    return true;
}

// Mudanças de um detalhe, para desfazer se a garantia quebrar
class Obra {
public:
    explicit Obra(Grade& g) : g_(g) {}
    void mudar(int x, int y, int chao, int teto, Rampa r = Rampa::Nenhuma, Detalhe d = Detalhe::Nenhum) {
        antes_.push_back({x, y, g_.chao(x, y), g_.teto(x, y), g_.rampa(x, y), g_.detalhe(x, y)});
        g_.definirAltura(x, y, chao, std::min(teto, TETO_MAXIMO));
        g_.definirRampa(x, y, r);
        g_.definirDetalhe(x, y, d);
    }
    void desfazer() {
        for (auto it = antes_.rbegin(); it != antes_.rend(); ++it) {
            g_.definirAltura(it->x, it->y, it->chao, it->teto);
            g_.definirRampa(it->x, it->y, it->rampa);
            g_.definirDetalhe(it->x, it->y, it->detalhe);
        }
        antes_.clear();
    }
    bool vazia() const { return antes_.empty(); }

private:
    struct Antes {
        int x, y, chao, teto;
        Rampa rampa;
        Detalhe detalhe;
    };
    Grade& g_;
    std::vector<Antes> antes_;
};

}

void aplicarRelevo(Andar& a, uint32_t semente) {
    Grade& g = a.grade;
    const auto& salas = a.salas;
    if (salas.empty()) return;
    std::mt19937 rng(semente ^ 0x5A17u);

    // 1. Nível de cada sala (andar 1 e esgoto: até 1 bloco)
    const bool plano = a.numero == 1 || a.zona == Zona::Esgoto;
    std::vector<int> nivel(salas.size()), tetoSala(salas.size());
    for (size_t r = 0; r < salas.size(); ++r) {
        nivel[r] = sortear(rng, 0, plano ? 1 : 3) * BLOCO;
        tetoSala[r] = sortear(rng, 8, 12);
    }
    if (a.arena) tetoSala[*a.arena] = TETO_ARENA;  // teto alto: a Grande Máquina chama voadores

    // 2. Platôs e distâncias de cada célula até cada platô
    size_t salaDoElevador = 0;
    for (size_t r = 0; r < salas.size(); ++r)
        if (salas[r].contem(a.elevador.x, a.elevador.y)) salaDoElevador = r;
    const std::vector<int> dono = donosDoPlato(g, salas, salaDoElevador);
    std::vector<std::vector<glm::ivec2>> plato(salas.size());
    for (int y = 0; y < LADO_ANDAR; ++y)
        for (int x = 0; x < LADO_ANDAR; ++x)
            if (dono[indice(x, y)] != SEM_SALA) plato[static_cast<size_t>(dono[indice(x, y)])].push_back({x, y});
    std::vector<std::vector<int>> distancia(salas.size());
    for (size_t r = 0; r < salas.size(); ++r) distancia[r] = distanciasPlanas(g, plato[r]);

    // 3. Sala alta demais para a distância até a vizinha desce até caber (no máximo 1 quarto por célula)
    for (bool mudou = true; mudou;) {
        mudou = false;
        for (size_t r = 0; r < salas.size(); ++r)
            for (size_t s = 0; s < salas.size(); ++s) {
                if (r == s) continue;
                int perto = 1 << 20;
                for (const glm::ivec2 c : plato[r])
                    if (const int d = distancia[s][indice(c.x, c.y)]; d >= 0) perto = std::min(perto, d);
                if (nivel[r] > nivel[s] + perto) {
                    nivel[r] = nivel[s] + perto;
                    mudou = true;
                }
            }
    }

    // 4. Chão: a média entre o teto "nível + distância" e o piso "nível − distância" das salas. As duas variam no
    //    máximo 1 quarto por célula (escada), igualam o nível no platô e a média não faz morro nem vale entre salas.
    const int nivelMaximo = *std::max_element(nivel.begin(), nivel.end());
    for (int y = 0; y < LADO_ANDAR; ++y)
        for (int x = 0; x < LADO_ANDAR; ++x) {
            if (!aberta(g, x, y)) continue;
            int cima = 1 << 20, baixo = -(1 << 20);
            for (size_t r = 0; r < salas.size(); ++r)
                if (const int d = distancia[r][indice(x, y)]; d >= 0) {
                    cima = std::min(cima, nivel[r] + d);
                    baixo = std::max(baixo, nivel[r] - d);
                }
            const int h = cima == 1 << 20 ? 0 : std::clamp((cima + baixo) / 2, 0, nivelMaximo);
            const int r = dono[indice(x, y)];
            const bool interior = r != SEM_SALA && salas[static_cast<size_t>(r)].contem(x, y);
            g.definirAltura(x, y, h, std::min(TETO_MAXIMO, h + (interior ? tetoSala[static_cast<size_t>(r)] : TETO_CORREDOR)));
        }
    // Degraus de escada nos corredores
    for (int y = 0; y < LADO_ANDAR; ++y)
        for (int x = 0; x < LADO_ANDAR; ++x) {
            if (!aberta(g, x, y)) continue;
            if (std::any_of(salas.begin(), salas.end(), [&](const Sala& s) { return s.contem(x, y); })) continue;
            for (const glm::ivec2 v : VIZINHOS)
                if (aberta(g, x + v.x, y + v.y) && g.chao(x + v.x, y + v.y) != g.chao(x, y)) g.definirDetalhe(x, y, Detalhe::Escada);
        }
    // Comporta comum torta vira passagem
    for (int y = 0; y < LADO_ANDAR; ++y)
        for (int x = 0; x < LADO_ANDAR; ++x) {
            if (g.em(x, y) != Celula::Comporta) continue;
            for (const glm::ivec2 v : VIZINHOS)
                if (aberta(g, x + v.x, y + v.y) && g.chao(x + v.x, y + v.y) != g.chao(x, y)) {
                    g.definir(x, y, Celula::Chao);
                    break;
                }
        }

    // 5. Detalhes, cada um desfeito se quebrar a garantia a pé
    std::mt19937 sorteio(semente ^ 0xDE7Au);
    const glm::ivec2 inicio(glm::floor(a.inicio));
    auto protegida = [&](int x, int y) {
        auto perto = [&](glm::ivec2 c, int raio) { return std::abs(c.x - x) <= raio && std::abs(c.y - y) <= raio; };
        if (perto(inicio, 2) || perto(a.elevador, 2) || (a.chave && perto(glm::ivec2(glm::floor(*a.chave)), 2))) return true;
        // Perto das aberturas da sala (entrada e saída livres)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = x + dx, ny = y + dy;
                if (!aberta(g, nx, ny)) continue;
                const int r = dono[indice(nx, ny)];
                if (r != SEM_SALA && !salas[static_cast<size_t>(r)].contem(nx, ny)) return true;
            }
        return false;
    };
    auto tentar = [&](Obra& obra) {
        if (obra.vazia()) return false;
        if (garantiaAPe(a)) return true;
        obra.desfazer();
        return false;
    };

    for (size_t r = 0; r < salas.size(); ++r) {
        if (a.arena && r == *a.arena) continue;  // arena do chefe: chão plano, sem plataforma, poço nem caixas
        const Sala& s = salas[r];
        const int L = nivel[r];
        const int base = L;
        auto livreNaBase = [&](int x, int y) {
            return s.contem(x, y) && !protegida(x, y) && g.chao(x, y) == base && g.rampa(x, y) == Rampa::Nenhuma &&
                   g.detalhe(x, y) == Detalhe::Nenhum && g.em(x, y) == Celula::Chao;
        };

        // Plataforma ou mezanino (fábrica; minas às vezes)
        const float chancePlataforma = a.zona == Zona::Fabrica ? 0.6f : a.zona == Zona::Minas ? 0.25f : 0.0f;
        if (r != 0 && s.largura >= 6 && s.altura >= 6 && L + BLOCO <= CHAO_MAXIMO && sortear01(sorteio) < chancePlataforma) {
            const int lado = sortear(sorteio, 0, 3), fundo = sortear(sorteio, 1, 2);
            const bool segredo = sortear01(sorteio) < CHANCE_SEGREDO;
            Obra obra(g);
            std::vector<glm::ivec2> faixa;
            for (int y = s.y; y < s.y + s.altura; ++y)
                for (int x = s.x; x < s.x + s.largura; ++x) {
                    const bool naFaixa = lado == 0 ? y < s.y + fundo : lado == 1 ? y >= s.y + s.altura - fundo
                                       : lado == 2 ? x < s.x + fundo : x >= s.x + s.largura - fundo;
                    if (naFaixa && livreNaBase(x, y)) faixa.push_back({x, y});
                }
            for (const glm::ivec2 c : faixa)
                obra.mudar(c.x, c.y, L + BLOCO, std::max(g.teto(c.x, c.y), L + BLOCO + ESPACO_MINIMO));
            bool comRampa = false;
            if (!faixa.empty() && !segredo) {
                // Rampa logo em frente ao meio da faixa, subindo na direção da parede
                const glm::ivec2 meio = faixa[faixa.size() / 2];
                const glm::ivec2 frente = lado == 0 ? glm::ivec2(meio.x, s.y + fundo) : lado == 1 ? glm::ivec2(meio.x, s.y + s.altura - fundo - 1)
                                        : lado == 2 ? glm::ivec2(s.x + fundo, meio.y) : glm::ivec2(s.x + s.largura - fundo - 1, meio.y);
                const Rampa sobe = lado == 0 ? Rampa::Norte : lado == 1 ? Rampa::Sul : lado == 2 ? Rampa::Oeste : Rampa::Leste;
                if (livreNaBase(frente.x, frente.y)) {
                    obra.mudar(frente.x, frente.y, L, std::max(g.teto(frente.x, frente.y), L + BLOCO + ESPACO_MINIMO), sobe);
                    comRampa = true;
                }
            }
            if (tentar(obra) && !comRampa && !faixa.empty()) {
                const glm::ivec2 meio = faixa[faixa.size() / 2];
                a.pontosSecretos.push_back(glm::vec2(meio) + 0.5f);
            }
        }

        // Poço (minas; fábrica às vezes)
        const float chancePoco = a.zona == Zona::Minas ? 0.5f : a.zona == Zona::Fabrica ? 0.15f : 0.0f;
        if (r != 0 && s.largura >= 7 && s.altura >= 7 && L >= BLOCO && sortear01(sorteio) < chancePoco) {
            const int fundo = std::min(L, sortear(sorteio, 1, 3) * BLOCO), rampas = fundo / BLOCO;
            const bool deitado = sortear01(sorteio) < 0.5f;  // comprido em x
            const int comprido = std::max(sortear(sorteio, 2, 3), rampas + 1), curto = sortear(sorteio, 2, 3);
            const int w = deitado ? comprido : curto, h = deitado ? curto : comprido;
            if (w <= s.largura - 2 && h <= s.altura - 2) {
                const int px = sortear(sorteio, s.x + 1, s.x + s.largura - 1 - w), py = sortear(sorteio, s.y + 1, s.y + s.altura - 1 - h);
                bool cabe = true;
                for (int y = py; y < py + h; ++y)
                    for (int x = px; x < px + w; ++x) cabe &= livreNaBase(x, y);
                if (cabe) {
                    Obra obra(g);
                    for (int y = py; y < py + h; ++y)
                        for (int x = px; x < px + w; ++x) obra.mudar(x, y, L - fundo, g.teto(x, y));
                    // Saída: fila de rampas no fim do lado comprido, subindo até o chão da sala
                    for (int j = 0; j < rampas; ++j) {
                        const int x = deitado ? px + w - rampas + j : px, y = deitado ? py : py + h - rampas + j;
                        obra.mudar(x, y, L - fundo + j * BLOCO, g.teto(x, y), deitado ? Rampa::Leste : Rampa::Sul);
                    }
                    tentar(obra);
                }
            }
        }

        // Caixas (cobertura e degrau para pular)
        const int caixas = a.zona == Zona::Fabrica ? sortear(sorteio, 2, 4) : a.zona == Zona::Minas ? sortear(sorteio, 0, 2)
                                                                                                  : sortear(sorteio, 0, 1);
        for (int k = 0; k < caixas; ++k) {
            const int x = sortear(sorteio, s.x, s.x + s.largura - 1), y = sortear(sorteio, s.y, s.y + s.altura - 1);
            if (!livreNaBase(x, y)) continue;
            const bool pilha = sortear01(sorteio) < CHANCE_PILHA && L + 2 * BLOCO <= CHAO_MAXIMO;
            if (L + BLOCO > CHAO_MAXIMO) continue;
            Obra obra(g);
            obra.mudar(x, y, L + BLOCO, std::max(g.teto(x, y), L + BLOCO + ESPACO_MINIMO), Rampa::Nenhuma, Detalhe::Caixa);
            // Pilha de 2 sempre colada numa caixa de 1 (o degrau para chegar lá pulando)
            if (pilha)
                for (const glm::ivec2 v : VIZINHOS)
                    if (livreNaBase(x + v.x, y + v.y)) {
                        const int topo = L + 2 * BLOCO;
                        obra.mudar(x + v.x, y + v.y, topo, std::max(g.teto(x + v.x, y + v.y), topo + ESPACO_MINIMO),
                                   Rampa::Nenhuma, Detalhe::Caixa);
                        break;
                    }
            tentar(obra);
        }
    }
}

}
