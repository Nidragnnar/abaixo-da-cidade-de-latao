#include "render/Texturas.h"

#include <cstdint>
#include <format>

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_WINDOWS_UTF8
#include <stb_image.h>
#pragma warning(pop)

#include "plataforma/Log.h"
#include "render/Texto.h"
#include "render/Xadrez.h"

namespace render {

namespace {

stbi_uc* carregarPng(const std::filesystem::path& arquivo, int& l, int& a) {
    const std::u8string nome = arquivo.u8string();
    int canais = 0;
    return stbi_load(reinterpret_cast<const char*>(nome.c_str()), &l, &a, &canais, 4);
}

void criarArray(GLuint& tex, int lado, int camadas) {
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, lado, lado, camadas, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

// Pixels nítidos; repete nas bordas (paredes) ou prende na borda (glifos e HUD)
void filtros(bool repetir) {
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    const GLint modo = repetir ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, modo);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, modo);
}

}

GLuint carregarArrayTexturas(const std::vector<std::filesystem::path>& arquivos, int lado) {
    GLuint tex = 0;
    criarArray(tex, lado, static_cast<int>(arquivos.size()));
    const auto reserva = gerarXadrez(lado);
    for (size_t i = 0; i < arquivos.size(); ++i) {
        int l = 0, a = 0;
        stbi_uc* px = carregarPng(arquivos[i], l, a);
        const void* dados = px;
        if (!px) {
            plataforma::logar(std::format("Textura não carregada ({}): {}", stbi_failure_reason(),
                                          arquivos[i].filename().string()));
            dados = reserva.data();
        } else if (l != lado || a != lado) {
            plataforma::logar(std::format("Textura com tamanho {}x{} (esperado {}x{}): {}", l, a, lado, lado,
                                          arquivos[i].filename().string()));
            dados = reserva.data();
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, static_cast<GLint>(i), lado, lado, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                        dados);
        stbi_image_free(px);
    }
    filtros(lado == LADO_TEXTURA);
    return tex;
}

GLuint carregarFonte(const std::filesystem::path& png) {
    const int glifos = static_cast<int>(caracteresFonte().size());
    GLuint tex = 0;
    criarArray(tex, LADO_GLIFO, glifos);
    int l = 0, a = 0;
    stbi_uc* px = carregarPng(png, l, a);
    const int linhas = (glifos + COLUNAS_FONTE - 1) / COLUNAS_FONTE;
    if (!px || l != COLUNAS_FONTE * LADO_GLIFO || a < linhas * LADO_GLIFO) {
        plataforma::logar(std::format("Fonte não carregada ou com tamanho errado: {}", png.filename().string()));
        const auto reserva = gerarXadrez(LADO_GLIFO);
        for (int i = 0; i < glifos; ++i)
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, LADO_GLIFO, LADO_GLIFO, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                            reserva.data());
    } else {
        // Recorta cada célula 8x8 da grade direto da imagem
        glPixelStorei(GL_UNPACK_ROW_LENGTH, l);
        for (int i = 0; i < glifos; ++i) {
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, (i % COLUNAS_FONTE) * LADO_GLIFO);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, (i / COLUNAS_FONTE) * LADO_GLIFO);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, LADO_GLIFO, LADO_GLIFO, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        }
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    }
    stbi_image_free(px);
    filtros(false);
    return tex;
}

}
