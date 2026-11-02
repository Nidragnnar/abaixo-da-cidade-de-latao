#include "jogo/ArquivoTexto.h"

#include <fstream>

namespace jogo {

bool gravarAtomico(const std::filesystem::path& arquivo, std::string_view conteudo) {
    try {
        if (arquivo.has_parent_path()) std::filesystem::create_directories(arquivo.parent_path());
        std::filesystem::path temporario = arquivo;
        temporario += ".tmp";
        {
            std::ofstream f(temporario, std::ios::binary | std::ios::trunc);
            if (!f) return false;
            f.write(conteudo.data(), static_cast<std::streamsize>(conteudo.size()));
            if (!f) return false;
        }
        std::filesystem::rename(temporario, arquivo);  // substitui o anterior
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool renomearCorrompido(const std::filesystem::path& arquivo) {
    std::error_code erro;
    std::filesystem::path corrompido = arquivo;
    corrompido += ".corrompido";
    std::filesystem::remove(corrompido, erro);
    std::filesystem::rename(arquivo, corrompido, erro);
    return !erro;
}

}
