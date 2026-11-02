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

bool Entrada::pediuPausa() {
    const bool pediu = pausaPendente_;
    pausaPendente_ = false;
    return pediu;
}

void Entrada::modoMenu(bool sim) {
    modoMenu_ = sim;
    menu_ = {};
    pausaPendente_ = false;
    if (sim) {
        inventarioAberto_ = false;
        capturar(false);
        descartarGiro();
    }
}

ui::EntradaMenu Entrada::coletarMenu() {
    ui::EntradaMenu m = std::move(menu_);
    menu_ = {};
    return m;
}

void Entrada::entrarNoJogo() {
    modoMenu(false);
    capturar(true);
    // O botão ainda pode estar apertado do clique que confirmou: só ataca depois de soltar
    ignorarBotao_ = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) != 0;
}

void Entrada::teclaNoMenu(const SDL_KeyboardEvent& k) {
    // Segurar a tecla repete a navegação
    switch (k.keysym.scancode) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W: menu_.mover = -1; break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S: menu_.mover = 1; break;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A: menu_.ajustar = -1; break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D: menu_.ajustar = 1; break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            if (!k.repeat) menu_.confirmar = true;
            break;
        case SDL_SCANCODE_ESCAPE:
            if (!k.repeat) menu_.voltar = true;
            break;
        case SDL_SCANCODE_BACKSPACE: menu_.apagar = true; break;
        default: break;
    }
}

void Entrada::processarEvento(const SDL_Event& e) {
    switch (e.type) {
        case SDL_QUIT: pediuSair_ = true; break;
        case SDL_MOUSEBUTTONDOWN:
            posicaoMouse_ = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
            if (modoMenu_) {
                if (e.button.button == SDL_BUTTON_LEFT) menu_.clique = true;
                if (e.button.button == SDL_BUTTON_RIGHT) menu_.cliqueDireito = true;
            } else if (inventarioAberto_) {
                if (e.button.button == SDL_BUTTON_LEFT) cliques_.pressionou = true;
                if (e.button.button == SDL_BUTTON_RIGHT) cliques_.direito = true;
            } else if (!capturado_ && e.button.button == SDL_BUTTON_LEFT) {
                capturar(true);
                ignorarBotao_ = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT) {
                ignorarBotao_ = false;
                if (inventarioAberto_ && !modoMenu_) {
                    cliques_.soltou = true;
                    posicaoMouse_ = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
                }
            }
            break;
        case SDL_MOUSEWHEEL:
            if (capturado_ && e.wheel.y != 0) trocarPendente_ = true;
            break;
        case SDL_TEXTINPUT:
            if (modoMenu_) menu_.texto += e.text.text;
            break;
        case SDL_KEYDOWN:
            if (modoMenu_) {
                teclaNoMenu(e.key);
                break;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE && !e.key.repeat) {
                if (inventarioAberto_) abrirInventario(false);
                else pausaPendente_ = true;
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
                default: break;
            }
            break;
        case SDL_WINDOWEVENT:
            // Perder o foco no jogo pausa e solta o mouse (seção 11 da spec)
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                if (capturado_) capturar(false);
                if (!modoMenu_) pausaPendente_ = true;
            }
            break;
        case SDL_MOUSEMOTION:
            if (capturado_) {
                giroAcumulado_ += e.motion.xrel * sensibilidade_;
                inclinarAcumulado_ -= e.motion.yrel * sensibilidade_;  // mouse para cima = olhar para cima
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
