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
#include "render/Xadrez.h"

namespace render {

GLuint carregarArrayTexturas(const std::vector<std::filesystem::path>& arquivos) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, LADO_TEXTURA, LADO_TEXTURA, static_cast<GLsizei>(arquivos.size()),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    const auto reserva = gerarXadrez(LADO_TEXTURA);
    for (size_t i = 0; i < arquivos.size(); ++i) {
        const std::u8string nome = arquivos[i].u8string();
        int l = 0, a = 0, canais = 0;
        stbi_uc* px = stbi_load(reinterpret_cast<const char*>(nome.c_str()), &l, &a, &canais, 4);
        const void* dados = px;
        if (!px) {
            plataforma::logar(std::format("Textura não carregada ({}): {}", stbi_failure_reason(),
                                          arquivos[i].filename().string()));
            dados = reserva.data();
        } else if (l != LADO_TEXTURA || a != LADO_TEXTURA) {
            plataforma::logar(std::format("Textura com tamanho {}x{} (esperado {}x{}): {}", l, a, LADO_TEXTURA,
                                          LADO_TEXTURA, arquivos[i].filename().string()));
            dados = reserva.data();
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, static_cast<GLint>(i), LADO_TEXTURA, LADO_TEXTURA, 1, GL_RGBA,
                        GL_UNSIGNED_BYTE, dados);
        stbi_image_free(px);
    }

    // Pixels nítidos, repetindo nas bordas
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return tex;
}

}
