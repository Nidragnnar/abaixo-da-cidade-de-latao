#include "jogo/Inventario.h"

#include <utility>

namespace jogo {

namespace {
bool valido(int i) { return i >= 0 && i < ESPACOS_INVENTARIO; }
}

Inventario::Inventario()
    : armaBranca_(criarItem(TipoItem::ChaveInglesa)),
      armaDeFogo_(criarItem(TipoItem::Revolver)),
      armadura_(criarItem(TipoItem::Casaco)) {
    Item tonicos = criarItem(TipoItem::Tonico);
    tonicos.quantidade = 2;
    espacos_[0] = tonicos;
}

bool Inventario::adicionar(Item i) {
    if (i.tipo == TipoItem::Tonico) {
        for (auto& e : espacos_)
            if (e && e->tipo == TipoItem::Tonico && e->quantidade + i.quantidade <= PILHA_MAXIMA_TONICOS) {
                e->quantidade += i.quantidade;
                return true;
            }
    }
    for (auto& e : espacos_)
        if (!e) {
            e = std::move(i);
            return true;
        }
    return false;
}

std::optional<Item> Inventario::remover(int i) {
    if (!valido(i)) return std::nullopt;
    std::optional<Item> r = std::move(espacos_[static_cast<size_t>(i)]);
    espacos_[static_cast<size_t>(i)].reset();
    return r;
}

void Inventario::trocar(int a, int b) {
    if (valido(a) && valido(b)) std::swap(espacos_[static_cast<size_t>(a)], espacos_[static_cast<size_t>(b)]);
}

bool Inventario::equipar(int i) {
    if (!valido(i) || !espacos_[static_cast<size_t>(i)]) return false;
    Item& item = *espacos_[static_cast<size_t>(i)];
    switch (classe(item.tipo)) {
        case ClasseItem::ArmaBranca: std::swap(item, armaBranca_); return true;
        case ClasseItem::ArmaDeFogo: std::swap(item, armaDeFogo_); return true;
        case ClasseItem::Armadura: std::swap(item, armadura_); return true;
        default: return false;
    }
}

bool Inventario::consumirTonico(int i) {
    if (!valido(i)) return false;
    auto& e = espacos_[static_cast<size_t>(i)];
    if (!e || e->tipo != TipoItem::Tonico) return false;
    if (--e->quantidade <= 0) e.reset();
    return true;
}

Atributos calcularAtributos(const Inventario& inv, int nivel) {
    const Item& a = inv.armadura();
    return {nivel - 1, dadosArmadura(a.tipo).defesa + somaBonus(a, TipoBonus::Defesa),
            VIDA_BASE + VIDA_POR_NIVEL * (nivel - 1) + somaBonus(a, TipoBonus::VidaMaxima)};
}

}
