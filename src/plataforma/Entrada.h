#pragma once
#include <SDL.h>
#include <glm/vec2.hpp>

#include "jogo/Atalhos.h"
#include "jogo/Comandos.h"
#include "ui/Controle.h"
#include "ui/Menus.h"

namespace plataforma {

// Cliques com o inventário aberto (desde a última consulta)
struct CliqueInventario {
    bool pressionou = false;  // botão esquerdo desceu
    bool soltou = false;      // botão esquerdo subiu
    bool direito = false;     // botão direito desceu
};

// Traduz teclado e mouse em jogo::Comandos (no jogo) ou ui::EntradaMenu (nos menus) e cuida da captura do mouse
class Entrada {
public:
    explicit Entrada(SDL_Window* janela) : janela_(janela) {}
    ~Entrada();
    Entrada(const Entrada&) = delete;
    Entrada& operator=(const Entrada&) = delete;

    void processarEvento(const SDL_Event& e);
    // Lê andar e atacar (pelos atalhos); devolve giro e eventos (usar, trocar/roda, recarregar, itens) acumulados desde a
    // última chamada, zerando-os
    jogo::Comandos coletar();
    // Joga fora o giro e os eventos acumulados (usado enquanto o jogo está pausado)
    void descartarGiro();
    // Radianos por pixel do mouse (opção de sensibilidade)
    void definirSensibilidade(float radPorPixel) { sensibilidade_ = radPorPixel; }
    // Teclas e botões das ações (opções); Esc, menus e roda ficam fixos
    void definirAtalhos(const jogo::Atalhos& a) { atalhos_ = a; }
    // Tab alterna o automapa
    bool mapaVisivel() const { return mapaVisivel_; }
    // I abre e fecha o inventário (abrir solta o mouse; fechar recaptura); Esc também fecha
    bool inventarioAberto() const { return inventarioAberto_; }
    void abrirInventario(bool sim);
    // Com o inventário aberto ou nos menus: posição do mouse (coordenadas da janela) e cliques
    glm::vec2 posicaoMouse() const { return posicaoMouse_; }
    CliqueInventario coletarCliques();
    // Esc no jogo (fora do inventário) ou a janela perdeu o foco no jogo; consome o pedido
    bool pediuPausa();
    // Menus: mouse livre, teclas viram navegação
    void modoMenu(bool sim);
    bool emModoMenu() const { return modoMenu_; }
    // Setas/WASD, Enter, Esc, Backspace, dígitos e cliques desde a última chamada (sem o item sob o mouse)
    ui::EntradaMenu coletarMenu();
    // Sai dos menus e captura o mouse (o clique que confirmou não vira ataque)
    void entrarNoJogo();
    void soltarMouse() { capturar(false); }

    // Controle de videogame (o primeiro ligado; liga e desliga com o jogo aberto): lê eixos e botões uma vez por
    // quadro, antes de coletar() e coletarMenu(); no jogo vira comandos, nos menus navegação, no inventário cursor
    void atualizarControle(float dt);
    void definirControle(int sensibilidade, bool inverter) {
        sensibilidadeControle_ = sensibilidade;
        inverterVertical_ = inverter;
    }
    // A última entrada veio do controle (os textos mostram o botão em vez da tecla)
    bool ultimoFoiControle() const { return ultimoFoiControle_; }

    bool capturado() const { return capturado_; }
    bool pediuSair() const { return pediuSair_; }

private:
    void capturar(bool sim);
    void teclaNoMenu(const SDL_KeyboardEvent& k);
    void acao(jogo::AcaoJogo a);  // tecla ou botão de uma ação desceu (no jogo)
    bool ataqueApertado() const;   // a tecla ou o botão do atacar está apertado agora

    static constexpr Uint32 CLIQUE_DO_FOCO_MS = 150;  // clique tão perto de ganhar o foco foi para ativar a janela

    SDL_Window* janela_;
    Uint32 focoGanhoEm_ = 0;          // SDL_GetTicks de quando a janela ganhou o foco
    float sensibilidade_ = 0.0025f;   // radianos por pixel
    float giroAcumulado_ = 0.0f;
    float inclinarAcumulado_ = 0.0f;  // mouse vertical
    bool capturado_ = false;
    bool pediuSair_ = false;
    bool usarPendente_ = false;        // E apertado desde o último coletar()
    bool trocarPendente_ = false;      // Q ou roda
    bool recarregarPendente_ = false;  // R
    int rapidoPendente_ = 0;           // 1–4
    bool pularPendente_ = false;       // Espaço
    bool pausaPendente_ = false;       // Esc ou foco perdido no jogo
    bool ignorarBotao_ = false;        // o clique que capturou o mouse não ataca (vale até soltar o atacar)
    bool mapaVisivel_ = false;
    bool inventarioAberto_ = false;
    bool modoMenu_ = false;
    glm::vec2 posicaoMouse_{0.0f};
    CliqueInventario cliques_;
    ui::EntradaMenu menu_;
    jogo::Atalhos atalhos_ = jogo::Atalhos::padrao();
    // Controle
    void abrirControle(int indice);
    ui::EstadoControle lerControle() const;
    SDL_GameController* controle_ = nullptr;
    SDL_JoystickID idControle_ = -1;
    ui::EstadoControle antes_, agora_;
    ui::MovimentoControle movimento_;  // do último atualizarControle (andar e atacar somam com o teclado)
    int sensibilidadeControle_ = 5;
    bool inverterVertical_ = false;
    bool ultimoFoiControle_ = false;
    glm::ivec2 ondeLevouMouse_{-1};  // o movimento que o SDL_WarpMouseInWindow gera não conta como mouse
};

}
