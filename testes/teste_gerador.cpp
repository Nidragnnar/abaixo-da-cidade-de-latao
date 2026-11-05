#include <doctest/doctest.h>

#include <algorithm>
#include <map>
#include <string>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include "mundo/Consultas.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"

namespace {

bool tudoAlcancavel(const mundo::Andar& a) {
    auto d = mundo::distanciasNaGrade(a.grade, glm::ivec2(a.inicio), true);
    for (int y = 0; y < a.grade.altura(); ++y)
        for (int x = 0; x < a.grade.largura(); ++x)
            if (a.grade.em(x, y) != mundo::Celula::Parede && d[y * a.grade.largura() + x] < 0) return false;
    return true;
}

int contarTrancas(const mundo::Andar& a) {
    int n = 0;
    for (int y = 0; y < a.grade.altura(); ++y)
        for (int x = 0; x < a.grade.largura(); ++x) n += a.grade.em(x, y) == mundo::Celula::ComportaTrancada;
    return n;
}

}

TEST_CASE("mesma semente gera o mesmo andar") {
    auto a = mundo::gerarAndar(1234, 2), b = mundo::gerarAndar(1234, 2);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) REQUIRE(a.grade.em(x, y) == b.grade.em(x, y));
    CHECK(a.inicio == b.inicio);
}

TEST_CASE("sementes ou andares diferentes geram andares diferentes") {
    auto a = mundo::gerarAndar(1, 1), b = mundo::gerarAndar(2, 1), c = mundo::gerarAndar(1, 2);
    bool difAB = false, difAC = false;
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            difAB |= a.grade.em(x, y) != b.grade.em(x, y);
            difAC |= a.grade.em(x, y) != c.grade.em(x, y);
        }
    CHECK(difAB);
    CHECK(difAC);
}

TEST_CASE("invariantes em 1000 sementes") {
    for (uint32_t s = 1; s <= 1000; ++s)
        for (int andar = 1; andar <= 3; ++andar) {
            auto a = mundo::gerarAndar(s, andar);
            INFO("semente " << s << " andar " << andar);
            REQUIRE(a.grade.largura() == 64);
            REQUIRE(a.salas.size() >= 8);
            REQUIRE(a.salas.size() <= 15);
            REQUIRE(tudoAlcancavel(a));
            REQUIRE(a.grade.em(a.elevador.x, a.elevador.y) == mundo::Celula::Elevador);
            REQUIRE(contarTrancas(a) <= 1);
            REQUIRE(a.chave.has_value() == (contarTrancas(a) == 1));
        }
}

TEST_CASE("início em célula de chão longe do elevador") {
    for (uint32_t s = 1; s <= 200; ++s) {
        auto a = mundo::gerarAndar(s, 1);
        INFO("semente " << s);
        REQUIRE(a.grade.em(int(a.inicio.x), int(a.inicio.y)) == mundo::Celula::Chao);
        REQUIRE(glm::distance(a.inicio, glm::vec2(a.elevador) + 0.5f) > 8.0f);
    }
}

TEST_CASE("chave alcançável sem passar pela tranca, elevador só com ela") {
    int comTranca = 0;
    for (uint32_t s = 1; s <= 500; ++s) {
        auto a = mundo::gerarAndar(s, 2);
        if (!a.chave) continue;
        ++comTranca;
        INFO("semente " << s);
        auto d = mundo::distanciasNaGrade(a.grade, glm::ivec2(a.inicio), false);
        auto idx = [&](glm::ivec2 c) { return c.y * 64 + c.x; };
        REQUIRE(d[idx(glm::ivec2(*a.chave))] >= 0);
        REQUIRE(d[idx(a.elevador)] < 0);
    }
    CHECK(comTranca > 500 * 0.25);  // alvo da spec 40%; algumas variações desistem
    CHECK(comTranca < 500 * 0.50);
}

TEST_CASE("andar 1 nunca tem tranca") {
    for (uint32_t s = 1; s <= 300; ++s) REQUIRE_FALSE(mundo::gerarAndar(s, 1).chave.has_value());
}

TEST_CASE("comportas ficam entre duas paredes") {
    using mundo::Celula;
    for (uint32_t s = 1; s <= 200; ++s) {
        auto a = mundo::gerarAndar(s, 2);
        for (int y = 1; y < 63; ++y)
            for (int x = 1; x < 63; ++x) {
                auto c = a.grade.em(x, y);
                if (c != Celula::Comporta && c != Celula::ComportaTrancada) continue;
                INFO("semente " << s << " célula " << x << "," << y);
                bool horizontal = a.grade.em(x, y - 1) == Celula::Parede && a.grade.em(x, y + 1) == Celula::Parede;
                bool vertical = a.grade.em(x - 1, y) == Celula::Parede && a.grade.em(x + 1, y) == Celula::Parede;
                REQUIRE((horizontal || vertical));
            }
    }
}

