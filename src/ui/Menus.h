#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "jogo/Config.h"

namespace ui {

// Telas da seção 8 da spec: menu principal → jogo → pausa / inventário → fim de partida
enum class Tela { Principal, Jogo, Pausa, Placar, Opcoes, Fim, Teclas, Controle };

// O que o laço principal precisa fazer depois da navegação
enum class Acao {
    Nenhuma, NovoJogo, Continuar, Abandonar, Sair, OpcoesMudaram, SalvarOpcoes, AbrirPlacar,
    ContinuarSalva, SalvarESair  // "Continuar partida" (do arquivo) e "Salvar e sair" (pausa)
};

// Entrada já traduzida para os menus (desde o último quadro)
struct EntradaMenu {
    int mover = 0;    // −1 cima, +1 baixo
    int ajustar = 0;  // −1 / +1 (A/D, setas laterais)
    bool confirmar = false, voltar = false, apagar = false;
    std::optional<int> itemSobMouse;  // o mouse em cima seleciona
    bool clique = false, cliqueDireito = false;
    std::string texto;  // caracteres digitados (só dígitos contam, e só na semente)
    std::optional<int> teclaFisica;  // primeira tecla ou botão do mouse que desceu (jogo::Atalhos; tela Teclas)
};

// Navegação das telas: seleção, confirmação, ajuste das opções e semente digitada (puro, sem SDL)
class Menus {
public:
    Tela tela() const { return tela_; }
    // Jogo, Fim e Principal vêm de fora (início da partida, morte)
    void abrir(Tela t);
    // Jogo → Pausa; na tela Teclas cancela a espera; nas outras não faz nada
    void focoPerdido();
    // Há partida salva: o menu principal ganha "Continuar partida" e "Novo jogo" pede confirmação
    void definirPartidaSalva(bool salva) { partidaSalva_ = salva; }
    Acao atualizar(const EntradaMenu& e, jogo::Config& c);
    // Textos dos itens da tela atual
    std::vector<std::string> itens(const jogo::Config& c) const;
    int selecionado() const { return selecionado_; }
    // Tela Teclas: esperando a nova tecla da ação selecionada (Esc cancela)
    bool esperandoTecla() const { return esperandoTecla_; }
    // "Abaixo da Cidade de Latão", "Pausa", "Opções", "Placar" (vazio no jogo e no fim)
    std::string_view titulo() const;
    // Texto embaixo dos itens (partida salva que não abriu, salvamento que falhou); some ao trocar de tela
    void definirAviso(std::string aviso) { aviso_ = std::move(aviso); }
    const std::string& aviso() const { return aviso_; }

private:
    Acao confirmarItem(jogo::Config& c);
    Acao ajustarOpcao(int item, int delta, jogo::Config& c);
    Acao digitarSemente(const EntradaMenu& e, jogo::Config& c);
    Acao esperarTecla(const EntradaMenu& e, jogo::Config& c);
    void irPara(Tela t, int selecionado = 0);

    Tela tela_ = Tela::Principal;
    Tela origemOpcoes_ = Tela::Principal;
    int selecionado_ = 0;
    bool confirmandoAbandono_ = false;
    bool confirmandoNovo_ = false;  // "Novo jogo" com partida salva: segundo Enter apaga e começa
    bool esperandoTecla_ = false;
    bool partidaSalva_ = false;
    int base() const { return partidaSalva_ ? 1 : 0; }  // "Continuar partida" empurra os itens do principal
    std::string aviso_;
    std::optional<int> ultimoSobMouse_;  // item sob o mouse no quadro anterior (fica entre telas)
};

}
