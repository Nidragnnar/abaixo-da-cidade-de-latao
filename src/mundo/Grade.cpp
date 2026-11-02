#include "mundo/Grade.h"

#include <format>
#include <stdexcept>

namespace mundo {

Grade::Grade(int largura, int altura)
    : largura_(largura), altura_(altura), celulas_(static_cast<size_t>(largura) * altura, Celula::Parede) {}

Celula Grade::em(int x, int y) const {
    if (!dentro(x, y)) return Celula::Parede;
    return celulas_[static_cast<size_t>(y) * largura_ + x];
}

void Grade::definir(int x, int y, Celula c) {
    if (dentro(x, y)) celulas_[static_cast<size_t>(y) * largura_ + x] = c;
}

bool Grade::solida(int x, int y) const {
    Celula c = em(x, y);
    return c == Celula::Parede || c == Celula::Comporta || c == Celula::ComportaTrancada;
}

MapaCarregado carregarDeTexto(const std::vector<std::string>& linhas) {
    if (linhas.empty() || linhas[0].empty()) throw std::invalid_argument("mapa vazio");
    const int largura = static_cast<int>(linhas[0].size());
    const int altura = static_cast<int>(linhas.size());

    MapaCarregado mapa{Grade(largura, altura), {}};
    int inicios = 0;
    for (int y = 0; y < altura; ++y) {
        if (static_cast<int>(linhas[y].size()) != largura)
            throw std::invalid_argument(std::format("linha {} com tamanho diferente", y));
        for (int x = 0; x < largura; ++x) {
            switch (linhas[y][x]) {
                case '#': mapa.grade.definir(x, y, Celula::Parede); break;
                case '.': mapa.grade.definir(x, y, Celula::Chao); break;
                case '@':
                    mapa.grade.definir(x, y, Celula::Chao);
                    mapa.inicio = {x + 0.5f, y + 0.5f};
                    ++inicios;
                    break;
                default:
                    throw std::invalid_argument(std::format("caractere desconhecido em ({}, {})", x, y));
            }
        }
    }
    if (inicios != 1) throw std::invalid_argument("o mapa precisa de exatamente um '@'");
    return mapa;
}

}
