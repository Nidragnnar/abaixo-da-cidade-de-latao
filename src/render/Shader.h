#pragma once
#include <filesystem>

#include <glad/glad.h>

namespace render {

// Compila e liga os dois estágios; lança std::runtime_error com o log do GLSL
GLuint compilarPrograma(const std::filesystem::path& vert, const std::filesystem::path& frag);

}
