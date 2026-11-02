#pragma once
#include <SDL.h>

#include "jogo/Comandos.h"

namespace plataforma {

// Traduz teclado e mouse em jogo::Comandos e cuida da captura do mouse
class Entrada {
public:
    explicit Entrada(SDL_Window* janela) : janela_(janela) {}

    void processarEvento(const SDL_Event& e);
    // Lê WASD e devolve o giro acumulado desde a última chamada (zerando-o)
    jogo::Comandos coletar();
    // Joga fora o giro e o E acumulados (usado enquanto o jogo está pausado)
    void descartarGiro() {
        giroAcumulado_ = 0.0f;
        usarPendente_ = false;
    }
    // Tab alterna o automapa
    bool mapaVisivel() const { return mapaVisivel_; }

    bool capturado() const { return capturado_; }
    bool pediuSair() const { return pediuSair_; }

private:
    void capturar(bool sim);

    static constexpr float SENSIBILIDADE = 0.0025f;  // radianos por pixel

    SDL_Window* janela_;
    float giroAcumulado_ = 0.0f;
    bool capturado_ = false;
    bool pediuSair_ = false;
    bool usarPendente_ = false;  // E apertado desde o último coletar()
    bool mapaVisivel_ = false;
};

}
