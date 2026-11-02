#version 330 core
out vec2 vUv;

void main() {
    // Triângulo gigante que cobre a tela, sem buffer de vértices
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vUv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
