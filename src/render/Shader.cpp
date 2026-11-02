#include "render/Shader.h"

#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace render {

namespace {

std::string lerArquivo(const std::filesystem::path& caminho) {
    std::ifstream f(caminho, std::ios::binary);
    if (!f) throw std::runtime_error(std::format("Arquivo de shader não encontrado: {}", caminho.string()));
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

GLuint compilarEstagio(GLenum tipo, const std::filesystem::path& caminho) {
    const std::string fonte = lerArquivo(caminho);
    const char* texto = fonte.c_str();
    GLuint s = glCreateShader(tipo);
    glShaderSource(s, 1, &texto, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        glDeleteShader(s);
        throw std::runtime_error(std::format("Erro no shader {}:\n{}", caminho.filename().string(), log));
    }
    return s;
}

}

GLuint compilarPrograma(const std::filesystem::path& vert, const std::filesystem::path& frag) {
    GLuint v = compilarEstagio(GL_VERTEX_SHADER, vert);
    GLuint f = compilarEstagio(GL_FRAGMENT_SHADER, frag);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        glDeleteProgram(p);
        throw std::runtime_error(std::format("Erro ao ligar shaders {} + {}:\n{}", vert.filename().string(),
                                             frag.filename().string(), log));
    }
    return p;
}

}