TEST_CASE("andar 50 gera na zona das minas") {
    auto a = mundo::gerarAndar(77, 50);
    CHECK(a.zona == mundo::Zona::Minas);
    CHECK(a.numero == 50);
}

namespace {
// Impressão digital do desenho 2D (aberto/parede, início, elevador, chave) de 30 andares
uint64_t digitalDoLayout() {
    uint64_t h = 1469598103934665603ull;
    auto mistura = [&](uint64_t v) { h = (h ^ v) * 1099511628211ull; };
    for (uint32_t s = 1; s <= 10; ++s)
        for (int n = 1; n <= 3; ++n) {
            const auto a = mundo::gerarAndar(s, n);
            for (int y = 0; y < 64; ++y)
                for (int x = 0; x < 64; ++x) mistura(a.grade.em(x, y) == mundo::Celula::Parede ? 1 : 2);
            mistura(static_cast<uint64_t>(a.inicio.x * 10)); mistura(static_cast<uint64_t>(a.inicio.y * 10));
            mistura(static_cast<uint64_t>(a.elevador.x)); mistura(static_cast<uint64_t>(a.elevador.y));
            mistura(a.chave ? static_cast<uint64_t>(a.chave->x * 10 + a.chave->y * 1000) : 7);
        }
    return h;
}
}

TEST_CASE("layout 2D igual ao da v1") {
    CHECK(digitalDoLayout() == 14210815957670128114ull);  // valor da v1, gravado antes da Etapa 6A
}

namespace {
// 1000 andares variados: semente s no andar 1 + s % 6 (todas as zonas, com e sem tranca)
template <typename Verificar>
void emMilAndares(Verificar verificar) {
#ifdef NDEBUG
    const uint32_t total = 1000;
#else
    const uint32_t total = 150;  // Debug é bem mais lento
#endif
    for (uint32_t s = 1; s <= total; ++s) {
        const int n = 1 + static_cast<int>(s % 6);
        const auto a = mundo::gerarAndar(s, n);
        INFO("semente ", s, " andar ", n);
        verificar(a);
    }
}

bool aberta(const mundo::Andar& a, int x, int y) { return a.grade.em(x, y) != mundo::Celula::Parede; }
int indiceDe(glm::ivec2 c) { return c.y * 64 + c.x; }
}

TEST_CASE("caminho a pé até a chave e o elevador") {
    emMilAndares([](const mundo::Andar& a) {
        const glm::ivec2 inicio(glm::floor(a.inicio));
        mundo::RegrasPassagem tudo = mundo::A_PE;
        tudo.atravessaTrancada = tudo.atravessaElevador = true;
        REQUIRE(mundo::distanciasComAltura(a.grade, a.elevador, tudo)[indiceDe(inicio)] >= 0);
        if (a.chave) REQUIRE(mundo::distanciasComAltura(a.grade, glm::ivec2(glm::floor(*a.chave)), mundo::A_PE)[indiceDe(inicio)] >= 0);
    });
}

TEST_CASE("todo teto pelo menos 1,25 acima do chão; chão e teto nos limites") {
    emMilAndares([](const mundo::Andar& a) {
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                if (!aberta(a, x, y)) continue;
                const int topo = a.grade.chao(x, y) + (a.grade.rampa(x, y) != mundo::Rampa::Nenhuma ? 4 : 0);
                REQUIRE(a.grade.teto(x, y) - topo >= 5);
                REQUIRE(a.grade.chao(x, y) >= 0);
                REQUIRE(topo <= 12);
                REQUIRE(a.grade.teto(x, y) <= 20);
            }
    });
}

TEST_CASE("comportas e elevador em trecho plano") {
    emMilAndares([](const mundo::Andar& a) {
        auto plano = [&](int x, int y) {
            for (const glm::ivec2 d : {glm::ivec2(1, 0), glm::ivec2(-1, 0), glm::ivec2(0, 1), glm::ivec2(0, -1)}) {
                if (!aberta(a, x + d.x, y + d.y)) continue;
                if (a.grade.chao(x + d.x, y + d.y) != a.grade.chao(x, y) || a.grade.rampa(x + d.x, y + d.y) != mundo::Rampa::Nenhuma)
                    return false;
            }
            return true;
        };
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                const auto c = a.grade.em(x, y);
                if (c == mundo::Celula::Comporta || c == mundo::Celula::ComportaTrancada || c == mundo::Celula::Elevador) {
                    INFO("célula ", x, ",", y, " tipo ", static_cast<int>(c));
                    REQUIRE(plano(x, y));
                }
            }
    });
}

