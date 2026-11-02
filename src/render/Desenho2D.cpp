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
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VerticeCor), reinterpret_cast<void*>(offsetof(VerticeCor, cor)));
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(programa_);
    glUniform2f(locTela_, static_cast<float>(telaL), static_cast<float>(telaA));
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(VerticeCor)), v.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(v.size()));
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

Desenho2DTextura::Desenho2DTextura() {
    programa_ = compilarPrograma(plataforma::caminhoAsset("shaders/tex2d.vert"),
                                 plataforma::caminhoAsset("shaders/tex2d.frag"));
    locTela_ = glGetUniformLocation(programa_, "uTela");
    glUseProgram(programa_);
    glUniform1i(glGetUniformLocation(programa_, "uTexturas"), 0);
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const GLsizei passo = sizeof(VerticeTex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, passo, reinterpret_cast<void*>(offsetof(VerticeTex, posicao)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, passo, reinterpret_cast<void*>(offsetof(VerticeTex, uv)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, passo, reinterpret_cast<void*>(offsetof(VerticeTex, camada)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, passo, reinterpret_cast<void*>(offsetof(VerticeTex, cor)));
    glBindVertexArray(0);
}

Desenho2DTextura::~Desenho2DTextura() {
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(programa_);
}

void Desenho2DTextura::desenhar(const std::vector<VerticeTex>& v, GLuint arrayTextura, int telaL, int telaA) {
    if (v.empty()) return;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(programa_);
    glUniform2f(locTela_, static_cast<float>(telaL), static_cast<float>(telaA));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, arrayTextura);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(VerticeTex)), v.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(v.size()));
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

}
