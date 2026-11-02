#version 330 core
layout(location = 0) in vec2 aPos;  // pixels, origem em cima à esquerda
layout(location = 1) in vec2 aUv;
layout(location = 2) in float aCamada;
layout(location = 3) in vec4 aCor;

uniform vec2 uTela;

out vec2 vUv;
out float vCamada;
out vec4 vCor;

void main() {
    vUv = aUv;
    vCamada = aCamada;
    vCor = aCor;
    vec2 ndc = aPos / uTela * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);  // Y da tela cresce para baixo
}