TEST_CASE("todo lugar aberto tem saída a pé (poços incluídos)") {
    emMilAndares([](const mundo::Andar& a) {
        mundo::RegrasPassagem tudo = mundo::A_PE;
        tudo.atravessaTrancada = tudo.atravessaElevador = true;
        const auto d = mundo::distanciasComAltura(a.grade, glm::ivec2(glm::floor(a.inicio)), tudo);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                if (aberta(a, x, y) && a.grade.em(x, y) != mundo::Celula::Elevador) REQUIRE(d[indiceDe({x, y})] >= 0);
    });
}

TEST_CASE("andar 1 e esgoto com no máximo 1 bloco de desnível") {
    emMilAndares([](const mundo::Andar& a) {
        if (a.numero != 1 && a.zona != mundo::Zona::Esgoto) return;
        int menor = 99, maior = -99;
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                if (aberta(a, x, y) && a.grade.detalhe(x, y) != mundo::Detalhe::Caixa) {  // caixas soltas não contam
                    menor = std::min(menor, a.grade.chao(x, y));
                    maior = std::max(maior, a.grade.chao(x, y) + (a.grade.rampa(x, y) != mundo::Rampa::Nenhuma ? 4 : 0));
                }
        REQUIRE(maior - menor <= 4);
    });
}

TEST_CASE("andares têm desníveis de verdade") {
    int comNivel = 0, comCaixa = 0, comPlataformaOuPoco = 0;
    for (uint32_t s = 1; s <= 100; ++s) {
        const auto a = mundo::gerarAndar(s, 3);
        bool nivel = false, caixa = false, rampa = false;
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                if (!aberta(a, x, y)) continue;
                nivel |= a.grade.chao(x, y) >= 4 && a.grade.detalhe(x, y) != mundo::Detalhe::Caixa;
                caixa |= a.grade.detalhe(x, y) == mundo::Detalhe::Caixa;
                rampa |= a.grade.rampa(x, y) != mundo::Rampa::Nenhuma;
            }
        comNivel += nivel;
        comCaixa += caixa;
        comPlataformaOuPoco += rampa || !a.pontosSecretos.empty();
    }
    CHECK(comNivel >= 80);
    CHECK(comCaixa >= 50);
    CHECK(comPlataformaOuPoco >= 30);
}

TEST_CASE("ponto secreto só se alcança pulando") {
    int secretos = 0;
    emMilAndares([&](const mundo::Andar& a) {
        mundo::RegrasPassagem tudo = mundo::A_PE;
        tudo.atravessaTrancada = tudo.atravessaElevador = true;
        const auto d = mundo::distanciasComAltura(a.grade, glm::ivec2(glm::floor(a.inicio)), tudo);
        for (const glm::vec2 p : a.pontosSecretos) {
            ++secretos;
            // Do início não se chega a pé: a busca a partir do segredo até o início existe (desce), mas o contrário não
            const auto ate = mundo::distanciasComAltura(a.grade, glm::ivec2(glm::floor(p)), tudo);
            REQUIRE(ate[indiceDe(glm::ivec2(glm::floor(a.inicio)))] < 0);
            REQUIRE(d[indiceDe(glm::ivec2(glm::floor(p)))] >= 0);
        }
    });
    CHECK(secretos > 0);
}


TEST_CASE("pilha de caixas tem caixa de 1 bloco ao lado") {
    emMilAndares([](const mundo::Andar& a) {
        for (const auto& s : a.salas) {
            // Nível da sala: o chão mais comum dentro dela (plataformas, poços e caixas são minoria)
            std::map<int, int> contagem;
            for (int y = s.y; y < s.y + s.altura; ++y)
                for (int x = s.x; x < s.x + s.largura; ++x) ++contagem[a.grade.chao(x, y)];
            const int nivel = std::max_element(contagem.begin(), contagem.end(),
                                               [](auto p, auto q) { return p.second < q.second; })->first;
            for (int y = s.y; y < s.y + s.altura; ++y)
                for (int x = s.x; x < s.x + s.largura; ++x) {
                    if (a.grade.detalhe(x, y) != mundo::Detalhe::Caixa || a.grade.chao(x, y) - nivel < 8) continue;
                    bool degrau = false;  // topo de pilha: precisa de uma caixa de 1 bloco colada
                    for (const glm::ivec2 d : {glm::ivec2(1, 0), glm::ivec2(-1, 0), glm::ivec2(0, 1), glm::ivec2(0, -1)})
                        degrau |= a.grade.detalhe(x + d.x, y + d.y) == mundo::Detalhe::Caixa &&
                                  a.grade.chao(x + d.x, y + d.y) == a.grade.chao(x, y) - 4;
                    INFO("pilha em ", x, ",", y, " chão ", a.grade.chao(x, y), " nível ", nivel);
                    REQUIRE(degrau);
                }
        }
    });
}
