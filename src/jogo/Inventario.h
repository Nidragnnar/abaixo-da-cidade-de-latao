#pragma once
#include <array>
#include <optional>

#include "jogo/Itens.h"

namespace jogo {

constexpr int ESPACOS_INVENTARIO = 16;
constexpr int PILHA_MAXIMA_TONICOS = 9;
constexpr int VIDA_BASE = 30;
constexpr int VIDA_POR_NIVEL = 8;

// 16 espaços + arma branca, arma de fogo e armadura equipadas (nunca vazias)
class Inventario {
public:
    // Equipamento inicial (chave inglesa, revólver carregado, casaco) e 2 tônicos no espaço 0
    Inventario();

    const std::optional<Item>& espaco(int i) const { return espacos_[static_cast<size_t>(i)]; }
    const Item& armaBranca() const { return armaBranca_; }
    const Item& armaDeFogo() const { return armaDeFogo_; }
    Item& armaDeFogo() { return armaDeFogo_; }
    const Item& armadura() const { return armadura_; }

    // Tônico empilha num espaço com tônico (até 9); senão vai para o primeiro livre. Falso se não coube.
    bool adicionar(Item i);
    std::optional<Item> remover(int i);
    void trocar(int a, int b);
    // Arma ou armadura do espaço vai para o equipamento; o equipado anterior fica nesse mesmo espaço
    bool equipar(int i);
    // Tira um tônico do espaço (esvazia em 0); falso se ali não há tônico
    bool consumirTonico(int i);

private:
    std::array<std::optional<Item>, ESPACOS_INVENTARIO> espacos_;
    Item armaBranca_, armaDeFogo_, armadura_;
};

// Onde o item é equipado: 0 arma branca, 1 arma de fogo, 2 armadura; nada para consumíveis
std::optional<int> espacoDeEquipamento(TipoItem t);

struct Atributos {
    int danoBase;
    int defesa;
    int vidaMaxima;
};

// danoBase = nível − 1; defesa = armadura + bônus; vida máxima = 30 + 8 × (nível − 1) + bônus de vida
Atributos calcularAtributos(const Inventario& inv, int nivel);

}
