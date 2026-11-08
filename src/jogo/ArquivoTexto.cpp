#include "jogo/ArquivoTexto.h"

#include <fstream>

namespace jogo {

bool gravarAtomico(const std::filesystem::path& arquivo, std::string_view conteudo) {
    std::filesystem::path temporario = arquivo;
    temporario += ".tmp";
    try {
        if (arquivo.has_parent_path()) std::filesystem::create_directories(arquivo.parent_path());
        bool escreveu = false;
        {
            std::ofstream f(temporario, std::ios::binary | std::ios::trunc);
            if (f) {
                f.write(conteudo.data(), static_cast<std::streamsize>(conteudo.size()));
                f.close();  // disco cheio aparece aqui, ao esvaziar o buffer
                escreveu = !f.fail();
            }
        }
        if (escreveu) {
            std::filesystem::rename(temporario, arquivo);  // substitui o anterior só com o novo inteiro
            return true;
        }
    } catch (const std::exception&) {
    }
    std::error_code erro;
    std::filesystem::remove(temporario, erro);  // não deixa o .tmp para trás
    return false;
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
