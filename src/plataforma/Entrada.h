#pragma once
#include <SDL.h>

#include "jogo/Comandos.h"

namespace plataforma {

// Traduz teclado e mouse em jogo::Comandos e cuida da captura do mouse
class Entrada {
public:
    explicit Entrada(SDL_Window* janela) : janela_(janela) {}

    void processarEvento(const SDL_Event& e);
    // Lê WASD e o botão do mouse; devolve giro e eventos (E, Q/roda, R) acumulados desde a última chamada, zerando-os
    jogo::Comandos coletar();
    // Joga fora o giro e os eventos acumulados (usado enquanto o jogo está pausado)
    void descartarGiro();
    // Tab alterna o automapa
    bool mapaVisivel() const { return mapaVisivel_; }
    // Enter apertado desde a última consulta (tela de fim)
    bool pediuNovaPartida();
    void soltarMouse() { capturar(false); }

    bool capturado() const { return capturado_; }
    bool pediuSair() const { return pediuSair_; }

private:
    void capturar(bool sim);

    static constexpr float SENSIBILIDADE = 0.0025f;  // radianos por pixel

    SDL_Window* janela_;
    float giroAcumulado_ = 0.0f;
    float inclinarAcumulado_ = 0.0f;  // mouse vertical
    bool capturado_ = false;
    bool pediuSair_ = false;
    bool usarPendente_ = false;        // E apertado desde o último coletar()
    bool trocarPendente_ = false;      // Q ou roda
    bool recarregarPendente_ = false;  // R
    bool novaPartidaPendente_ = false;  // Enter
    bool ignorarBotao_ = false;        // o clique que capturou o mouse não ataca
    bool mapaVisivel_ = false;
};

}
