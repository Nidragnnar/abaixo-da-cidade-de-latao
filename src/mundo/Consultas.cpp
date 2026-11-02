#include "mundo/Consultas.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>

namespace mundo {

bool temLinhaDeVisao(const Grade& g, glm::vec2 de, glm::vec2 para) {
    int cx = static_cast<int>(std::floor(de.x)), cy = static_cast<int>(std::floor(de.y));
    const int alvoX = static_cast<int>(std::floor(para.x)), alvoY = static_cast<int>(std::floor(para.y));
    const glm::vec2 d = para - de;
    const int passoX = d.x > 0 ? 1 : -1, passoY = d.y > 0 ? 1 : -1;
    constexpr float INF = std::numeric_limits<float>::infinity();
    // Distância (em fração do segmento) até a próxima borda de célula em cada eixo
    const float deltaX = d.x != 0 ? std::abs(1.0f / d.x) : INF;
    const float deltaY = d.y != 0 ? std::abs(1.0f / d.y) : INF;
    float tX = d.x != 0 ? ((passoX > 0 ? cx + 1 - de.x : de.x - cx) * deltaX) : INF;
    float tY = d.y != 0 ? ((passoY > 0 ? cy + 1 - de.y : de.y - cy) * deltaY) : INF;

    auto bloqueia = [&](int x, int y) { return !(x == alvoX && y == alvoY) && g.solida(x, y); };
    while (cx != alvoX || cy != alvoY) {
        if (std::abs(tX - tY) < 1e-6f) {
            // Passa exatamente pela quina: as duas vizinhas precisam estar livres
            if (bloqueia(cx + passoX, cy) || bloqueia(cx, cy + passoY)) return false;
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
        if (bloqueia(cx, cy)) return false;
        if (tX > 1.0f + 1e-4f && tY > 1.0f + 1e-4f) break;  // segurança contra erro de arredondamento
    }
    return true;
}

Exploracao::Exploracao(int largura, int altura)
    : largura_(largura), altura_(altura), vistas_(static_cast<size_t>(largura) * altura, false) {}

bool Exploracao::vista(int x, int y) const {
    if (x < 0 || y < 0 || x >= largura_ || y >= altura_) return false;
    return vistas_[static_cast<size_t>(y) * largura_ + x];
}

void Exploracao::marcar(int x, int y) {
    if (x < 0 || y < 0 || x >= largura_ || y >= altura_) return;
    vistas_[static_cast<size_t>(y) * largura_ + x] = true;
}

void Exploracao::marcarTudo() { std::fill(vistas_.begin(), vistas_.end(), true); }

void explorarAoRedor(Exploracao& e, const Grade& g, glm::vec2 posicao, float raio) {
    const int x0 = static_cast<int>(std::floor(posicao.x - raio)), x1 = static_cast<int>(std::floor(posicao.x + raio));
    const int y0 = static_cast<int>(std::floor(posicao.y - raio)), y1 = static_cast<int>(std::floor(posicao.y + raio));
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            const glm::vec2 centro{x + 0.5f, y + 0.5f};
            const glm::vec2 d = centro - posicao;
            if (d.x * d.x + d.y * d.y > raio * raio || e.vista(x, y)) continue;
            if (temLinhaDeVisao(g, posicao, centro)) e.marcar(x, y);
        }
}

std::optional<glm::ivec2> celulaAFrente(const Grade& g, glm::vec2 posicao, float angulo, float alcance) {
    const glm::vec2 direcao{std::cos(angulo), std::sin(angulo)};
    for (float d = 0.0f; d <= alcance; d += 0.05f) {
        const glm::vec2 p = posicao + direcao * d;
        const glm::ivec2 c{static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y))};
        const Celula tipo = g.em(c.x, c.y);
        if (tipo != Celula::Chao && tipo != Celula::ComportaAberta) return c;
    }
    return std::nullopt;
}

std::vector<int> distanciasNaGrade(const Grade& g, glm::ivec2 origem, bool atravessaTrancada) {
    const int L = g.largura(), A = g.altura();
    std::vector<int> dist(static_cast<size_t>(L) * A, -1);
    auto passa = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= L || y >= A) return false;
        const Celula c = g.em(x, y);
        return c != Celula::Parede && (atravessaTrancada || c != Celula::ComportaTrancada);
    };
    if (!passa(origem.x, origem.y)) return dist;

    std::deque<glm::ivec2> fila{origem};
    dist[static_cast<size_t>(origem.y) * L + origem.x] = 0;
    constexpr glm::ivec2 VIZINHOS[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!fila.empty()) {
        const glm::ivec2 c = fila.front();
        fila.pop_front();
        const int atual = dist[static_cast<size_t>(c.y) * L + c.x];
        for (const auto& v : VIZINHOS) {
            const glm::ivec2 n = c + v;
            if (!passa(n.x, n.y)) continue;
            int& dn = dist[static_cast<size_t>(n.y) * L + n.x];
            if (dn >= 0) continue;
            dn = atual + 1;
            fila.push_back(n);
        }
    }
    return dist;
}

}
