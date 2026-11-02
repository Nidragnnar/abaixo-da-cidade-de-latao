#pragma once
#include <filesystem>
#include <string_view>

namespace jogo {

// Cria a pasta, grava num .tmp e troca (não corrompe se o jogo fechar no meio). Falso se não conseguiu.
bool gravarAtomico(const std::filesystem::path& arquivo, std::string_view conteudo);

// arquivo → arquivo.corrompido (substitui um anterior); falso se não conseguiu
bool renomearCorrompido(const std::filesystem::path& arquivo);

}
