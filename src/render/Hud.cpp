#include "render/Hud.h"

#include <algorithm>
#include <format>
#include <string>

#include "render/Texto.h"

namespace render {

namespace {

constexpr glm::vec4 COR_TEXTO{1.0f, 0.95f, 0.85f, 1.0f};
constexpr glm::vec4 COR_LATAO{0.85f, 0.62f, 0.20f, 1.0f};
constexpr glm::vec4 COR_FUNDO_BARRA{0.15f, 0.10f, 0.05f, 1.0f};
constexpr float LARGURA_BARRA = 100.0f, ALTURA_BARRA = 8.0f, MARGEM = 8.0f;
constexpr float LADO_ARMA = 128.0f;  // fica abaixo da mira
constexpr float DURACAO_FLASH = 0.25f, ALFA_FLASH = 0.35f;
constexpr float DURACAO_BALANCO = 0.3f, BALANCO_MAXIMO = 6.0f;
constexpr float TAMANHO_MIRA = 3.5f;  // meio comprimento de cada traço
constexpr glm::vec4 COR_MIRA{0.95f, 0.75f, 0.35f, 0.8f};

void texto(std::vector<VerticeTex>& v, std::string_view s, glm::vec2 p, float escala, glm::vec4 cor) {
    auto t = construirTexto(s, p, escala, cor);
    v.insert(v.end(), t.begin(), t.end());
}

void textoCentralizado(std::vector<VerticeTex>& v, std::string_view s, float y, int telaL, float escala, glm::vec4 cor) {
    texto(v, s, {std::floor((telaL - larguraTexto(s, escala)) / 2), y}, escala, cor);
}

}

ElementosHud construirHud(const EstadoHud& e, int telaL, int telaA) {
    ElementosHud h;
    const float L = static_cast<float>(telaL), A = static_cast<float>(telaA);

    // Barra de vida (latão) em baixo à esquerda
    const glm::vec2 barra{MARGEM, A - MARGEM - ALTURA_BARRA};
    const float fracao = std::clamp(static_cast<float>(e.vida) / std::max(1, e.vidaMaxima), 0.0f, 1.0f);
    adicionarRetangulo(h.cores, barra, barra + glm::vec2(LARGURA_BARRA, ALTURA_BARRA), COR_FUNDO_BARRA);
    adicionarRetangulo(h.cores, barra, barra + glm::vec2(LARGURA_BARRA * fracao, ALTURA_BARRA), COR_LATAO);
    texto(h.texto, std::format("{}/{}", std::max(0, e.vida), e.vidaMaxima), {barra.x + LARGURA_BARRA + 6, barra.y}, 1,
          COR_TEXTO);

    // Mira no centro (com o olhar vertical, mostra para onde vai o tiro)
    const glm::vec2 centro{std::floor(L / 2), std::floor(A / 2)};
    adicionarRetangulo(h.cores, centro - glm::vec2(TAMANHO_MIRA, 0.5f), centro + glm::vec2(TAMANHO_MIRA, 0.5f), COR_MIRA);
    adicionarRetangulo(h.cores, centro - glm::vec2(0.5f, TAMANHO_MIRA), centro + glm::vec2(0.5f, TAMANHO_MIRA), COR_MIRA);

    // Munição do revólver em baixo à direita
    if (e.arma == jogo::ArmaAtiva::Revolver) {
        const std::string municao = e.recarregando ? "RECARREGANDO" : std::format("{} | {}", e.carga, e.reserva);
        texto(h.texto, municao, {L - MARGEM - larguraTexto(municao, 1), barra.y}, 1, COR_TEXTO);
    }

    texto(h.texto, std::format("Andar {}", e.andar), {MARGEM, MARGEM}, 1, COR_TEXTO);
    if (!e.mensagem.empty()) textoCentralizado(h.texto, e.mensagem, 24, telaL, 1, COR_TEXTO);

    // Arma na mão, balançando logo depois de levar dano
    QuadroArma quadro;
    if (e.arma == jogo::ArmaAtiva::ChaveInglesa) quadro = e.animacao > 0 ? ChaveGolpe : ChaveParada;
    else quadro = e.recarregando ? RevolverRecarga : (e.animacao > 0 ? RevolverTiro : RevolverParado);
    const float balanco =
        e.tempoDesdeDano < DURACAO_BALANCO ? BALANCO_MAXIMO * (1.0f - e.tempoDesdeDano / DURACAO_BALANCO) : 0.0f;
    const glm::vec2 arma{std::floor((L - LADO_ARMA) / 2), A - LADO_ARMA + balanco};
    adicionarQuadro(h.arma, arma, arma + LADO_ARMA, static_cast<float>(quadro), glm::vec4(1.0f));

    // Flash vermelho por cima de tudo
    if (e.tempoDesdeDano < DURACAO_FLASH)
        adicionarRetangulo(h.cores, {0, 0}, {L, A},
                           {0.8f, 0.0f, 0.0f, ALFA_FLASH * (1.0f - e.tempoDesdeDano / DURACAO_FLASH)});
    return h;
}

ElementosHud construirTelaFim(const DadosFim& d, int telaL, int telaA) {
    ElementosHud h;
    adicionarRetangulo(h.cores, {0, 0}, {static_cast<float>(telaL), static_cast<float>(telaA)}, {0, 0, 0, 0.8f});
    float y = 60;
    textoCentralizado(h.texto, "Você morreu", y, telaL, 2, {0.9f, 0.25f, 0.2f, 1.0f});
    y += 32;
    textoCentralizado(h.texto, d.causa, y, telaL, 1, COR_TEXTO);
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
    textoCentralizado(h.texto, "Enter: nova partida", y, telaL, 1, COR_TEXTO);
    return h;
}

}
