#include "render/TelaInventario.h"

#include <algorithm>
#include <cmath>
#include <format>

#include "jogo/Armas.h"
#include "render/Icones.h"
#include "render/Texto.h"

namespace render {

namespace {

constexpr int LADO_ESPACO = 36, FOLGA = 4, LADO_ICONE = 32, ICONE_ARRASTADO = 16;
constexpr float ENTRE_LINHAS = 9;  // detalhes do item (cabem 6 linhas abaixo da grade)
constexpr glm::vec4 COR_PAINEL{0.06f, 0.05f, 0.04f, 0.88f};
constexpr glm::vec4 COR_FUNDO_ESPACO{0.14f, 0.12f, 0.10f, 1.0f};
constexpr glm::vec4 COR_SOB_MOUSE{0.30f, 0.26f, 0.20f, 1.0f};
constexpr glm::vec4 COR_TEXTO{1.0f, 0.95f, 0.85f, 1.0f};
constexpr glm::vec4 COR_LATAO{0.85f, 0.62f, 0.20f, 1.0f};
constexpr glm::vec4 COR_COMUM{0.55f, 0.52f, 0.48f, 1.0f};
constexpr glm::vec4 COR_MODIFICADA{0.45f, 0.70f, 1.0f, 1.0f};
constexpr glm::vec4 COR_OBRA_PRIMA{1.0f, 0.80f, 0.30f, 1.0f};

glm::vec4 corDaQualidade(jogo::Qualidade q) {
    switch (q) {
        case jogo::Qualidade::Modificada: return COR_MODIFICADA;
        case jogo::Qualidade::ObraPrima: return COR_OBRA_PRIMA;
        default: return COR_COMUM;
    }
}

void texto(std::vector<VerticeTex>& v, std::string_view s, glm::vec2 p, glm::vec4 cor) {
    auto t = construirTexto(s, p, 1, cor);
    v.insert(v.end(), t.begin(), t.end());
}

// "0,50"
std::string comVirgula(float valor) {
    std::string s = std::format("{:.2f}", valor);
    for (char& c : s)
        if (c == '.') c = ',';
    return s;
}

}

LayoutInventario layoutInventario(int telaL, int telaA) {
    LayoutInventario l{};
    const int grade = 4 * LADO_ESPACO + 3 * FOLGA;
    const int larguraPainel = 320, alturaPainel = 242;
    l.painel = {(telaL - larguraPainel) / 2, (telaA - alturaPainel) / 2, larguraPainel, alturaPainel};
    const int x0 = l.painel.x + 12, y0 = l.painel.y + 26;
    for (int i = 0; i < jogo::ESPACOS_INVENTARIO; ++i)
        l.espacos[static_cast<size_t>(i)] = {x0 + (i % 4) * (LADO_ESPACO + FOLGA), y0 + (i / 4) * (LADO_ESPACO + FOLGA),
                                             LADO_ESPACO, LADO_ESPACO};
    const int xDireita = x0 + grade + 20;
    for (int k = 0; k < 3; ++k)
        l.equipados[static_cast<size_t>(k)] = {xDireita, y0 + k * (LADO_ESPACO + 14), LADO_ESPACO, LADO_ESPACO};
    // Embaixo da armadura, na coluna da direita, com as dicas logo abaixo
    l.largar = {xDireita, y0 + 3 * (LADO_ESPACO + 14) + 2, l.painel.x + larguraPainel - 8 - xDireita, 24};
    return l;
}

bool sobLargar(const LayoutInventario& l, glm::vec2 ponto) { return contem(l.largar, ponto) || !contem(l.painel, ponto); }

std::optional<int> espacoSob(const LayoutInventario& l, glm::vec2 ponto) {
    for (int i = 0; i < jogo::ESPACOS_INVENTARIO; ++i)
        if (contem(l.espacos[static_cast<size_t>(i)], ponto)) return i;
    for (int k = 0; k < 3; ++k)
        if (contem(l.equipados[static_cast<size_t>(k)], ponto)) return jogo::ESPACOS_INVENTARIO + k;
    return std::nullopt;
}

std::string descricaoItem(const jogo::Item& i) {
    switch (jogo::classe(i.tipo)) {
        case jogo::ClasseItem::Consumivel: return "Cura 15 de vida";
        case jogo::ClasseItem::Armadura: {
            std::string s = std::format("Defesa {}", jogo::dadosArmadura(i.tipo).defesa + jogo::somaBonus(i, jogo::TipoBonus::Defesa));
            if (const int vida = jogo::somaBonus(i, jogo::TipoBonus::VidaMaxima)) s += std::format("   Vida +{}", vida);
            return s;
        }
        default: break;
    }
    const jogo::DadosArma& d = jogo::dadosArma(i.tipo);
    const int bonus = jogo::bonusPorChumbo(d, jogo::somaBonus(i, jogo::TipoBonus::Dano));  // por chumbo no bacamarte
    const float intervalo = d.intervalo * (1.0f - jogo::somaBonus(i, jogo::TipoBonus::Velocidade) / 100.0f);
    std::string s = d.chumbos > 1 ? std::format("Dano {}–{} ×{}", d.danoMin + bonus, d.danoMax + bonus, d.chumbos)
                                  : std::format("Dano {}–{}", d.danoMin + bonus, d.danoMax + bonus);
    s += std::format("   Intervalo {} s", comVirgula(intervalo));
    if (d.carga > 0) {
        const float recarga = d.recarga * (1.0f - jogo::somaBonus(i, jogo::TipoBonus::Recarga) / 100.0f);
        s += std::format("   Carga {}   Recarga {} s", jogo::capacidade(i), comVirgula(recarga));
    }
    else if (jogo::classe(i.tipo) == jogo::ClasseItem::ArmaDeFogo) s += std::format("   Alcance {}", static_cast<int>(d.alcance));
    return s;
}

ElementosInventario construirInventario(const jogo::Inventario& inv, const LayoutInventario& l,
                                        std::optional<int> sobMouse, std::optional<int> arrastando, glm::vec2 mouse,
                                        std::string_view mensagem, std::string_view titulo) {
    ElementosInventario e;
    const auto p = [](const Retangulo& r) { return glm::vec2(r.x, r.y); };
    const auto fim = [](const Retangulo& r) { return glm::vec2(r.x + r.largura, r.y + r.altura); };
    adicionarRetangulo(e.cores, p(l.painel), fim(l.painel), COR_PAINEL);
    texto(e.texto, titulo, p(l.painel) + glm::vec2(12, 8), COR_LATAO);  // com a tecla atual do inventário

    // Espaço: borda na cor da qualidade, fundo, ícone e quantidade
    auto desenharEspaco = [&](const Retangulo& r, const jogo::Item* item, bool destacado, bool escondeIcone) {
        adicionarRetangulo(e.cores, p(r), fim(r), item ? corDaQualidade(item->qualidade) : COR_COMUM);
        adicionarRetangulo(e.cores, p(r) + 1.0f, fim(r) - 1.0f, destacado ? COR_SOB_MOUSE : COR_FUNDO_ESPACO);
        if (!item || escondeIcone) return;
        const glm::vec2 icone = p(r) + (LADO_ESPACO - LADO_ICONE) / 2.0f;
        adicionarQuadro(e.icones, icone, icone + static_cast<float>(LADO_ICONE), static_cast<float>(iconeDoItem(item->tipo)), glm::vec4(1.0f));
        if (item->quantidade > 1) texto(e.texto, std::to_string(item->quantidade), fim(r) - 9.0f, COR_TEXTO);
    };
    for (int i = 0; i < jogo::ESPACOS_INVENTARIO; ++i) {
        const auto& item = inv.espaco(i);
        desenharEspaco(l.espacos[static_cast<size_t>(i)], item ? &*item : nullptr, sobMouse == i, arrastando == i);
        if (i < 4) texto(e.texto, std::to_string(i + 1), p(l.espacos[static_cast<size_t>(i)]) + 1.0f, COR_LATAO);
    }
    const jogo::Item* equipados[3] = {&inv.armaBranca(), &inv.armaDeFogo(), &inv.armadura()};
    const char* rotulos[3] = {"Arma branca", "Arma de fogo", "Armadura"};
    for (int k = 0; k < 3; ++k) {
        const Retangulo& r = l.equipados[static_cast<size_t>(k)];
        desenharEspaco(r, equipados[k], sobMouse == jogo::ESPACOS_INVENTARIO + k, false);
        texto(e.texto, rotulos[k], p(r) + glm::vec2(0, LADO_ESPACO + 2), COR_TEXTO);
    }

    // Detalhes do item sob o mouse, na cor da qualidade
    const jogo::Item* detalhe = nullptr;
    if (sobMouse && *sobMouse < jogo::ESPACOS_INVENTARIO && inv.espaco(*sobMouse)) detalhe = &*inv.espaco(*sobMouse);
    else if (sobMouse && *sobMouse >= jogo::ESPACOS_INVENTARIO) detalhe = equipados[*sobMouse - jogo::ESPACOS_INVENTARIO];
    const float yDetalhe = static_cast<float>(l.espacos[15].y + LADO_ESPACO + 6);
    if (detalhe) {
        const glm::vec4 cor = detalhe->qualidade == jogo::Qualidade::Comum ? COR_TEXTO : corDaQualidade(detalhe->qualidade);
        const float x0 = static_cast<float>(l.espacos[0].x);
        // Nome (quebra entre palavras) e descrição (quebra entre atributos, separados por 3 espaços),
        // sem invadir a coluna da direita
        const float larguraMaxima = static_cast<float>(l.largar.x) - 8 - x0;
        float y = yDetalhe;
        const auto quebrado = [&](const std::string& s, std::string_view separador, glm::vec4 corLinha) {
            std::string linha;
            for (size_t inicio = 0; inicio < s.size();) {
                const size_t corte = s.find(separador, inicio);
                const std::string parte = s.substr(inicio, corte == std::string::npos ? std::string::npos : corte - inicio);
                const std::string tentativa = linha.empty() ? parte : linha + std::string(separador) + parte;
                if (!linha.empty() && larguraTexto(tentativa, 1) > larguraMaxima) {
                    texto(e.texto, linha, {x0, y}, corLinha);
                    y += ENTRE_LINHAS;
                    linha = parte;
                } else {
                    linha = tentativa;
                }
                inicio = corte == std::string::npos ? s.size() : corte + separador.size();
            }
            if (!linha.empty()) {
                texto(e.texto, linha, {x0, y}, corLinha);
                y += ENTRE_LINHAS;
            }
        };
        quebrado(jogo::nomeItem(*detalhe), " ", cor);
        quebrado(descricaoItem(*detalhe), "   ", COR_TEXTO);
    }
    // Caixa de largar (acende com um item arrastado em cima) e as dicas embaixo dela
    const bool soltandoNela = arrastando && *arrastando < jogo::ESPACOS_INVENTARIO && contem(l.largar, mouse);
    adicionarRetangulo(e.cores, p(l.largar), fim(l.largar), arrastando ? COR_LATAO : COR_COMUM);
    adicionarRetangulo(e.cores, p(l.largar) + 1.0f, fim(l.largar) - 1.0f, soltandoNela ? COR_SOB_MOUSE : COR_FUNDO_ESPACO);
    const std::string_view rotulo = "Largar aqui";
    texto(e.texto, rotulo,
          {std::floor(l.largar.x + (l.largar.largura - larguraTexto(rotulo, 1)) / 2.0f), static_cast<float>(l.largar.y + 8)},
          arrastando ? COR_LATAO : COR_TEXTO);
    float yDica = static_cast<float>(l.largar.y + l.largar.altura + 6);
    for (const char* dica : {"Clique: usar", "Direito: largar", "Arraste: trocar"}) {
        texto(e.texto, dica, {static_cast<float>(l.largar.x), yDica}, COR_TEXTO);
        yDica += 10;
    }

    // Mensagem da partida, centralizada acima do painel
    if (!mensagem.empty())
        texto(e.texto, mensagem,
              {std::floor(l.painel.x + (l.painel.largura - larguraTexto(mensagem, 1)) / 2.0f),
               static_cast<float>(std::max(0, l.painel.y - 11))},
              COR_LATAO);

    // O item arrastado acompanha o mouse (por último)
    if (arrastando && *arrastando < jogo::ESPACOS_INVENTARIO && inv.espaco(*arrastando)) {
        const glm::vec2 icone = mouse - ICONE_ARRASTADO / 2.0f;
        adicionarQuadro(e.icones, icone, icone + static_cast<float>(ICONE_ARRASTADO),
                        static_cast<float>(iconeDoItem(inv.espaco(*arrastando)->tipo)), glm::vec4(1.0f));
    }
    return e;
}

}
