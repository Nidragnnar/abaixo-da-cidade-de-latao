#include "plataforma/Caminhos.h"

#include <SDL.h>

namespace plataforma {

namespace {
// Texto UTF-8 (SDL e nossos literais) → caminho do Windows, sem perder acentos
std::filesystem::path deUtf8(std::string_view texto) {
    return std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(texto.data()), texto.size()));
}
}

std::filesystem::path pastaDoExecutavel() {
    static const std::filesystem::path pasta = [] {
        char* base = SDL_GetBasePath();
        if (!base) return std::filesystem::current_path();
        std::filesystem::path p = deUtf8(base);
        SDL_free(base);
        return p;
    }();
    return pasta;
}

std::filesystem::path caminhoAsset(std::string_view relativo) {
    return pastaDoExecutavel() / "assets" / deUtf8(relativo);
}

}
