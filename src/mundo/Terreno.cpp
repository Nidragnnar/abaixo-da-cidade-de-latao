#include "mundo/Terreno.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <limits>

#include <glm/common.hpp>

namespace mundo {

namespace {

constexpr int SUBIDA_RAMPA = 4;     // quartos ao longo da célula
constexpr int ESPACO_DO_CORPO = 3;  // quartos (0,75 ≥ 0,7)

// Direção para onde a rampa sobe, como vetor da grade (Norte = −y)
glm::ivec2 sentido(Rampa r) {
    switch (r) {
        case Rampa::Norte: return {0, -1};
        case Rampa::Sul: return {0, 1};
        case Rampa::Leste: return {1, 0};
        case Rampa::Oeste: return {-1, 0};
        default: return {0, 0};
    }
}

}

float alturaDoChao(const Grade& g, glm::vec2 p) {
    const int x = static_cast<int>(std::floor(p.x)), y = static_cast<int>(std::floor(p.y));
    const float base = g.chao(x, y) * QUARTO;
    const glm::ivec2 s = sentido(g.rampa(x, y));
    if (s == glm::ivec2(0)) return base;
    // Fração percorrida no sentido da subida (0 na borda de baixo, 1 na de cima)
    const float fx = p.x - static_cast<float>(x), fy = p.y - static_cast<float>(y);
    const float t = s.x > 0 ? fx : s.x < 0 ? 1.0f - fx : s.y > 0 ? fy : 1.0f - fy;
    return base + std::clamp(t, 0.0f, 1.0f) * SUBIDA_RAMPA * QUARTO;
}

float alturaDoTeto(const Grade& g, glm::vec2 p) {
    return g.teto(static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y))) * QUARTO;
}

float chaoNaBorda(const Grade& g, glm::ivec2 c, glm::ivec2 direcao) {
    const float base = g.chao(c.x, c.y) * QUARTO;
    const glm::ivec2 s = sentido(g.rampa(c.x, c.y));
    if (s == glm::ivec2(0)) return base;
    const float topo = base + SUBIDA_RAMPA * QUARTO;
    if (direcao == s) return topo;
    if (direcao == -s) return base;
    return (base + topo) / 2.0f;  // lateral
}

float chaoSobCorpo(const Grade& g, glm::vec2 posicao, float raio) {
    float h = alturaDoChao(g, posicao);
    for (const glm::vec2 canto : {glm::vec2(-raio, -raio), glm::vec2(raio, -raio), glm::vec2(-raio, raio), glm::vec2(raio, raio)}) {
        const glm::vec2 p = posicao + canto;
        const int x = static_cast<int>(std::floor(p.x)), y = static_cast<int>(std::floor(p.y));
        if (g.em(x, y) == Celula::Parede || g.rampa(x, y) != Rampa::Nenhuma) continue;
        h = std::max(h, alturaDoChao(g, p));
    }
    return h;
}

float atualizarVertical(CorpoVertical& c, const Grade& g, glm::vec2 posicao, float raio, float dt, float alturaCorpo) {
    const float chao = chaoSobCorpo(g, posicao, raio);
    float teto = alturaDoTeto(g, posicao);
    for (const glm::vec2 canto : {glm::vec2(-raio, -raio), glm::vec2(raio, -raio), glm::vec2(-raio, raio), glm::vec2(raio, raio)})
        if (g.em(static_cast<int>(std::floor(posicao.x + canto.x)), static_cast<int>(std::floor(posicao.y + canto.y))) != Celula::Parede)
            teto = std::min(teto, alturaDoTeto(g, posicao + canto));

    if (c.noChao) {
        if (c.velocidade > 0.0f) {
            c.noChao = false;  // pulo
            c.maisAlto = c.pes;
        } else if (chao < c.pes - QUARTO - 1e-3f) {
            c.noChao = false;  // o chão sumiu: começa a cair
            c.maisAlto = c.pes;
            c.velocidade = 0.0f;
        } else {
            c.pes = chao;  // degrau e rampa: os pés acompanham o chão
            c.velocidade = 0.0f;
            return 0.0f;
        }
    }
    c.velocidade -= GRAVIDADE * dt;
    c.pes += c.velocidade * dt;
    c.maisAlto = std::max(c.maisAlto, c.pes);
    if (c.pes + alturaCorpo > teto) {  // cabeça no teto
        c.pes = teto - alturaCorpo;
        c.velocidade = std::min(c.velocidade, 0.0f);
    }
    if (c.pes <= chao) {
        const float queda = c.maisAlto - chao;
        c.pes = chao;
        c.velocidade = 0.0f;
        c.noChao = true;
        return std::max(0.0f, queda);
    }
    return 0.0f;
}

int danoDeQueda(float blocos) {
    if (blocos <= 2.0f) return 0;
    return 5 * static_cast<int>(std::floor((blocos - 2.0f) / 0.5f + 1e-4f));
}

