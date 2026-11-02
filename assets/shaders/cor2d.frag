#version 330 core
in vec3 vCor;
out vec4 corFinal;

void main() {
    corFinal = vec4(vCor, 1.0);
}
