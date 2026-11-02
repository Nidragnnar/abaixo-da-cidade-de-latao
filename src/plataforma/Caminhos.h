#pragma once
#include <filesystem>
#include <string_view>

namespace plataforma {

// Pasta onde está o Roguelike.exe (não depende da pasta de trabalho)
std::filesystem::path pastaDoExecutavel();

// pasta do exe / assets / relativo
std::filesystem::path caminhoAsset(std::string_view relativo);

// Onde ficam placar e configurações: %APPDATA%\Roguelike, ou a pasta da opção --dados
std::filesystem::path pastaDeDados();
void definirPastaDeDados(const std::filesystem::path& pasta);

}
