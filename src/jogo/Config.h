#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>

#include "jogo/Atalhos.h"
#include "jogo/Dificuldade.h"

namespace jogo {

// Opções do jogador (seção 8 da spec), em config.txt na pasta de dados
struct Config {
    int sensibilidade = 5;            // 1–10
    int volumeEfeitos = 8;            // 0–10
    int volumeMusica = 5;             // 0–10
    bool telaCheia = false;
    std::optional<uint32_t> semente;  // vazio = aleatória
    Dificuldade dificuldade = Dificuldade::Normal;  // vale no próximo Novo jogo
    int sensibilidadeControle = 5;    // 1–10, mira pelo analógico direito
    bool inverterVertical = false;    // analógico direito para cima olha para baixo
    Atalhos teclas = Atalhos::padrao();  // tecla_<ação>; inválida ou repetida volta ao padrão só daquela ação
};

constexpr int SENSIBILIDADE_MIN = 1, SENSIBILIDADE_MAX = 10, VOLUME_MAX = 10;

struct LeituraConfig {
    Config config;
    bool estavaCorrompido = false;
    bool erroDeLeitura = false;  // existe mas não abriu (bloqueado): não gravar por cima
    bool renomeado = false;      // corrompido e o .corrompido foi mesmo criado
};

// Ausente: padrão. Linha sem '=', valor inválido ou fora da faixa: vira config.txt.corrompido e volta o padrão.
// Chave desconhecida é ignorada; chave ausente fica com o padrão.
LeituraConfig lerConfig(const std::filesystem::path& arquivo);

// Gravação atômica (como o placar); falso se não conseguiu
bool gravarConfig(const std::filesystem::path& arquivo, const Config& c);

// 0,0025 × 1,25^(s − 5)
float radianosPorPixel(int sensibilidade);

}
