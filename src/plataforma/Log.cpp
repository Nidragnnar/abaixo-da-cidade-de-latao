#include "plataforma/Log.h"

#include <ctime>
#include <format>
#include <fstream>

namespace plataforma {

namespace {
std::ofstream arquivoLog;
}

void iniciarLog(const std::filesystem::path& arquivo) {
    arquivoLog.open(arquivo, std::ios::out | std::ios::trunc);
}

void logar(std::string_view mensagem) {
    if (!arquivoLog.is_open()) return;
    // Hora do relógio do PC via localtime_s (current_zone() lança exceção em Windows sem ICU)
    const std::time_t agora = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &agora);
    arquivoLog << std::format("[{:02}:{:02}:{:02}] ", local.tm_hour, local.tm_min, local.tm_sec) << mensagem << '\n';
    arquivoLog.flush();  // não perder linhas se o jogo fechar de repente
}

}
