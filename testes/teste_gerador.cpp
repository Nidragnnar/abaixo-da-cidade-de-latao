#include <doctest/doctest.h>

#include <glm/geometric.hpp>

#include "mundo/Consultas.h"
#include "mundo/Gerador.h"

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
