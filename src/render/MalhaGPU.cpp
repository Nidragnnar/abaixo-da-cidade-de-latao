#include "render/MalhaGPU.h"

#include <cstddef>

namespace render {

MalhaGPU::~MalhaGPU() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}

void MalhaGPU::enviar(const std::vector<Vertice>& v, bool dinamica) {
    if (!vao_) {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
    }
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(Vertice)), v.data(),
                 dinamica ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    // Localizações: 0 posição, 1 uv, 2 camada, 3 normal
    const GLsizei passo = sizeof(Vertice);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, passo, reinterpret_cast<void*>(offsetof(Vertice, posicao)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, passo, reinterpret_cast<void*>(offsetof(Vertice, uv)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, passo, reinterpret_cast<void*>(offsetof(Vertice, camada)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, passo, reinterpret_cast<void*>(offsetof(Vertice, normal)));
    glBindVertexArray(0);
    quantidade_ = static_cast<GLsizei>(v.size());
}

void MalhaGPU::desenhar() const {
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, quantidade_);
    glBindVertexArray(0);
}

}
