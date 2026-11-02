#include "ui/Menus.h"

#include <algorithm>
#include <cstdint>
#include <format>

namespace ui {

namespace {

constexpr int ITEM_OPCOES_PRINCIPAL = 2, ITEM_OPCOES_PAUSA = 1, ITEM_ABANDONAR = 2;
constexpr int ITEM_SEMENTE = 4, ITEM_VOLTAR_OPCOES = 5;

// Muda um valor inteiro dentro da faixa; falso se já estava no limite
bool mudar(int& valor, int delta, int minimo, int maximo) {
    const int novo = std::clamp(valor + delta, minimo, maximo);
    if (novo == valor) return false;
    valor = novo;
    return true;
}

}

void Menus::irPara(Tela t, int selecionado) {
    tela_ = t;
    selecionado_ = selecionado;
    confirmandoAbandono_ = false;
}

void Menus::abrir(Tela t) { irPara(t); }

void Menus::focoPerdido() {
    if (tela_ == Tela::Jogo) irPara(Tela::Pausa);
}

std::string_view Menus::titulo() const {
    switch (tela_) {
        case Tela::Principal: return "Abaixo da Cidade de Latão";
        case Tela::Pausa: return "Pausa";
        case Tela::Opcoes: return "Opções";
        case Tela::Placar: return "Placar";
        default: return "";
    }
}

std::vector<std::string> Menus::itens(const jogo::Config& c) const {
    switch (tela_) {
        case Tela::Principal: return {"Novo jogo", "Placar", "Opções", "Sair"};
        case Tela::Pausa:
            return {"Continuar", "Opções", confirmandoAbandono_ ? "Confirmar: abandonar?" : "Abandonar partida"};
        case Tela::Placar: return {"Voltar"};
        case Tela::Opcoes:
            return {std::format("Sensibilidade do mouse: {}", c.sensibilidade),
                    std::format("Volume dos efeitos: {}", c.volumeEfeitos),
                    std::format("Volume da música: {}", c.volumeMusica),
                    std::string("Tela cheia: ") + (c.telaCheia ? "sim" : "não"),
                    c.semente ? std::format("Semente fixa: {}", *c.semente) : std::string("Semente fixa: aleatória"),
                    "Voltar"};
        default: return {};
    }
}

Acao Menus::ajustarOpcao(int item, int delta, jogo::Config& c) {
    bool mudou = false;
    switch (item) {
        case 0: mudou = mudar(c.sensibilidade, delta, jogo::SENSIBILIDADE_MIN, jogo::SENSIBILIDADE_MAX); break;
        case 1: mudou = mudar(c.volumeEfeitos, delta, 0, jogo::VOLUME_MAX); break;
        case 2: mudou = mudar(c.volumeMusica, delta, 0, jogo::VOLUME_MAX); break;
        case 3:
            c.telaCheia = !c.telaCheia;
            mudou = true;
            break;
        default: break;
    }
    return mudou ? Acao::OpcoesMudaram : Acao::Nenhuma;
}

Acao Menus::digitarSemente(const EntradaMenu& e, jogo::Config& c) {
    bool mudou = false;
    if (e.apagar && c.semente) {
        const uint32_t resto = *c.semente / 10;
        // "0" sozinho ou o último dígito: volta a aleatória
        if (resto == 0 && *c.semente < 10) c.semente.reset();
        else c.semente = resto;
        mudou = true;
    }
    for (char ch : e.texto) {
        if (ch < '0' || ch > '9') continue;
        const uint64_t novo = static_cast<uint64_t>(c.semente.value_or(0)) * 10 + static_cast<uint64_t>(ch - '0');
        if (novo > UINT32_MAX) continue;
        c.semente = static_cast<uint32_t>(novo);
        mudou = true;
    }
    return mudou ? Acao::OpcoesMudaram : Acao::Nenhuma;
}

Acao Menus::confirmarItem(jogo::Config& c) {
    switch (tela_) {
        case Tela::Principal:
            switch (selecionado_) {
                case 0: irPara(Tela::Jogo); return Acao::NovoJogo;
                case 1: irPara(Tela::Placar); return Acao::AbrirPlacar;
                case 2:
                    origemOpcoes_ = Tela::Principal;
                    irPara(Tela::Opcoes);
                    return Acao::Nenhuma;
                default: return Acao::Sair;
            }
        case Tela::Pausa:
            switch (selecionado_) {
                case 0: irPara(Tela::Jogo); return Acao::Continuar;
                case 1:
                    origemOpcoes_ = Tela::Pausa;
                    irPara(Tela::Opcoes);
                    return Acao::Nenhuma;
                default:
                    if (!confirmandoAbandono_) {
                        confirmandoAbandono_ = true;
                        return Acao::Nenhuma;
                    }
                    irPara(Tela::Principal);
                    return Acao::Abandonar;
            }
        case Tela::Placar: irPara(Tela::Principal, 1); return Acao::Nenhuma;
        case Tela::Opcoes:
            if (selecionado_ == ITEM_VOLTAR_OPCOES) {
                irPara(origemOpcoes_, origemOpcoes_ == Tela::Pausa ? ITEM_OPCOES_PAUSA : ITEM_OPCOES_PRINCIPAL);
                return Acao::SalvarOpcoes;
            }
            return selecionado_ == ITEM_SEMENTE ? Acao::Nenhuma : ajustarOpcao(selecionado_, 1, c);
        case Tela::Fim: irPara(Tela::Jogo); return Acao::NovoJogo;
        default: return Acao::Nenhuma;
    }
}

Acao Menus::atualizar(const EntradaMenu& e, jogo::Config& c) {
    switch (tela_) {
        case Tela::Jogo:
            if (e.voltar) irPara(Tela::Pausa);
            return Acao::Nenhuma;
        case Tela::Fim:
            if (e.confirmar) return confirmarItem(c);
            if (e.voltar) {
                irPara(Tela::Principal);
                return Acao::Abandonar;
            }
            return Acao::Nenhuma;
        default: break;
    }

    const int quantidade = static_cast<int>(itens(c).size());
    const bool mouseValido = e.itemSobMouse && *e.itemSobMouse >= 0 && *e.itemSobMouse < quantidade;
    // O mouse só seleciona quando passa para outro item (parado, não desfaz o que o teclado escolheu);
    // o clique vale sempre para o item que está embaixo dele
    const bool mouseMudou = e.itemSobMouse != ultimoSobMouse_;
    ultimoSobMouse_ = e.itemSobMouse;
    if (mouseValido && (mouseMudou || e.clique || e.cliqueDireito) && *e.itemSobMouse != selecionado_) {
        selecionado_ = *e.itemSobMouse;
        confirmandoAbandono_ = false;
    }
    if (e.mover != 0) {
        selecionado_ = ((selecionado_ + e.mover) % quantidade + quantidade) % quantidade;
        confirmandoAbandono_ = false;
    }

    if (e.voltar) {
        switch (tela_) {
            case Tela::Pausa: irPara(Tela::Jogo); return Acao::Continuar;
            case Tela::Placar: irPara(Tela::Principal, 1); return Acao::Nenhuma;
            case Tela::Opcoes:
                selecionado_ = ITEM_VOLTAR_OPCOES;
                return confirmarItem(c);
            default: return Acao::Nenhuma;
        }
    }

    // O clique só vale com o mouse em cima de um item
    const bool sobItem = e.itemSobMouse && *e.itemSobMouse == selecionado_;
    if (tela_ == Tela::Opcoes && selecionado_ < ITEM_SEMENTE) {
        if (e.ajustar != 0) return ajustarOpcao(selecionado_, e.ajustar, c);
        if (e.cliqueDireito && sobItem) return ajustarOpcao(selecionado_, -1, c);
    }
    if (tela_ == Tela::Opcoes && selecionado_ == ITEM_SEMENTE) {
        const Acao a = digitarSemente(e, c);
        if (a != Acao::Nenhuma) return a;
    }
    if (e.confirmar || (e.clique && sobItem)) return confirmarItem(c);
    return Acao::Nenhuma;
}

}
