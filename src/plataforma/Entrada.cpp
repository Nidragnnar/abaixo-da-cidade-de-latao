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
    rapidoPendente_ = 0;
}

void Entrada::abrirInventario(bool sim) {
    inventarioAberto_ = sim;
    cliques_ = {};
    capturar(!sim);  // aberto: mouse livre para clicar; fechado: volta ao jogo
}

CliqueInventario Entrada::coletarCliques() {
    const CliqueInventario c = cliques_;
    cliques_ = {};
    return c;
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
            if (inventarioAberto_) {
                if (e.button.button == SDL_BUTTON_LEFT) cliques_.pressionou = true;
                if (e.button.button == SDL_BUTTON_RIGHT) cliques_.direito = true;
                posicaoMouse_ = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
            } else if (!capturado_ && e.button.button == SDL_BUTTON_LEFT) {
                capturar(true);
                ignorarBotao_ = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT) {
                ignorarBotao_ = false;
                if (inventarioAberto_) {
                    cliques_.soltou = true;
                    posicaoMouse_ = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
                }
            }
            break;
        case SDL_MOUSEWHEEL:
            if (capturado_ && e.wheel.y != 0) trocarPendente_ = true;
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                if (inventarioAberto_) abrirInventario(false);
                else if (capturado_) capturar(false);
            }
            // Teclas de ação contam uma vez por aperto (segurar não repete)
            if (e.key.repeat) break;
            switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_E: usarPendente_ = capturado_; break;
                case SDL_SCANCODE_Q: trocarPendente_ = capturado_; break;
                case SDL_SCANCODE_R: recarregarPendente_ = capturado_; break;
                case SDL_SCANCODE_1: rapidoPendente_ = capturado_ ? 1 : 0; break;
                case SDL_SCANCODE_2: rapidoPendente_ = capturado_ ? 2 : 0; break;
                case SDL_SCANCODE_3: rapidoPendente_ = capturado_ ? 3 : 0; break;
                case SDL_SCANCODE_4: rapidoPendente_ = capturado_ ? 4 : 0; break;
                case SDL_SCANCODE_I:
                    if (capturado_ || inventarioAberto_) abrirInventario(!inventarioAberto_);
                    break;
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
            } else {
                posicaoMouse_ = {static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
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
    c.usarRapido = rapidoPendente_;
    giroAcumulado_ = inclinarAcumulado_ = 0.0f;
    usarPendente_ = trocarPendente_ = recarregarPendente_ = false;
    rapidoPendente_ = 0;
    return c;
}

}
