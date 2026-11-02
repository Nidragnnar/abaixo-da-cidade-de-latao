#pragma once
#include <vector>

#include <glad/glad.h>

#include "render/Automapa.h"

namespace render {

// Triângulos coloridos em pixels da tela baixa (automapa e, depois, o HUD)
class Desenho2D {
public:
    Desenho2D();  // precisa de contexto OpenGL; lança std::runtime_error se o shader falhar
    ~Desenho2D();
    Desenho2D(const Desenho2D&) = delete;
    Desenho2D& operator=(const Desenho2D&) = delete;

    void desenhar(const std::vector<VerticeCor>& v, int telaL, int telaA);

private:
    GLuint programa_ = 0, vao_ = 0, vbo_ = 0;
    GLint locTela_ = -1;
};

}
