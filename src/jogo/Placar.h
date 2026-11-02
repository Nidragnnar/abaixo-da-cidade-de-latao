#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace jogo {

// 100 × andar + 10 × inimigos destruídos + engrenagens + 50 × nível (seção 7 da spec)
int calcularPontuacao(int andar, int inimigos, int engrenagens, int nivel);

struct RegistroPlacar {
    int pontuacao;
    int andar;
    int nivel;
    int inimigos;
    int engrenagens;
    uint32_t semente;
    std::string causa;
};

constexpr size_t TAMANHO_PLACAR = 10;

// Ordena por pontuação (no empate, o mais antigo fica na frente) e corta em 10.
// *posicao recebe a colocação do novo registro (1–10) ou 0 se ficou de fora.
std::vector<RegistroPlacar> inserirNoPlacar(std::vector<RegistroPlacar> placar, RegistroPlacar r, int* posicao);

struct LeituraPlacar {
    std::vector<RegistroPlacar> registros;
    bool estavaCorrompido = false;
    bool erroDeLeitura = false;  // existe mas não abriu (bloqueado, sem permissão): não gravar por cima
    bool renomeado = false;      // corrompido e o .corrompido foi mesmo criado
};

// Ausente: vazio. Linha inválida: o arquivo vira placar.txt.corrompido e o placar recomeça vazio.
LeituraPlacar lerPlacar(const std::filesystem::path& arquivo);

// Cria a pasta, grava num .tmp e troca (não corrompe se o jogo fechar no meio). Falso se não conseguiu.
bool gravarPlacar(const std::filesystem::path& arquivo, const std::vector<RegistroPlacar>& registros);

}
