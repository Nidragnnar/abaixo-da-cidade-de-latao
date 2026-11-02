#include "plataforma/Entrada.h"

namespace plataforma {

void Entrada::capturar(bool sim) {
    capturado_ = sim;
    SDL_SetRelativeMouseMode(sim ? SDL_TRUE : SDL_FALSE);
    giroAcumulado_ = 0.0f;  // na volta, a câmera não pula
}

void Entrada::processarEvento(const SDL_Event& e) {
    switch (e.type) {
        case SDL_QUIT: pediuSair_ = true; break;
        case SDL_MOUSEBUTTONDOWN:
            if (!capturado_ && e.button.button == SDL_BUTTON_LEFT) capturar(true);
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_ESCAPE && capturado_) capturar(false);
            break;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST && capturado_) capturar(false);
            break;
        case SDL_MOUSEMOTION:
            if (capturado_) giroAcumulado_ += e.motion.xrel * SENSIBILIDADE;
            break;
        default: break;
    }
}

jogo::Comandos Entrada::coletar() {
    jogo::Comandos c;
    const Uint8* teclas = SDL_GetKeyboardState(nullptr);
    if (teclas[SDL_SCANCODE_W]) c.frente += 1.0f;
    if (teclas[SDL_SCANCODE_S]) c.frente -= 1.0f;
    if (teclas[SDL_SCANCODE_D]) c.lado += 1.0f;
    if (teclas[SDL_SCANCODE_A]) c.lado -= 1.0f;
    c.giro = giroAcumulado_;
    giroAcumulado_ = 0.0f;
    return c;
}

}
