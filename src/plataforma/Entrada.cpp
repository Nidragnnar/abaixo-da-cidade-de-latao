#include "plataforma/Entrada.h"

#include <algorithm>
#include <cmath>

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
    pularPendente_ = false;
}

void Entrada::abrirInventario(bool sim) {
    inventarioAberto_ = sim;
    cliques_ = {};
    capturar(!sim);  // aberto: mouse livre para clicar; fechado: volta ao jogo
    // Fechar (I ou Esc) com o botão ainda apertado de um arraste não dispara a arma
    if (!sim) ignorarBotao_ = ataqueApertado();
}

bool Entrada::ataqueApertado() const {
    return jogo::apertada(atalhos_.de(jogo::AcaoJogo::Atacar), SDL_GetKeyboardState(nullptr),
                          SDL_GetMouseState(nullptr, nullptr));
}

void Entrada::acao(jogo::AcaoJogo a) {
    using jogo::AcaoJogo;
    switch (a) {
        case AcaoJogo::Usar: usarPendente_ = capturado_; break;
        case AcaoJogo::TrocarArma: trocarPendente_ = capturado_; break;
        case AcaoJogo::Recarregar: recarregarPendente_ = capturado_; break;
        case AcaoJogo::Item1:
        case AcaoJogo::Item2:
        case AcaoJogo::Item3:
        case AcaoJogo::Item4:
            rapidoPendente_ = capturado_ ? static_cast<int>(a) - static_cast<int>(AcaoJogo::Item1) + 1 : 0;
            break;
        case AcaoJogo::Inventario:
            if (capturado_ || inventarioAberto_) abrirInventario(!inventarioAberto_);
            break;
        case AcaoJogo::Mapa: mapaVisivel_ = !mapaVisivel_; break;
        case AcaoJogo::Pular: pularPendente_ = capturado_; break;
        default: break;  // andar e atacar são lidos segurados, em coletar()
    }
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
    m.tempo = SDL_GetTicks() / 1000.0;
    menu_ = {};
    return m;
}

