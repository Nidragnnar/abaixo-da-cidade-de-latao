#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 2) in float aCamada;
layout(location = 3) in vec3 aNormal;

uniform mat4 uVisao;
uniform mat4 uProjecao;

out vec3 vPos;
out vec2 vUv;
out float vCamada;
out vec3 vNormal;

void main() {
    vPos = aPos;  // a masmorra já está em coordenadas do mundo
    vUv = aUv;
    vCamada = aCamada;
    vNormal = aNormal;
    gl_Position = uProjecao * uVisao * vec4(aPos, 1.0);
}
