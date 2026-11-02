#include "jogo/PassoFixo.h"

#include <algorithm>

namespace jogo {

PassoFixo::PassoFixo(double passo, double maxAcumulado) : passo_(passo), maxAcumulado_(maxAcumulado) {}

int PassoFixo::adicionar(double dtReal) {
    // Travadas longas viram no máximo maxAcumulado, para não entrar em espiral
    acumulado_ = std::min(acumulado_ + std::max(dtReal, 0.0), maxAcumulado_);
    int passos = 0;
    while (acumulado_ >= passo_) {
        acumulado_ -= passo_;
        ++passos;
    }
    return passos;
}

}