void Entrada::entrarNoJogo() {
    modoMenu(false);
    capturar(true);
    // O botão (ou a tecla do atacar) ainda pode estar apertado do clique que confirmou: só ataca depois de soltar
    ignorarBotao_ = ataqueApertado();
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
        case SDL_CONTROLLERDEVICEADDED:
            if (!controle_) abrirControle(e.cdevice.which);  // which = índice do aparelho
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (controle_ && e.cdevice.which == idControle_) {  // which = id da instância
                SDL_GameControllerClose(controle_);
                controle_ = nullptr;
                idControle_ = -1;
                antes_ = agora_ = {};
                movimento_ = {};  // o jogador para (sem pausar sozinho)
                for (int i = 0; i < SDL_NumJoysticks() && !controle_; ++i) abrirControle(i);  // outro que esteja ligado
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            ultimoFoiControle_ = false;
            posicaoMouse_ = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
            // O clique que só devolveu o foco à janela (voltando do Alt-Tab) não escolhe nada no menu
            if (modoMenu_ && e.button.timestamp - focoGanhoEm_ < CLIQUE_DO_FOCO_MS) break;
            if (modoMenu_) {
                if (e.button.button == SDL_BUTTON_LEFT) menu_.clique = true;
                if (e.button.button == SDL_BUTTON_RIGHT) menu_.cliqueDireito = true;
                if (!menu_.teclaFisica) menu_.teclaFisica = jogo::BOTAO_MOUSE + e.button.button;  // tela Teclas
            } else if (inventarioAberto_) {
                // Botão que é o atalho do inventário fecha; o esquerdo e o direito mexem nos itens
                if (atalhos_.acaoDa(jogo::BOTAO_MOUSE + e.button.button) == jogo::AcaoJogo::Inventario) abrirInventario(false);
                else if (e.button.button == SDL_BUTTON_LEFT) cliques_.pressionou = true;
                else if (e.button.button == SDL_BUTTON_RIGHT) cliques_.direito = true;
            } else if (!capturado_) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    capturar(true);
                    ignorarBotao_ = true;  // se o atacar for este botão, só depois de soltar
                }
            } else if (const auto a = atalhos_.acaoDa(jogo::BOTAO_MOUSE + e.button.button)) {
                acao(*a);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT) {
                if (inventarioAberto_ && !modoMenu_) {
                    cliques_.soltou = true;
                    posicaoMouse_ = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
                }
            }
            break;
        case SDL_MOUSEWHEEL:
            if (capturado_ && e.wheel.y != 0) trocarPendente_ = true;
            break;
        case SDL_KEYUP:
            if (modoMenu_ && e.key.keysym.scancode == modificadorNoMenu_) {
                if (!menu_.teclaFisica) menu_.teclaFisica = modificadorNoMenu_;
                modificadorNoMenu_ = 0;
            }
            break;
        case SDL_TEXTINPUT:
            if (modoMenu_) menu_.texto += e.text.text;
            break;
        case SDL_KEYDOWN:
            ultimoFoiControle_ = false;
            if (modoMenu_) {
                teclaNoMenu(e.key);
                if (!e.key.repeat) {
                    // Ctrl, Shift e Alt só valem ao soltar, sem outra tecla no meio (Alt+Tab não grava o Alt)
                    const int t = e.key.keysym.scancode;
                    const bool modificador = t >= SDL_SCANCODE_LCTRL && t <= SDL_SCANCODE_RGUI;
                    if (modificador) modificadorNoMenu_ = t;
                    else {
                        modificadorNoMenu_ = 0;
                        if (!menu_.teclaFisica) menu_.teclaFisica = t;  // tela Teclas
                    }
                }
                break;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE && !e.key.repeat) {
                if (inventarioAberto_) abrirInventario(false);
                else pausaPendente_ = true;
            }
            // Teclas de ação contam uma vez por aperto (segurar não repete)
            if (e.key.repeat || e.key.keysym.sym == SDLK_ESCAPE) break;
            if (const auto a = atalhos_.acaoDa(e.key.keysym.scancode)) acao(*a);
            break;
        case SDL_WINDOWEVENT:
            // Perder o foco no jogo pausa e solta o mouse (seção 11 da spec)
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                modificadorNoMenu_ = 0;  // Alt+Tab: o Alt que desceu não vira tecla
                if (capturado_) capturar(false);
                if (!modoMenu_) pausaPendente_ = true;
            }
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) focoGanhoEm_ = e.window.timestamp;
            break;
        case SDL_MOUSEMOTION:
            // O movimento que o cursor do controle provocou (SDL_WarpMouseInWindow) não é o jogador no mouse
            if (glm::ivec2(e.motion.x, e.motion.y) != ondeLevouMouse_) ultimoFoiControle_ = false;
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
    const Uint32 botoes = SDL_GetMouseState(nullptr, nullptr);
    const auto segura = [&](jogo::AcaoJogo a) { return jogo::apertada(atalhos_.de(a), teclas, botoes); };
    if (segura(jogo::AcaoJogo::Frente)) c.frente += 1.0f;
    if (segura(jogo::AcaoJogo::Tras)) c.frente -= 1.0f;
    if (segura(jogo::AcaoJogo::Direita)) c.lado += 1.0f;
    if (segura(jogo::AcaoJogo::Esquerda)) c.lado -= 1.0f;
    // O clique que capturou o mouse (ou a tecla que fechou um menu) só ataca depois de soltar
    const bool atacando = segura(jogo::AcaoJogo::Atacar);
    if (!atacando) ignorarBotao_ = false;
    c.atacar = capturado_ && !ignorarBotao_ && atacando;
    // Controle junto com teclado e mouse
    c.frente = std::clamp(c.frente + movimento_.frente, -1.0f, 1.0f);
    c.lado = std::clamp(c.lado + movimento_.lado, -1.0f, 1.0f);
    c.atacar = c.atacar || (capturado_ && movimento_.atacar);
    c.giro = giroAcumulado_;
    c.inclinar = inclinarAcumulado_;
    c.usar = usarPendente_;
    c.trocarArma = trocarPendente_;
    c.recarregar = recarregarPendente_;
    c.usarRapido = rapidoPendente_;
    c.pular = pularPendente_;
    giroAcumulado_ = inclinarAcumulado_ = 0.0f;
    usarPendente_ = trocarPendente_ = recarregarPendente_ = false;
    rapidoPendente_ = 0;
    pularPendente_ = false;
    return c;
}

Entrada::~Entrada() {
    if (controle_) SDL_GameControllerClose(controle_);
}

void Entrada::abrirControle(int indice) {
    if (!SDL_IsGameController(indice)) return;  // joystick sem mapeamento conhecido: fica de fora
    controle_ = SDL_GameControllerOpen(indice);
    if (!controle_) return;
    idControle_ = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controle_));
    antes_ = agora_ = lerControle();  // o que já está apertado ao ligar não dispara nada
}

