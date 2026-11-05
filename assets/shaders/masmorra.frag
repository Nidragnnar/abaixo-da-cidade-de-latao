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

// Lampiões e lustres do andar: os 16 mais perto da câmera (cor já com a tremulação)
uniform vec3 uLuzPos[16];
uniform vec3 uLuzCor[16];
uniform int uNumLuzes;
uniform float uLuzAlcance[16];  // lampião 5, lustre 7
uniform int uEmissivo;           // 1: o próprio lampião (sempre aceso, quase sem neblina)

out vec4 corFinal;

// Forte perto, some no alcance; a face virada para a luz recebe mais
float atenuacao(vec3 fonte, float alcance) {
    float aten = pow(clamp(1.0 - distance(vPos, fonte) / alcance, 0.0, 1.0), 2.0);
    float lambert = max(dot(normalize(vNormal), normalize(fonte - vPos)), 0.0);
    return aten * (0.35 + 0.65 * lambert);
}

void main() {
    float d = distance(vPos, uPosLanterna);
    vec4 tex = texture(uTexturas, vec3(vUv, vCamada));
    if (tex.a < 0.5) discard;
    if (uEmissivo == 1) {
        corFinal = vec4(mix(tex.rgb, vec3(0.0), 0.3 * clamp(d / uFimNeblina, 0.0, 1.0)), 1.0);
        return;
    }
    // Lanterna a gás e clarão do disparo, com o mesmo alcance
    float aten = pow(clamp(1.0 - d / uAlcanceLanterna, 0.0, 1.0), 2.0);
    vec3 luz = uCorAmbiente + uCorLanterna * atenuacao(uPosLanterna, uAlcanceLanterna);
    luz += uClarao * vec3(1.0, 0.85, 0.6) * aten;
    for (int i = 0; i < uNumLuzes; ++i) luz += uLuzCor[i] * atenuacao(uLuzPos[i], uLuzAlcance[i]);
    vec3 cor = tex.rgb * luz;
    // Neblina até o preto
    cor = mix(cor, vec3(0.0), clamp(d / uFimNeblina, 0.0, 1.0));
    corFinal = vec4(cor, 1.0);
}