bool podePassar(const Grade& g, glm::ivec2 a, glm::ivec2 b, const RegrasPassagem& r) {
    const glm::ivec2 d = b - a;
    if (std::abs(d.x) + std::abs(d.y) != 1) return false;
    switch (g.em(b.x, b.y)) {
        case Celula::Parede: return false;
        case Celula::ComportaTrancada:
            if (!r.atravessaTrancada) return false;
            break;
        case Celula::Elevador:
            if (!r.atravessaElevador) return false;
            break;
        default: break;
    }
    const float diferenca = (chaoNaBorda(g, b, -d) - chaoNaBorda(g, a, d)) / QUARTO;
    if (diferenca > static_cast<float>(r.subida) + 1e-3f || diferenca < -static_cast<float>(r.descida) - 1e-3f) return false;
    const int topoDoChao = g.chao(b.x, b.y) + (g.rampa(b.x, b.y) != Rampa::Nenhuma ? SUBIDA_RAMPA : 0);
    return g.teto(b.x, b.y) - topoDoChao >= ESPACO_DO_CORPO;
}

}

namespace mundo {

std::vector<int> distanciasComAltura(const Grade& g, glm::ivec2 alvo, const RegrasPassagem& r) {
    const int L = g.largura(), A = g.altura();
    std::vector<int> dist(static_cast<size_t>(L) * A, -1);
    auto indice = [&](glm::ivec2 c) { return static_cast<size_t>(c.y) * L + c.x; };
    if (alvo.x < 0 || alvo.y < 0 || alvo.x >= L || alvo.y >= A) return dist;
    std::deque<glm::ivec2> fila{alvo};
    dist[indice(alvo)] = 0;
    while (!fila.empty()) {
        const glm::ivec2 b = fila.front();
        fila.pop_front();
        for (const glm::ivec2 d : {glm::ivec2(1, 0), glm::ivec2(-1, 0), glm::ivec2(0, 1), glm::ivec2(0, -1)}) {
            const glm::ivec2 a = b + d;
            if (a.x < 0 || a.y < 0 || a.x >= L || a.y >= A || dist[indice(a)] >= 0) continue;
            // Quem está em `a` precisa conseguir andar até `b`; e `a` precisa ser um lugar onde se fica
            if (g.em(a.x, a.y) == Celula::Parede || !podePassar(g, a, b, r)) continue;
            dist[indice(a)] = dist[indice(b)] + 1;
            fila.push_back(a);
        }
    }
    return dist;
}

bool temLinhaDeVisao3D(const Grade& g, glm::vec3 de, glm::vec3 para) {
    const glm::vec2 a{de.x, de.z}, b{para.x, para.z};
    const glm::vec2 d = b - a;
    int cx = static_cast<int>(std::floor(a.x)), cy = static_cast<int>(std::floor(a.y));
    const int alvoX = static_cast<int>(std::floor(b.x)), alvoY = static_cast<int>(std::floor(b.y));
    const int passoX = d.x > 0 ? 1 : -1, passoY = d.y > 0 ? 1 : -1;
    constexpr float INF = std::numeric_limits<float>::infinity();
    const float deltaX = d.x != 0 ? std::abs(1.0f / d.x) : INF;
    const float deltaY = d.y != 0 ? std::abs(1.0f / d.y) : INF;
    float tX = d.x != 0 ? ((passoX > 0 ? cx + 1 - a.x : a.x - cx) * deltaX) : INF;
    float tY = d.y != 0 ? ((passoY > 0 ? cy + 1 - a.y : a.y - cy) * deltaY) : INF;

    // O raio entre t0 e t1 dentro da célula (x, y) fica entre o chão e o teto dela?
    auto livreNaCelula = [&](int x, int y, float t0, float t1) {
        if (!(x == alvoX && y == alvoY) && g.solida(x, y)) return false;
        for (const float t : {t0, t1}) {
            const glm::vec3 p = de + (para - de) * std::clamp(t, 0.0f, 1.0f);
            const glm::vec2 dentro = glm::clamp(glm::vec2(p.x, p.z), glm::vec2(x, y), glm::vec2(x + 1, y + 1) - 1e-4f);
            if (p.y < alturaDoChao(g, dentro) - 1e-3f || p.y > alturaDoTeto(g, dentro) + 1e-3f) return false;
        }
        return true;
    };

    float t = 0.0f;
    for (int passos = 0; passos < 4 * (g.largura() + g.altura()); ++passos) {
        const float saida = std::min({tX, tY, 1.0f});
        if (!livreNaCelula(cx, cy, t, saida)) return false;
        if (cx == alvoX && cy == alvoY) return true;
        if (saida >= 1.0f) return true;  // arredondamento: já chegou ao fim do segmento
        if (std::abs(tX - tY) < 1e-6f) {
            // Quina: as duas vizinhas precisam estar livres
            if (!livreNaCelula(cx + passoX, cy, saida, saida) || !livreNaCelula(cx, cy + passoY, saida, saida)) return false;
            cx += passoX;
            cy += passoY;
            tX += deltaX;
            tY += deltaY;
        } else if (tX < tY) {
            cx += passoX;
            tX += deltaX;
        } else {
            cy += passoY;
            tY += deltaY;
        }
        t = saida;
    }
    return false;
}

}
