#pragma once
#include <filesystem>
#include <string_view>

namespace plataforma {

// Abre (recria) o arquivo de log; chamar uma vez no início
void iniciarLog(const std::filesystem::path& arquivo);

// Grava uma linha com a hora; não faz nada se o log não foi iniciado
void logar(std::string_view mensagem);

}
