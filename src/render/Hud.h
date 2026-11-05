#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "jogo/Itens.h"
#include "render/Primitivas2D.h"

namespace render {

struct ElementosHud {
    std::vector<VerticeCor> cores;  // barras e painéis; o flash vermelho vem por último
    std::vector<VerticeTex> texto;  // glifos da fonte
    std::vector<VerticeTex> arma;   // arma na mão (array das armas)
    std::vector<VerticeTex> icones; // itens rápidos (array de sprites)
};

struct RapidoHud {
    int icone = -1;  // camada no array de sprites; -1 = espaço vazio
    int quantidade = 0;
};

struct EstadoHud {
    int vida = 30, vidaMaxima = 30;
    int nivel = 1, xp = 0, xpProximo = 20;
    int engrenagens = 0, andar = 1;
    jogo::TipoItem arma = jogo::TipoItem::ChaveInglesa;  // a que está na mão
    bool armaDeFogoAtiva = false;
    int carga = 0, reserva = 0;
    jogo::TipoMunicao municao = jogo::TipoMunicao::Balas;
    bool semRecarga = false;  // Tesla: gasta da reserva
    bool recarregando = false;
    float animacao = 0;        // > 0 enquanto o golpe/tiro aparece
    float tempoDesdeDano = 9;
    std::string_view mensagem;
    std::array<RapidoHud, 4> rapidos{};  // espaços 1–4 do inventário
    // Luta com chefe (Etapa 7): barra larga no alto; nome vazio = sem barra
    std::string_view nomeChefe;
    int vidaChefe = 0, vidaMaximaChefe = 0;
};

// Camada no array das armas na mão: 0 chave parada, 1 chave golpe, 2 revólver parado, 3 tiro, 4 recarga,
// 5 sabre parado, 6 golpe, 7 martelo parado, 8 golpe, 9 bacamarte parado, 10 tiro, 11 recarga, 12 Tesla parada, 13 disparo
int quadroArma(jogo::TipoItem arma, bool ataque, bool recarregando);
constexpr int TOTAL_QUADROS_ARMA = 14;

// "6 | 12 balas", "| 8 células" (Tesla), "RECARREGANDO" ou vazio (arma branca na mão)
std::string textoMunicao(const EstadoHud& e);

// Vida com moldura de latão, nível e XP, mira, itens rápidos, munição, engrenagens, andar, mensagem,
// arma na mão e flash vermelho (seção 8 da spec)
ElementosHud construirHud(const EstadoHud& e, int telaL, int telaA);

struct DadosFim {
    int andar, nivel, inimigos, engrenagens, pontuacao;
    uint32_t semente;
    std::string_view causa;
    int posicaoPlacar;  // 0 se não entrou no placar
    bool vitoria = false;  // venceu a Grande Máquina (Etapa 7)
    int segundos = 0;      // tempo de jogo, mostrado na vitória
};

// Painel escuro com o resumo da partida (seção 7 da spec)
ElementosHud construirTelaFim(const DadosFim& d, int telaL, int telaA);

}
