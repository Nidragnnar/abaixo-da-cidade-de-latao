#version 330 core
in vec3 vPos;
in vec2 vUv;
in float vCamada;
in vec3 vNormal;

uniform sampler2DArray uTexturas;
uniform vec3 uPosLanterna;      // olho do jogador
uniform vec3 uCorLanterna;
uniform float uAlcanceLanterna;
uniform vec3 uCorAmbiente;      // cor da zona
uniform float uFimNeblina;
uniform float uClarao;           // 1 durante o clarão do disparo

out vec4 corFinal;

void main() {
    float d = distance(vPos, uPosLanterna);
    // Lanterna a gás: forte perto, some no alcance
    float aten = pow(clamp(1.0 - d / uAlcanceLanterna, 0.0, 1.0), 2.0);
    float lambert = max(dot(normalize(vNormal), normalize(uPosLanterna - vPos)), 0.0);
    vec3 luz = uCorAmbiente + uCorLanterna * aten * (0.35 + 0.65 * lambert);
    luz += uClarao * vec3(1.0, 0.85, 0.6) * aten;  // clarão do disparo, com o mesmo alcance da lanterna

    vec4 tex = texture(uTexturas, vec3(vUv, vCamada));
    if (tex.a < 0.5) discard;
    vec3 cor = tex.rgb * luz;
    // Neblina até o preto
    cor = mix(cor, vec3(0.0), clamp(d / uFimNeblina, 0.0, 1.0));
    corFinal = vec4(cor, 1.0);
}
