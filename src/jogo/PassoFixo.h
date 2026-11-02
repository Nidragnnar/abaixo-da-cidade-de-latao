#pragma once

namespace jogo {

// Acumula o tempo real e diz quantos passos lógicos de tamanho fixo rodar
class PassoFixo {
public:
    explicit PassoFixo(double passo = 1.0 / 60.0, double maxAcumulado = 0.25);

    // Soma o tempo do quadro (limitado a maxAcumulado) e devolve quantos passos rodar
    int adicionar(double dtReal);
    // Fração de passo que sobrou, em [0, 1), para interpolar o desenho
    double alfa() const { return acumulado_ / passo_; }
    double passo() const { return passo_; }
    // Descarta a sobra (usado na pausa)
    void zerar() { acumulado_ = 0.0; }

private:
    double passo_;
    double maxAcumulado_;
    double acumulado_ = 0.0;
};

}
