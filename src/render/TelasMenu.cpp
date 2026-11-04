#include "render/TelasMenu.h"

#include <algorithm>
#include <cmath>
#include <format>

#include "render/Texto.h"

namespace render {

namespace {

constexpr int LARGURA_ITEM = 264, ALTURA_ITEM = 12, ENTRE_ITENS = 14, Y_TITULO = 48, Y_ITENS = 96;
constexpr glm::vec4 COR_PAINEL{0.06f, 0.05f, 0.04f, 0.85f};
constexpr glm::vec4 COR_DESTAQUE{0.30f, 0.26f, 0.20f, 1.0f};
constexpr glm::vec4 COR_TEXTO{1.0f, 0.95f, 0.85f, 1.0f};
constexpr glm::vec4 COR_LATAO{0.85f, 0.62f, 0.20f, 1.0f};

void texto(std::vector<VerticeTex>& v, std::string_view s, glm::vec2 p, float escala, glm::vec4 cor) {
    const auto t = construirTexto(s, p, escala, cor);
    v.insert(v.end(), t.begin(), t.end());
}

void centralizado(std::vector<VerticeTex>& v, std::string_view s, float y, int telaL, float escala, glm::vec4 cor) {
    texto(v, s, {std::floor((telaL - larguraTexto(s, escala)) / 2.0f), y}, escala, cor);
}

void retangulo(std::vector<VerticeCor>& v, const Retangulo& r, glm::vec4 cor) {
    adicionarRetangulo(v, {static_cast<float>(r.x), static_cast<float>(r.y)},
                       {static_cast<float>(r.x + r.largura), static_cast<float>(r.y + r.altura)}, cor);
}

// Corta o fim (sem partir caractere UTF-8) até caber, com "..."
std::string cortar(std::string s, float largura) {
    if (larguraTexto(s, 1) <= largura) return s;
    const auto continuacao = [](char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; };
    while (!s.empty() && larguraTexto(s + "...", 1) > largura) {
        // Tira um caractere inteiro: os bytes de continuação do fim e o byte que os começa
        while (!s.empty() && continuacao(s.back())) s.pop_back();
        if (!s.empty()) s.pop_back();
    }
    return s + "...";
}

}

std::vector<Retangulo> layoutMenu(int quantidade, int telaL, int /*telaA*/) {
    std::vector<Retangulo> r;
    for (int i = 0; i < quantidade; ++i)
        r.push_back({(telaL - LARGURA_ITEM) / 2, Y_ITENS + i * ENTRE_ITENS - 2, LARGURA_ITEM, ALTURA_ITEM});
    return r;
}

std::optional<int> itemSob(const std::vector<Retangulo>& itens, glm::vec2 ponto) {
    for (size_t i = 0; i < itens.size(); ++i)
        if (contem(itens[i], ponto)) return static_cast<int>(i);
    return std::nullopt;
}

ElementosHud construirMenu(const VistaMenu& v, int telaL, int telaA) {
    ElementosHud e;
    const auto itens = layoutMenu(static_cast<int>(v.itens.size()), telaL, telaA);
    const int largura = std::max(LARGURA_ITEM + 40, static_cast<int>(larguraTexto(v.titulo, 2)) + 24);
    const int baixo = itens.empty() ? Y_ITENS : itens.back().y + itens.back().altura + 14;
    retangulo(e.cores, {(telaL - largura) / 2, Y_TITULO - 14, largura, baixo - (Y_TITULO - 14)}, COR_PAINEL);
    centralizado(e.texto, v.titulo, static_cast<float>(Y_TITULO), telaL, 2, COR_LATAO);
    for (size_t i = 0; i < v.itens.size(); ++i) {
        const bool selecionado = static_cast<int>(i) == v.selecionado;
        centralizado(e.texto, v.itens[i], static_cast<float>(itens[i].y + 2), telaL, 1,
                     selecionado ? COR_LATAO : COR_TEXTO);
        if (selecionado) texto(e.texto, ">", {static_cast<float>(itens[i].x + 4), static_cast<float>(itens[i].y + 2)}, 1, COR_LATAO);
    }
    // Destaque por último (o teste e o laço contam com isso); desenhado antes do texto, que vem em outra lista
    if (v.selecionado >= 0 && v.selecionado < static_cast<int>(itens.size()))
        retangulo(e.cores, itens[static_cast<size_t>(v.selecionado)], COR_DESTAQUE);
    return e;
}

std::vector<Retangulo> layoutPlacar(int telaL, int telaA) {
    return {{(telaL - LARGURA_ITEM) / 2, telaA - 30, LARGURA_ITEM, ALTURA_ITEM}};
}

ElementosHud construirPlacar(const std::vector<jogo::RegistroPlacar>& registros, int telaL, int telaA) {
    ElementosHud e;
    retangulo(e.cores, {0, 0, telaL, telaA}, COR_PAINEL);
    centralizado(e.texto, "Placar", 14, telaL, 2, COR_LATAO);
    const float maximo = static_cast<float>(telaL - 32);
    if (registros.empty()) centralizado(e.texto, "Nenhuma partida ainda", 110, telaL, 1, COR_TEXTO);
    for (size_t i = 0; i < registros.size() && i < jogo::TAMANHO_PLACAR; ++i) {
        const jogo::RegistroPlacar& r = registros[i];
        const std::string linha = cortar(
            std::format("{}. {}  andar {}  nível {}  {}", i + 1, r.pontuacao, r.andar, r.nivel, r.causa), maximo);
        texto(e.texto, linha, {16, 44 + static_cast<float>(i) * 18}, 1, i == 0 ? COR_LATAO : COR_TEXTO);
    }
    const Retangulo voltar = layoutPlacar(telaL, telaA)[0];
    retangulo(e.cores, voltar, COR_DESTAQUE);
    centralizado(e.texto, "Voltar", static_cast<float>(voltar.y + 2), telaL, 1, COR_LATAO);
    return e;
}

}
