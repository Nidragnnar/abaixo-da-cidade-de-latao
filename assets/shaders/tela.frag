#version 330 core
in vec2 vUv;
uniform sampler2D uImagem;
out vec4 corFinal;

void main() {
    corFinal = texture(uImagem, vUv);
}
