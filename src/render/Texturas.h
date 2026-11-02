#pragma once
#include <filesystem>
#include <vector>

#include <glad/glad.h>

namespace render {

constexpr int LADO_TEXTURA = 64;

// Uma camada por arquivo (PNG 64×64). Arquivo ausente ou de outro tamanho vira xadrez rosa + linha no log.
GLuint carregarArrayTexturas(const std::vector<std::filesystem::path>& arquivos);

}
