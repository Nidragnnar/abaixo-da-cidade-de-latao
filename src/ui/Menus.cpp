#include "ui/Menus.h"

#include <algorithm>
#include <cstdint>
#include <format>

namespace ui {

namespace {

constexpr int ITEM_OPCOES_PRINCIPAL = 2, ITEM_OPCOES_PAUSA = 2, ITEM_ABANDONAR = 3;  // principal: sem a partida salva
constexpr int ITEM_SEMENTE = 4, ITEM_DIFICULDADE = 5, ITEM_INVERTER = 7, ITEM_TECLAS = 8, ITEM_CONTROLE = 9,
              ITEM_VOLTAR_OPCOES = 10;
constexpr int ITEM_RESTAURAR = jogo::TOTAL_ACOES, ITEM_VOLTAR_TECLAS = jogo::TOTAL_ACOES + 1;  // tela Teclas

// Tela Controle: o esquema fixo (seção 4 da spec), com o nome do PlayStation entre parênteses
const std::vector<std::string>& esquemaControle() {
    static const std::vector<std::string> v = {
        "Analógico esquerdo: andar",     "Analógico direito: olhar e mirar", "RT (PS: R2): atacar",
        "A (PS: X): pular",              "X (PS: Quadrado): usar",           "B (PS: Círculo): recarregar",
        "Y (PS: Triângulo): trocar arma", "Cruz: itens rápidos 1 a 4",       "View (PS: Share): inventário",
        "LB (PS: L1): mapa",             "Start (PS: Options): pausa",       "Menus: A confirma, B volta",
    };
    return v;
}

// Itens das Opções que mudam com ← → (a semente é digitada; Teclas… e Controle… abrem telas)
bool ajustavel(int item) { return item < ITEM_SEMENTE || (item >= ITEM_DIFICULDADE && item <= ITEM_INVERTER); }

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
    confirmandoAbandono_ = confirmandoNovo_ = false;
    aviso_.clear();  // o aviso é da tela em que apareceu
    esperandoTecla_ = false;
}

void Menus::abrir(Tela t) { irPara(t); }

void Menus::focoPerdido() {
    if (tela_ == Tela::Jogo) irPara(Tela::Pausa);
    if (tela_ == Tela::Teclas && esperandoTecla_) {  // outra janela recebeu a tecla: desiste
        esperandoTecla_ = false;
        aviso_.clear();
    }
}

std::string_view Menus::titulo() const {
    switch (tela_) {
        case Tela::Principal: return "Abaixo da Cidade de Latão";
        case Tela::Pausa: return "Pausa";
        case Tela::Opcoes: return "Opções";
        case Tela::Placar: return "Placar";
        case Tela::Teclas: return "Teclas";
        case Tela::Controle: return "Controle";
        default: return "";
    }
}

std::vector<std::string> Menus::itens(const jogo::Config& c) const {
    switch (tela_) {
        case Tela::Principal:
            if (!partidaSalva_) return {"Novo jogo", "Placar", "Opções", "Sair"};
            return {"Continuar partida", confirmandoNovo_ ? "Apagar a partida salva?" : "Novo jogo", "Placar", "Opções", "Sair"};
        case Tela::Pausa:
            return {"Continuar", "Salvar e sair", "Opções", confirmandoAbandono_ ? "Confirmar: abandonar?" : "Abandonar partida"};
        case Tela::Placar: return {"Voltar"};
        case Tela::Controle: {
            std::vector<std::string> v = esquemaControle();
            v.push_back("Voltar");
            return v;
        }
        case Tela::Teclas: {
            std::vector<std::string> v;
            for (int i = 0; i < jogo::TOTAL_ACOES; ++i) {
                const auto acao = static_cast<jogo::AcaoJogo>(i);
                const bool esperando = esperandoTecla_ && i == selecionado_;
                v.push_back(std::format("{}: {}", jogo::nomeDaAcao(acao),
                                        esperando ? "..." : jogo::nomeDaTecla(c.teclas.de(acao))));
            }
            v.push_back("Restaurar padrão");
            v.push_back("Voltar");
            return v;
        }
        case Tela::Opcoes:
            return {std::format("Sensibilidade do mouse: {}", c.sensibilidade),
                    std::format("Volume dos efeitos: {}", c.volumeEfeitos),
                    std::format("Volume da música: {}", c.volumeMusica),
                    std::string("Tela cheia: ") + (c.telaCheia ? "sim" : "não"),
                    c.semente ? std::format("Semente fixa: {}", *c.semente) : std::string("Semente fixa: aleatória"),
                    // A partida em andamento continua na dificuldade com que começou
                    std::format("Dificuldade: {}{}", jogo::nomeDa(c.dificuldade),
                                origemOpcoes_ == Tela::Pausa ? " (próx. partida)" : ""),
                    std::format("Sensibilidade do controle: {}", c.sensibilidadeControle),
                    std::string("Inverter eixo vertical: ") + (c.inverterVertical ? "sim" : "não"),
                    "Teclas...",
                    "Controle...",
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
        case ITEM_DIFICULDADE: {
            int d = static_cast<int>(c.dificuldade);
            mudou = mudar(d, delta, 0, jogo::TOTAL_DIFICULDADES - 1);
            c.dificuldade = static_cast<jogo::Dificuldade>(d);
            break;
        }
        case ITEM_DIFICULDADE + 1:
            mudou = mudar(c.sensibilidadeControle, delta, jogo::SENSIBILIDADE_MIN, jogo::SENSIBILIDADE_MAX);
            break;
        case ITEM_INVERTER:
            c.inverterVertical = !c.inverterVertical;
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

Acao Menus::esperarTecla(const EntradaMenu& e, jogo::Config& c) {
    if (e.voltar) {  // Esc cancela
        esperandoTecla_ = false;
        aviso_.clear();
        return Acao::Nenhuma;
    }
    if (!e.teclaFisica) return Acao::Nenhuma;
    if (!jogo::teclaAceita(*e.teclaFisica)) {
        aviso_ = "Essa tecla não pode ser usada";
        return Acao::Nenhuma;
    }
    c.teclas.definir(static_cast<jogo::AcaoJogo>(selecionado_), *e.teclaFisica);
    esperandoTecla_ = false;
    aviso_.clear();
    return Acao::OpcoesMudaram;
}

Acao Menus::confirmarItem(jogo::Config& c) {
    switch (tela_) {
        case Tela::Principal:
            if (partidaSalva_ && selecionado_ == 0) {
                irPara(Tela::Jogo);
                return Acao::ContinuarSalva;
            }
            switch (selecionado_ - base()) {
                case 0:
                    if (partidaSalva_ && !confirmandoNovo_) {  // apagaria a partida salva: pergunta antes
                        confirmandoNovo_ = true;
                        return Acao::Nenhuma;
                    }
                    irPara(Tela::Jogo);
                    return Acao::NovoJogo;
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
                case 1: return Acao::SalvarESair;  // quem grava decide para onde vai (falhou: fica na pausa)
                case 2:
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
        case Tela::Placar: irPara(Tela::Principal, base() + 1); return Acao::Nenhuma;
        case Tela::Opcoes:
            if (selecionado_ == ITEM_VOLTAR_OPCOES) {
                irPara(origemOpcoes_, origemOpcoes_ == Tela::Pausa ? ITEM_OPCOES_PAUSA : base() + ITEM_OPCOES_PRINCIPAL);
                return Acao::SalvarOpcoes;
            }
            if (selecionado_ == ITEM_TECLAS) {
                irPara(Tela::Teclas);
                return Acao::Nenhuma;
            }
            if (selecionado_ == ITEM_CONTROLE) {
                irPara(Tela::Controle);
                return Acao::Nenhuma;
            }
            return ajustavel(selecionado_) ? ajustarOpcao(selecionado_, 1, c) : Acao::Nenhuma;
        case Tela::Controle:  // só leitura: só o "Voltar" faz algo
            if (selecionado_ != static_cast<int>(esquemaControle().size())) return Acao::Nenhuma;
            irPara(Tela::Opcoes, ITEM_CONTROLE);
            return Acao::Nenhuma;
        case Tela::Teclas:
            if (selecionado_ < jogo::TOTAL_ACOES) {
                // A tecla nova vem dos próximos quadros (a que abriu a espera não conta)
                esperandoTecla_ = true;
                aviso_ = "Aperte a tecla ou o botão (Esc cancela)";
                return Acao::Nenhuma;
            }
            if (selecionado_ == ITEM_RESTAURAR) {
                c.teclas = jogo::Atalhos::padrao();
                return Acao::OpcoesMudaram;
            }
            irPara(Tela::Opcoes, ITEM_TECLAS);
            return Acao::SalvarOpcoes;
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
        case Tela::Teclas:
            if (esperandoTecla_) {
                ultimoSobMouse_ = e.itemSobMouse;
                return esperarTecla(e, c);
            }
            break;
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
        confirmandoAbandono_ = confirmandoNovo_ = false;
    }
    if (e.mover != 0) {
        selecionado_ = ((selecionado_ + e.mover) % quantidade + quantidade) % quantidade;
        confirmandoAbandono_ = confirmandoNovo_ = false;
    }

    if (e.voltar) {
        switch (tela_) {
            case Tela::Pausa: irPara(Tela::Jogo); return Acao::Continuar;
            case Tela::Placar: irPara(Tela::Principal, base() + 1); return Acao::Nenhuma;
            case Tela::Opcoes:
                selecionado_ = ITEM_VOLTAR_OPCOES;
                return confirmarItem(c);
            case Tela::Teclas:
                selecionado_ = ITEM_VOLTAR_TECLAS;
                return confirmarItem(c);
            case Tela::Controle: irPara(Tela::Opcoes, ITEM_CONTROLE); return Acao::Nenhuma;
            default: return Acao::Nenhuma;
        }
    }

    // O clique só vale com o mouse em cima de um item
    const bool sobItem = e.itemSobMouse && *e.itemSobMouse == selecionado_;
    if (tela_ == Tela::Opcoes && ajustavel(selecionado_)) {
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
