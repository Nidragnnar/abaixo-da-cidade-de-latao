#include "render/Desenho2D.h"

#include <cstddef>

#include "plataforma/Caminhos.h"
#include "render/Shader.h"

namespace render {

Desenho2D::Desenho2D() {
    programa_ = compilarPrograma(plataforma::caminhoAsset("shaders/cor2d.vert"),
                                 plataforma::caminhoAsset("shaders/cor2d.frag"));
    locTela_ = glGetUniformLocation(programa_, "uTela");
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VerticeCor),
                          reinterpret_cast<void*>(offsetof(VerticeCor, posicao)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VerticeCor), reinterpret_cast<void*>(offsetof(VerticeCor, cor)));
    glBindVertexArray(0);
}

Desenho2D::~Desenho2D() {
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(programa_);
}

void Desenho2D::desenhar(const std::vector<VerticeCor>& v, int telaL, int telaA) {
    if (v.empty()) return;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);  // o triângulo do jogador pode vir em qualquer sentido
    glUseProgram(programa_);
    glUniform2f(locTela_, static_cast<float>(telaL), static_cast<float>(telaA));
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(VerticeCor)), v.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(v.size()));
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

}
