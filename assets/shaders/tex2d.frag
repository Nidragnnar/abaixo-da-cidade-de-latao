#version 330 core
in vec2 vUv;
in float vCamada;
in vec4 vCor;

uniform sampler2DArray uTexturas;

out vec4 corFinal;

void main() {
    vec4 tex = texture(uTexturas, vec3(vUv, vCamada));
    if (tex.a < 0.5) discard;
    corFinal = tex * vCor;
}
