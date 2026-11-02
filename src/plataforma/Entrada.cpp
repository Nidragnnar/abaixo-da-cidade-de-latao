#include "plataforma/Entrada.h"

namespace plataforma {

void Entrada::capturar(bool sim) {
    capturado_ = sim;
    SDL_SetRelativeMouseMode(sim ? SDL_TRUE : SDL_FALSE);
    giroAcumulado_ = inclinarAcumulado_ = 0.0f;  // na volta, a câmera não pula
}

void Entrada::descartarGiro() {
    giroAcumulado_ = inclinarAcumulado_ = 0.0f;
    usarPendente_ = trocarPendente_ = recarregarPendente_ = false;
}

bool Entrada::pediuNovaPartida() {
    const bool pediu = novaPartidaPendente_;
    novaPartidaPendente_ = false;
    return pediu;
}

void Entrada::processarEvento(const SDL_Event& e) {
    switch (e.type) {
        case SDL_QUIT: pediuSair_ = true; break;
        case SDL_MOUSEBUTTONDOWN:
            if (!capturado_ && e.button.button == SDL_BUTTON_LEFT) {
                capturar(true);
                ignorarBotao_ = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT) ignorarBotao_ = false;
            break;
        case SDL_MOUSEWHEEL:
            if (capturado_ && e.wheel.y != 0) trocarPendente_ = true;
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_ESCAPE && capturado_) capturar(false);
            // Teclas de ação contam uma vez por aperto (segurar não repete)
            if (e.key.repeat) break;
            switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_E: usarPendente_ = capturado_; break;
                case SDL_SCANCODE_Q: trocarPendente_ = capturado_; break;
                case SDL_SCANCODE_R: recarregarPendente_ = capturado_; break;
                case SDL_SCANCODE_TAB: mapaVisivel_ = !mapaVisivel_; break;
                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_KP_ENTER: novaPartidaPendente_ = true; break;
                default: break;
            }
            break;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST && capturado_) capturar(false);
            break;
        case SDL_MOUSEMOTION:
            if (capturado_) {
                giroAcumulado_ += e.motion.xrel * SENSIBILIDADE;
                inclinarAcumulado_ -= e.motion.yrel * SENSIBILIDADE;  // mouse para cima = olhar para cima
            }
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
    c.atacar = capturado_ && !ignorarBotao_ && (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK);
    c.giro = giroAcumulado_;
    c.inclinar = inclinarAcumulado_;
    c.usar = usarPendente_;
    c.trocarArma = trocarPendente_;
    c.recarregar = recarregarPendente_;
    giroAcumulado_ = inclinarAcumulado_ = 0.0f;
    usarPendente_ = trocarPendente_ = recarregarPendente_ = false;
    return c;
}

}
