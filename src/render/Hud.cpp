#include "render/Hud.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

#include "render/Texto.h"

namespace render {

namespace {

constexpr glm::vec4 COR_TEXTO{1.0f, 0.95f, 0.85f, 1.0f};
constexpr glm::vec4 COR_LATAO{0.85f, 0.62f, 0.20f, 1.0f};
constexpr glm::vec4 COR_LATAO_ESCURO{0.45f, 0.30f, 0.10f, 1.0f};
constexpr glm::vec4 COR_FUNDO_BARRA{0.15f, 0.10f, 0.05f, 1.0f};
constexpr glm::vec4 COR_XP{0.45f, 0.75f, 0.95f, 1.0f};
constexpr glm::vec4 COR_RAPIDO{0.10f, 0.08f, 0.06f, 0.8f};
constexpr float LARGURA_BARRA = 100.0f, ALTURA_BARRA = 8.0f, ALTURA_XP = 3.0f, MARGEM = 8.0f;
constexpr float LADO_ARMA = 128.0f;  // fica abaixo da mira
constexpr float DURACAO_FLASH = 0.25f, ALFA_FLASH = 0.35f;
constexpr float DURACAO_BALANCO = 0.3f, BALANCO_MAXIMO = 6.0f;
constexpr float TAMANHO_MIRA = 3.5f;  // meio comprimento de cada traço
constexpr glm::vec4 COR_MIRA{0.95f, 0.75f, 0.35f, 0.8f};
constexpr float LADO_RAPIDO = 20.0f, ICONE_RAPIDO = 16.0f, PASSO_RAPIDO = 22.0f;
constexpr float LARGURA_BARRA_CHEFE = 200.0f, ALTURA_BARRA_CHEFE = 6.0f;
constexpr glm::vec4 COR_VIDA_CHEFE{0.75f, 0.15f, 0.10f, 1.0f};

void texto(std::vector<VerticeTex>& v, std::string_view s, glm::vec2 p, float escala, glm::vec4 cor) {
    auto t = construirTexto(s, p, escala, cor);
    v.insert(v.end(), t.begin(), t.end());
}

void textoCentralizado(std::vector<VerticeTex>& v, std::string_view s, float y, int telaL, float escala, glm::vec4 cor) {
    texto(v, s, {std::floor((telaL - larguraTexto(s, escala)) / 2), y}, escala, cor);
}

const char* nomeMunicao(jogo::TipoMunicao t) {
    switch (t) {
        case jogo::TipoMunicao::Cartuchos: return "cartuchos";
        case jogo::TipoMunicao::Celulas: return "células";
        default: return "balas";
    }
}

}

int quadroArma(jogo::TipoItem arma, bool ataque, bool recarregando) {
    switch (arma) {
        case jogo::TipoItem::Sabre: return ataque ? 6 : 5;
        case jogo::TipoItem::Martelo: return ataque ? 8 : 7;
        case jogo::TipoItem::Revolver: return recarregando ? 4 : ataque ? 3 : 2;
        case jogo::TipoItem::Bacamarte: return recarregando ? 11 : ataque ? 10 : 9;
        case jogo::TipoItem::Tesla: return ataque ? 13 : 12;
        default: return ataque ? 1 : 0;  // chave inglesa
    }
}

std::string textoMunicao(const EstadoHud& e) {
    if (!e.armaDeFogoAtiva) return {};
    if (e.recarregando) return "RECARREGANDO";
    if (e.semRecarga) return std::format("| {} {}", e.reserva, nomeMunicao(e.municao));
    return std::format("{} | {} {}", e.carga, e.reserva, nomeMunicao(e.municao));
}

ElementosHud construirHud(const EstadoHud& e, int telaL, int telaA) {
    ElementosHud h;
    const float L = static_cast<float>(telaL), A = static_cast<float>(telaA);

    // Manômetro de vida: moldura de latão, fundo e preenchimento (retângulos 0, 1 e 2)
    const glm::vec2 barra{MARGEM + 2, A - MARGEM - 2 - ALTURA_BARRA};
    const float fracaoVida = std::clamp(static_cast<float>(e.vida) / std::max(1, e.vidaMaxima), 0.0f, 1.0f);
    adicionarRetangulo(h.cores, barra - 2.0f, barra + glm::vec2(LARGURA_BARRA, ALTURA_BARRA) + 2.0f, COR_LATAO_ESCURO);
    adicionarRetangulo(h.cores, barra, barra + glm::vec2(LARGURA_BARRA, ALTURA_BARRA), COR_FUNDO_BARRA);
    adicionarRetangulo(h.cores, barra, barra + glm::vec2(LARGURA_BARRA * fracaoVida, ALTURA_BARRA), COR_LATAO);

    // Barra fina de XP logo acima (retângulos 3 e 4)
    const glm::vec2 xp{barra.x, barra.y - 6 - ALTURA_XP};
    const float fracaoXp = std::clamp(static_cast<float>(e.xp) / std::max(1, e.xpProximo), 0.0f, 1.0f);
    adicionarRetangulo(h.cores, xp, xp + glm::vec2(LARGURA_BARRA, ALTURA_XP), COR_FUNDO_BARRA);
    adicionarRetangulo(h.cores, xp, xp + glm::vec2(LARGURA_BARRA * fracaoXp, ALTURA_XP), COR_XP);
    texto(h.texto, std::format("Nv {}   {}/{}", e.nivel, std::max(0, e.vida), e.vidaMaxima), {barra.x, xp.y - 11}, 1,
          COR_TEXTO);

    // Mira no centro (retângulos 5 e 6)
    const glm::vec2 centro{std::floor(L / 2), std::floor(A / 2)};
    adicionarRetangulo(h.cores, centro - glm::vec2(TAMANHO_MIRA, 0.5f), centro + glm::vec2(TAMANHO_MIRA, 0.5f), COR_MIRA);
    adicionarRetangulo(h.cores, centro - glm::vec2(0.5f, TAMANHO_MIRA), centro + glm::vec2(0.5f, TAMANHO_MIRA), COR_MIRA);

    // Itens rápidos 1–4 em baixo à direita, acima da munição
    const glm::vec2 rapidos{L - MARGEM - 4 * PASSO_RAPIDO + (PASSO_RAPIDO - LADO_RAPIDO), A - MARGEM - 12 - LADO_RAPIDO};
    for (int k = 0; k < 4; ++k) {
        const glm::vec2 p = rapidos + glm::vec2(k * PASSO_RAPIDO, 0);
        adicionarRetangulo(h.cores, p, p + LADO_RAPIDO, COR_RAPIDO);
        texto(h.texto, std::to_string(k + 1), p + glm::vec2(1, -9), 1, COR_LATAO);
        const RapidoHud& r = e.rapidos[static_cast<size_t>(k)];
        if (r.icone < 0) continue;
        const glm::vec2 icone = p + (LADO_RAPIDO - ICONE_RAPIDO) / 2;
        adicionarQuadro(h.icones, icone, icone + ICONE_RAPIDO, static_cast<float>(r.icone), glm::vec4(1.0f));
        if (r.quantidade > 1) texto(h.texto, std::to_string(r.quantidade), p + glm::vec2(LADO_RAPIDO - 7, LADO_RAPIDO - 7), 1, COR_TEXTO);
    }

    const std::string municao = textoMunicao(e);
    if (!municao.empty()) texto(h.texto, municao, {L - MARGEM - larguraTexto(municao, 1), barra.y}, 1, COR_TEXTO);

    texto(h.texto, std::format("Andar {}", e.andar), {MARGEM, MARGEM}, 1, COR_TEXTO);
    const std::string engrenagens = std::format("Engrenagens {}", e.engrenagens);
    texto(h.texto, engrenagens, {L - MARGEM - larguraTexto(engrenagens, 1), MARGEM}, 1, COR_LATAO);
    // Chefe: nome no alto e barra larga logo abaixo (a mensagem desce para não cobrir)
    float yMensagem = 24;
    if (!e.nomeChefe.empty()) {
        textoCentralizado(h.texto, e.nomeChefe, MARGEM, telaL, 1, COR_TEXTO);
        const glm::vec2 b{std::floor((L - LARGURA_BARRA_CHEFE) / 2), MARGEM + 12};
        const float fracao = e.vidaMaximaChefe > 0 ? std::clamp(static_cast<float>(e.vidaChefe) / e.vidaMaximaChefe, 0.0f, 1.0f) : 0.0f;
        adicionarRetangulo(h.cores, b - 2.0f, b + glm::vec2(LARGURA_BARRA_CHEFE, ALTURA_BARRA_CHEFE) + 2.0f, COR_LATAO_ESCURO);
        adicionarRetangulo(h.cores, b, b + glm::vec2(LARGURA_BARRA_CHEFE, ALTURA_BARRA_CHEFE), COR_FUNDO_BARRA);
        adicionarRetangulo(h.cores, b, b + glm::vec2(LARGURA_BARRA_CHEFE * fracao, ALTURA_BARRA_CHEFE), COR_VIDA_CHEFE);
        yMensagem = MARGEM + 12 + ALTURA_BARRA_CHEFE + 8;
    }
    if (!e.mensagem.empty()) textoCentralizado(h.texto, e.mensagem, yMensagem, telaL, 1, COR_TEXTO);

    // Arma na mão, balançando logo depois de levar dano
    const int quadro = quadroArma(e.arma, e.animacao > 0, e.armaDeFogoAtiva && e.recarregando);
    const float balanco =
        e.tempoDesdeDano < DURACAO_BALANCO ? BALANCO_MAXIMO * (1.0f - e.tempoDesdeDano / DURACAO_BALANCO) : 0.0f;
    const glm::vec2 arma{std::floor((L - LADO_ARMA) / 2), A - LADO_ARMA + balanco};
    adicionarQuadro(h.arma, arma, arma + LADO_ARMA, static_cast<float>(quadro), glm::vec4(1.0f));

    // Flash vermelho por cima de tudo
    if (e.tempoDesdeDano < DURACAO_FLASH)
        adicionarRetangulo(h.cores, {0, 0}, {L, A}, {0.8f, 0.0f, 0.0f, ALFA_FLASH * (1.0f - e.tempoDesdeDano / DURACAO_FLASH)});
    return h;
}

ElementosHud construirTelaFim(const DadosFim& d, int telaL, int telaA) {
    ElementosHud h;
    adicionarRetangulo(h.cores, {0, 0}, {static_cast<float>(telaL), static_cast<float>(telaA)}, {0, 0, 0, 0.8f});
    float y = 60;
    if (d.vitoria) textoCentralizado(h.texto, "Vitória!", y, telaL, 2, COR_LATAO);
    else textoCentralizado(h.texto, "Você morreu", y, telaL, 2, {0.9f, 0.25f, 0.2f, 1.0f});
    y += 32;
    textoCentralizado(h.texto, d.causa, y, telaL, 1, COR_TEXTO);
    if (d.vitoria) {
        y += 14;
        textoCentralizado(h.texto, std::format("Tempo {}:{:02}", d.segundos / 60, d.segundos % 60), y, telaL, 1, COR_TEXTO);
    }
    y += 24;
    textoCentralizado(h.texto, std::format("Andar {}   Nível {}   Inimigos {}", d.andar, d.nivel, d.inimigos), y, telaL,
                      1, COR_TEXTO);
    y += 14;
    textoCentralizado(h.texto, std::format("Engrenagens {}   Pontuação {}", d.engrenagens, d.pontuacao), y, telaL, 1,
                      COR_TEXTO);
    y += 14;
    textoCentralizado(h.texto, std::format("Semente {}", d.semente), y, telaL, 1, COR_TEXTO);
    y += 24;
    if (d.posicaoPlacar > 0)
        textoCentralizado(h.texto, std::format("{}º lugar no placar!", d.posicaoPlacar), y, telaL, 1, COR_LATAO);
    y += 24;
    textoCentralizado(h.texto, "Enter: nova partida   Esc: menu", y, telaL, 1, COR_TEXTO);
    return h;
}

}