ui::EstadoControle Entrada::lerControle() const {
    ui::EstadoControle e;
    if (!controle_) return e;
    e.ligado = true;
    const auto eixo = [&](SDL_GameControllerAxis a) {
        return std::clamp(SDL_GameControllerGetAxis(controle_, a) / 32767.0f, -1.0f, 1.0f);
    };
    e.esqX = eixo(SDL_CONTROLLER_AXIS_LEFTX);
    e.esqY = eixo(SDL_CONTROLLER_AXIS_LEFTY);
    e.dirX = eixo(SDL_CONTROLLER_AXIS_RIGHTX);
    e.dirY = eixo(SDL_CONTROLLER_AXIS_RIGHTY);
    e.gatilhoDir = std::max(0.0f, eixo(SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    using ui::BotaoControle;
    const std::pair<BotaoControle, SDL_GameControllerButton> botoes[] = {
        {BotaoControle::A, SDL_CONTROLLER_BUTTON_A},
        {BotaoControle::B, SDL_CONTROLLER_BUTTON_B},
        {BotaoControle::X, SDL_CONTROLLER_BUTTON_X},
        {BotaoControle::Y, SDL_CONTROLLER_BUTTON_Y},
        {BotaoControle::View, SDL_CONTROLLER_BUTTON_BACK},
        {BotaoControle::Start, SDL_CONTROLLER_BUTTON_START},
        {BotaoControle::LB, SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {BotaoControle::RB, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {BotaoControle::Cima, SDL_CONTROLLER_BUTTON_DPAD_UP},
        {BotaoControle::Baixo, SDL_CONTROLLER_BUTTON_DPAD_DOWN},
        {BotaoControle::Esquerda, SDL_CONTROLLER_BUTTON_DPAD_LEFT},
        {BotaoControle::Direita, SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
    };
    for (const auto& [nosso, sdl] : botoes)
        e.botoes[static_cast<size_t>(nosso)] = SDL_GameControllerGetButton(controle_, sdl) != 0;
    return e;
}

void Entrada::atualizarControle(float dt) {
    antes_ = agora_;
    agora_ = lerControle();
    movimento_ = {};
    if (!agora_.ligado) return;
    // Qualquer botão que desceu ou analógico fora da zona morta: os textos passam a mostrar os botões
    bool usou = agora_.gatilhoDir > 0.5f;
    for (size_t b = 0; b < agora_.botoes.size(); ++b) usou = usou || (agora_.botoes[b] && !antes_.botoes[b]);
    for (float v : {agora_.esqX, agora_.esqY, agora_.dirX, agora_.dirY}) usou = usou || ui::aplicarZonaMorta(v) != 0.0f;
    if (usou) ultimoFoiControle_ = true;

    using ui::BotaoControle;
    const auto desceu = [&](BotaoControle b) { return agora_.botao(b) && !antes_.botao(b); };

    if (modoMenu_) {
        const ui::EntradaMenu m = ui::menuDoControle(antes_, agora_);
        if (m.mover) menu_.mover = m.mover;
        if (m.ajustar) menu_.ajustar = m.ajustar;
        menu_.confirmar = menu_.confirmar || m.confirmar;
        menu_.voltar = menu_.voltar || m.voltar;
        return;
    }

    if (inventarioAberto_) {
        // Cursor: o mesmo do mouse (quem mexer por último manda); A pega/solta/arrasta, X usa, B, View ou Start fecham
        int largura = 0, altura = 0;
        SDL_GetWindowSize(janela_, &largura, &altura);
        const glm::vec2 novo = ui::moverCursor(posicaoMouse_, agora_, dt,
                                               {static_cast<float>(largura), static_cast<float>(altura)});
        if (novo != posicaoMouse_) {
            posicaoMouse_ = novo;
            ondeLevouMouse_ = glm::ivec2(std::lround(novo.x), std::lround(novo.y));
            SDL_WarpMouseInWindow(janela_, ondeLevouMouse_.x, ondeLevouMouse_.y);
        }
        if (desceu(BotaoControle::A)) cliques_.pressionou = true;
        if (antes_.botao(BotaoControle::A) && !agora_.botao(BotaoControle::A)) cliques_.soltou = true;
        if (desceu(BotaoControle::X)) cliques_.direito = true;
        if (desceu(BotaoControle::B) || desceu(BotaoControle::View) || desceu(BotaoControle::Start)) abrirInventario(false);
        return;
    }

    // No jogo: sem o mouse capturado (voltou do Alt+Tab), o controle recaptura
    if (!capturado_) {
        if (!usou) return;
        capturar(true);
        ignorarBotao_ = ataqueApertado();
    }
    movimento_ = ui::movimentoDoControle(agora_, dt, sensibilidadeControle_, inverterVertical_);
    giroAcumulado_ += movimento_.giro;
    inclinarAcumulado_ += movimento_.inclinar;
    const ui::EventosControle ev = ui::eventosDoControle(antes_, agora_);
    if (ev.pular) pularPendente_ = true;
    if (ev.usar) usarPendente_ = true;
    if (ev.recarregar) recarregarPendente_ = true;
    if (ev.trocarArma) trocarPendente_ = true;
    if (ev.usarRapido) rapidoPendente_ = ev.usarRapido;
    if (ev.mapa) mapaVisivel_ = !mapaVisivel_;
    if (ev.pausa) pausaPendente_ = true;
    if (ev.inventario) abrirInventario(true);
}

}
