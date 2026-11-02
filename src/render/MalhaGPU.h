#pragma once
#include <vector>

#include <glad/glad.h>

#include "render/GeometriaMasmorra.h"

namespace render {

// Vértices da masmorra na placa de vídeo
class MalhaGPU {
public:
    MalhaGPU() = default;
    ~MalhaGPU();
    MalhaGPU(const MalhaGPU&) = delete;
    MalhaGPU& operator=(const MalhaGPU&) = delete;

    void enviar(const std::vector<Vertice>& v);
    void desenhar() const;

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLsizei quantidade_ = 0;
};

}
