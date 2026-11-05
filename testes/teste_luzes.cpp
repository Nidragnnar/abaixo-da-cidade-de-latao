#include <doctest/doctest.h>

#include <cmath>
#include <ostream>  // o doctest precisa para mostrar string

#include <glm/geometric.hpp>

#include "mundo/Gerador.h"
#include "mundo/Luzes.h"
#include "mundo/Terreno.h"
#include "render/Luzes.h"

namespace {

bool dentroDeSala(const mundo::Andar& a, int x, int y) {
    for (const auto& s : a.salas)
        if (s.contem(x, y)) return true;
    return false;
}

}

TEST_CASE("toda sala tem ao menos uma luz") {
    for (uint32_t s = 1; s <= 200; ++s)
        for (int n = 1; n <= 6; ++n) {
            const auto a = mundo::gerarAndar(s, n);
            for (const auto& sala : a.salas) {
                bool tem = false;
                for (const auto& l : a.luzes)
                    tem |= l.posicao.x >= sala.x - 0.5f && l.posicao.x <= sala.x + sala.largura + 0.5f &&
                           l.posicao.z >= sala.y - 0.5f && l.posicao.z <= sala.y + sala.altura + 0.5f;
                INFO("semente ", s, " andar ", n);
                REQUIRE(tem);
            }
        }
}

TEST_CASE("corredor nunca fica mais de 10 células sem luz") {
    for (uint32_t s = 1; s <= 200; ++s) {
        const auto a = mundo::gerarAndar(s, 1 + static_cast<int>(s % 6));
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                if (a.grade.em(x, y) == mundo::Celula::Parede || dentroDeSala(a, x, y)) continue;
                float perto = 1e9f;
                for (const auto& l : a.luzes)
                    perto = std::min(perto, glm::distance(glm::vec2(x + 0.5f, y + 0.5f), glm::vec2(l.posicao.x, l.posicao.z)));
                INFO("semente ", s, " célula ", x, ",", y);
                REQUIRE(perto <= 5.0f);
            }
    }
}

TEST_CASE("luz presa na parede, acima do chão") {
    for (uint32_t s = 1; s <= 50; ++s) {
        const auto a = mundo::gerarAndar(s, 3);
        REQUIRE_FALSE(a.luzes.empty());
        for (const auto& l : a.luzes) {
            if (l.tipo != mundo::TipoLuz::Lampiao) continue;  // os lustres ficam no teto (teste abaixo)
            const int cx = static_cast<int>(std::floor(l.posicao.x)), cz = static_cast<int>(std::floor(l.posicao.z));
            REQUIRE(a.grade.em(cx, cz) == mundo::Celula::Chao);  // nunca dentro de comporta (a placa atravessaria o lampião)
            // Encostada numa parede: a parte fracionária perto da borda, e do outro lado é parede
            const float fx = l.posicao.x - cx, fz = l.posicao.z - cz;
            const bool encostada = (fx < 0.1f && a.grade.em(cx - 1, cz) == mundo::Celula::Parede) ||
                                   (fx > 0.9f && a.grade.em(cx + 1, cz) == mundo::Celula::Parede) ||
                                   (fz < 0.1f && a.grade.em(cx, cz - 1) == mundo::Celula::Parede) ||
                                   (fz > 0.9f && a.grade.em(cx, cz + 1) == mundo::Celula::Parede);
            REQUIRE(encostada);
            CHECK(l.posicao.y == doctest::Approx(mundo::alturaDoChao(a.grade, {l.posicao.x, l.posicao.z}) + mundo::ALTURA_LUZ));
            CHECK(l.alcance == doctest::Approx(mundo::ALCANCE_LUZ));
        }
    }
}

TEST_CASE("mesma semente, mesmas luzes") {
    const auto a = mundo::gerarAndar(7, 3), b = mundo::gerarAndar(7, 3);
    REQUIRE(a.luzes.size() == b.luzes.size());
    for (size_t i = 0; i < a.luzes.size(); ++i) {
        CHECK(a.luzes[i].posicao == b.luzes[i].posicao);
        CHECK(a.luzes[i].fase == b.luzes[i].fase);
    }
    CHECK(mundo::andarDeTexto({"###", "#@#", "###"}, 1).luzes.empty());
}

TEST_CASE("luzes mais próximas: no máximo 16, da mais perto para a mais longe") {
    std::vector<mundo::Luz> linha;
    for (int i = 30; i >= 1; --i) linha.push_back({{static_cast<float>(i), 0, 0}, {1, 1, 1}, 5, 0});
    const auto perto = render::luzesMaisProximas(linha, {0, 0, 0});
    REQUIRE(perto.size() == static_cast<size_t>(render::MAXIMO_LUZES));
    for (int i = 0; i < render::MAXIMO_LUZES; ++i) CHECK(perto[static_cast<size_t>(i)].posicao.x == doctest::Approx(i + 1.0f));
    const std::vector<mundo::Luz> tres(linha.begin(), linha.begin() + 3);
    CHECK(render::luzesMaisProximas(tres, {0, 0, 0}).size() == 3);
}

TEST_CASE("tremulação fica entre 0,9 e 1,1") {
    for (float fase : {0.0f, 1.7f, 4.2f})
        for (int i = 0; i < 1000; ++i) {
            const float t = render::tremulacao(i * 0.017f, fase);
            REQUIRE(t >= 0.9f - 1e-5f);
            REQUIRE(t <= 1.1f + 1e-5f);
        }
}

TEST_CASE("toda sala tem lustre no teto, 2 nas grandes") {
    for (uint32_t s = 1; s <= 100; ++s) {
        const auto a = mundo::gerarAndar(s, 1 + static_cast<int>(s % 6));
        for (const auto& sala : a.salas) {
            int lustres = 0;
            for (const auto& l : a.luzes) {
                if (l.tipo != mundo::TipoLuz::Lustre) continue;
                if (l.posicao.x < sala.x || l.posicao.x > sala.x + sala.largura || l.posicao.z < sala.y ||
                    l.posicao.z > sala.y + sala.altura)
                    continue;
                ++lustres;
                const glm::vec2 plano{l.posicao.x, l.posicao.z};
                CHECK(l.posicao.y == doctest::Approx(mundo::alturaDoTeto(a.grade, plano) - mundo::DESCIDA_LUSTRE));
                CHECK(l.alcance == doctest::Approx(mundo::ALCANCE_LUSTRE));
            }
            INFO("semente ", s, " sala ", sala.x, ",", sala.y);
            CHECK(lustres == (sala.largura * sala.altura < 30 ? 1 : 2));
        }
    }
}
