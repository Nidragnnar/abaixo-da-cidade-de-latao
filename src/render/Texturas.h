#pragma once
#include <filesystem>
#include <vector>

#include <glad/glad.h>

namespace render {

constexpr int LADO_TEXTURA = 128;  // paredes, chão, teto e sprites (arte nova; antes 64)

// Uma camada por arquivo (PNG lado × lado). Arquivo ausente ou de outro tamanho vira xadrez rosa + linha no log.
// Com o lado padrão (paredes) a textura se repete; com outro lado (armas na mão) fica presa na borda.
GLuint carregarArrayTexturas(const std::vector<std::filesystem::path>& arquivos, int lado = LADO_TEXTURA);

// Grade de glifos 8x8 (16 por linha, na ordem de caracteresFonte()) → uma camada por glifo
GLuint carregarFonte(const std::filesystem::path& png);

}
