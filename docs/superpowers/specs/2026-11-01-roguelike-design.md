# Roguelike steampunk em primeira pessoa — Especificação de design

Data: 2026-11-01 · Autor: Nidragnnar · Status: aguardando revisão

## 1. Objetivo

Um roguelike de masmorra em primeira pessoa, em tempo real, inspirado em The Elder Scrolls: Arena e Doom,
com tema **steampunk**: túneis, fábricas e minas sob uma cidade vitoriana, armas de fogo a vapor e autômatos.
É um jogo para compartilhar com amigos que não programam: precisa vir numa pasta `.zip` pronta e com um guia simples.

**Critério de sucesso da v1:** um amigo descompacta o `.zip`, abre `Roguelike.exe` e joga uma partida completa,
com masmorra aleatória, combate, itens, níveis, morte permanente e placar, sem instalar nada.

### Decisões tomadas

| Tema | Decisão |
|---|---|
| Ritmo | Tempo real, movimento livre (não por turnos) |
| Público | Jogo para compartilhar (código fica com o Claude; Nidragnnar dirige e testa) |
| Ambientação | Steampunk vitoriano subterrâneo |
| Combate | Meio a meio: armas brancas e armas de fogo com o mesmo peso; munição escassa obriga a alternar |
| Arte | Pacotes gratuitos com licença livre (CC0/CC-BY), visual pixel art retrô |
| Escopo v1 | Núcleo roguelike + itens, inventário, XP e níveis |
| Tecnologia | C++20, SDL2, OpenGL 3.3 core |

### Fora da v1 (etapas futuras)

Classes de personagem, engenhocas/magias (equivalente steampunk das magias), atributos estilo Arena,
salvar no meio da partida, chefes, olhar para cima e para baixo, controle (gamepad).

## 2. Base técnica

- **Linguagem e compilação:** C++20, CMake ≥ 3.25, Visual Studio Community 2026 (MSVC), Windows 10/11 64 bits.
- **Bibliotecas** (baixadas pelo CMake `FetchContent` na primeira compilação, com versões fixas):
  - **SDL2 2.32.x** (última série da SDL2): janela, contexto OpenGL, teclado, mouse, áudio (`SDL_mixer` 2.8.x para tocar OGG/WAV).
  - **glad**: carregador de funções do OpenGL 3.3 core (gerado e incluído no repositório em `externo\glad\`).
  - **glm 1.0.x**: matemática 3D.
  - **stb_image** (arquivo único, incluído em `externo\stb\`): carregar PNG.
  - **doctest**: testes automáticos.
- **Laço principal:** lógica em passo fixo de 60 Hz (acumulador de tempo); o desenho roda o mais rápido possível, com V-Sync ligado.

### Organização

```
Z:\Roguelike
  CMakeLists.txt
  src\
    main.cpp        laço principal: entrada → lógica (60 Hz) → desenho
    plataforma\     janela, entrada, tempo, áudio, caminhos de arquivo (único lugar que usa SDL)
    render\         shaders, texturas, malha da masmorra, sprites, HUD, framebuffer de baixa resolução
    mundo\          grade do andar, gerador procedural, linha de visão, caminhos na grade
    jogo\           jogador, inimigos, combate, tiros e projéteis, itens, inventário, XP, regras
    ui\             menus, inventário na tela, placar, opções
  externo\          glad, stb (código de terceiros incluído)
  assets\           texturas, sprites, sons, música, fontes, LICENCAS.txt
  testes\           testes automáticos da lógica (doctest)
  docs\             esta especificação, planos, guia para amigos
```

**Regra de dependência:** `mundo\` e `jogo\` não incluem SDL nem OpenGL. Eles recebem a entrada já traduzida
(ex.: "andar para frente", "atacar") e expõem o estado para o `render\` desenhar. Isso permite testar a lógica sem janela.

**Aleatoriedade:** um único gerador (`std::mt19937`) por partida, criado a partir de uma semente de 32 bits.
A mesma semente gera sempre os mesmos andares e os mesmos itens.

## 3. A masmorra

### Zonas

| Andares | Zona | Paredes / chão / teto | Luz ambiente |
|---|---|---|---|
| 1–2 | Esgotos | tijolo úmido, grades, pedra molhada | lampiões a gás, amarelada e fraca |
| 3–4 | Fábrica abandonada | chapas rebitadas, canos de latão, piso de ferro | fornalhas, alaranjada |
| 5+ | Minas e Grande Máquina | rocha, engrenagens gigantes, vigas | lâmpadas elétricas, azulada |

A zona só troca o conjunto de texturas e a cor da luz; o gerador é o mesmo.

### Grade

Cada andar é uma grade de 64×64 células; cada célula mede 1×1 unidade e as paredes têm 1 unidade de altura.
Tipos de célula: `Parede`, `Chao`, `Comporta`, `ComportaTrancada`, `Elevador`.

### Gerador procedural

1. Sortear de 8 a 15 salas retangulares (lados de 4 a 10 células), sem sobreposição, com 1 célula de margem.
2. Ligar todas as salas por corredores em "L" de 1 célula de largura, formando uma árvore (todas alcançáveis).
3. Acrescentar de 2 a 4 corredores extras entre salas próximas, para criar ciclos.
4. Pôr comportas onde corredores entram em salas (cerca de 50% das entradas).
5. Sala inicial = sala sorteada; sala do **elevador de carga** (desce ao próximo andar) = a mais distante da inicial
   (distância pelo caminho na grade).
6. A partir do andar 2, com 40% de chance: a única entrada da sala do elevador vira `ComportaTrancada`
   e a **válvula-chave** vai para uma sala alcançável sem passar por essa comporta.
7. Espalhar inimigos e itens (seções 5 e 6), nunca na sala inicial.

**Garantias verificadas por testes:** todas as células de chão são alcançáveis a partir do início
(tratando a comporta trancada como aberta); o elevador é alcançável; a válvula-chave é alcançável sem atravessar
a comporta trancada; a mesma semente gera a mesma grade.

### Desenho

- Ao entrar no andar, montar **uma malha única** com: faces de parede entre célula sólida e célula aberta, chão e teto
  de cada célula aberta. As texturas de 64×64 ficam num array de texturas do OpenGL (`GL_TEXTURE_2D_ARRAY`, uma camada por textura).
- Comportas são objetos separados (uma placa texturizada) que sobem em 0,5 s ao abrir.
- A cena é desenhada num **framebuffer de 480×270** e ampliada para a janela com filtro "vizinho mais próximo"
  (pixels nítidos), mantendo a proporção com faixas pretas se necessário.
- **Iluminação:** luz pontual quente da **lanterna a gás** do jogador (alcance de cerca de 6 células) + luz ambiente
  fraca na cor da zona + **neblina** que escurece até o preto em cerca de 12 células. O clarão dos disparos
  ilumina a cena por 0,05 s. Sem sombras.
- **Sprites:** inimigos, itens, projéteis e elevador são quadros 2D que giram só no eixo vertical para encarar a câmera,
  com descarte de pixels transparentes (sem ordenar por profundidade). Inimigos têm animações por quadros:
  parado, andar, atacar, morrer.
- **Câmera:** altura dos olhos 0,5; campo de visão horizontal de 75°; sem olhar para cima/baixo na v1.
- **Colisão:** jogador e inimigos são círculos (raio 0,25) que deslizam ao longo das paredes e comportas fechadas.
- **Automapa (Tab):** visão 2D de cima das células já vistas pelo jogador, com a posição e a direção dele.

## 4. Jogador e combate

### Controles

| Tecla | Ação |
|---|---|
| W A S D | andar e andar de lado |
| Mouse (horizontal) | girar |
| Clique esquerdo | atacar com a arma na mão |
| Q ou roda do mouse | alternar entre arma branca e arma de fogo |
| R | recarregar a arma de fogo |
| E | usar: abrir comporta, chamar elevador, pegar item à frente |
| I | inventário (pausa o jogo) |
| 1–4 | usar item rápido (ex.: tônico) |
| Tab | automapa |
| Esc | menu de pausa |

### Atributos do jogador

- Vida inicial 30. Ataque base 0, defesa base 0 (as armas e armaduras dão os valores).
- **Dano recebido** = máximo(1, dano do golpe − defesa).
- **XP para o próximo nível** = 20 × nível atual. Cada nível: +8 de vida máxima, +1 de dano base, vida cheia.
- Começa com chave inglesa comum, revólver comum (carregado) + 12 balas, casaco de couro comum e 2 tônicos restauradores.

### Armas brancas (sem munição)

| Arma | Dano | Intervalo entre golpes | Alcance | Detalhe |
|---|---|---|---|---|
| Chave inglesa | 4–6 | 0,35 s | 1,1 | rápida |
| Sabre de oficial | 6–9 | 0,5 s | 1,3 | equilibrado |
| Martelo a vapor | 11–15 | 0,9 s | 1,1 | empurra bem mais longe |

Golpe corpo a corpo acerta todos os inimigos num arco de 60° à frente, dentro do alcance.
Um golpe empurra o inimigo para trás e interrompe o ataque dele.

### Armas de fogo

| Arma | Dano | Intervalo | Carga | Recarga | Munição | Detalhe |
|---|---|---|---|---|---|---|
| Revólver | 7–10 | 0,4 s | 6 | 1,5 s | balas | tiro instantâneo, dispersão de 2° |
| Bacamarte | 7 chumbos × 2–4 | 0,6 s | 2 | 2,2 s | cartuchos | leque de 12°; o dano cai pela metade depois de 4 células |
| Pistola Tesla | 14–20 | 0,7 s | — | sem recarga | células elétricas | raio instantâneo, alcance 15, gasta 1 célula por disparo |

- Os tiros do jogador são instantâneos (o primeiro inimigo ou parede na linha do tiro é atingido), como em Doom.
- Disparar faz barulho: acorda inimigos parados a até 10 células (golpes corpo a corpo: 4 células).
- **Munição** não ocupa espaço no inventário; fica num contador por tipo. Máximos: balas 48, cartuchos 20, células 12.
- O bacamarte aparece a partir do andar 2 e a pistola Tesla a partir do andar 4.

Ao levar dano, a tela pisca em vermelho e a arma na mão balança.

## 5. Inimigos

| Inimigo | Vida | Dano | Velocidade | Comportamento | XP |
|---|---|---|---|---|---|
| Rato mecânico | 6 | 2–3 | rápida | aparece em bandos de 2–4 | 3 |
| Autômato operário | 16 | 4–6 | média | robô de fábrica, corpo a corpo | 8 |
| Atirador da Guilda | 10 | 3–5 (tiro) | média | mercenário de máscara de gás; mantém de 3 a 6 células de distância e atira | 8 |
| Colosso a vapor | 40 | 8–12 | lenta | blindado (defesa 2), golpe demorado | 20 |

- Os tiros do Atirador da Guilda são **projéteis visíveis** (velocidade 10 células/s), para que dê para desviar.
- **Escala por andar:** vida e dano × (1 + 0,15 × (andar − 1)).
- **Quantidade:** de 6 + 2 × andar até 10 + 2 × andar inimigos por andar. Atiradores a partir do andar 2, Colossos a partir do 3.
- **Estados:** `Parado` → (vê o jogador por linha de visão na grade até 10 células, ou ouve barulho dentro do alcance
  da seção 4) → `Perseguindo` (caminho na grade por busca em largura, recalculado a cada 0,5 s) → `Atacando` (no alcance)
  → `Destruido` (destroços somem depois de 10 s).
- Inimigos não abrem comportas trancadas, mas abrem comportas comuns.

## 6. Itens e inventário

- **Inventário:** 16 espaços + 3 equipamentos (arma branca, arma de fogo, armadura). Tônicos se empilham.
- **Qualidade:** comum (70%), modificada (25%, 1 bônus) e obra-prima (5%, 2 bônus).
  A chance de modificada/obra-prima sobe 2% por andar.
- **Bônus possíveis ("modificações"):** +dano, +defesa, +vida máxima, +velocidade de ataque,
  e em armas de fogo +carga e recarga mais rápida. Nome gerado (ex.: "Revólver Calibrado +3").
- **Armaduras:** casaco de couro (defesa 1), colete blindado (defesa 3, a partir do andar 3),
  armadura de latão (defesa 5, a partir do andar 5).
- **Consumíveis:** tônico restaurador (cura 15).
- **Munição no chão:** caixa de balas (6), caixa de cartuchos (4), células elétricas (3). Pegar acima do máximo deixa o resto no chão.
- **Engrenagens de latão** (moeda): pilhas de 5 a 20 × andar; somam na pontuação.
- **Válvula-chave:** pega automaticamente e usada automaticamente na comporta trancada do mesmo andar.
- **Quantidade por andar:** de 4 a 7 itens e de 3 a 5 caixas de munição no chão; cada inimigo destruído tem
  25% de chance de soltar um item e 35% de soltar munição (Atiradores sempre soltam balas).
- Pegar item: olhar para ele a até 1,5 de distância e apertar E. Se o inventário estiver cheio, aparece uma mensagem e o item fica no chão.

## 7. Morte permanente e placar

- Vida chegou a 0: a partida acaba (não há como continuar).
- **Pontuação** = 100 × andar alcançado + 10 × inimigos destruídos + engrenagens + 50 × nível.
- A tela de fim mostra andar, nível, inimigos destruídos, engrenagens, pontuação, semente e causa da morte.
- Os 10 melhores ficam em `%APPDATA%\Roguelike\placar.txt`.

## 8. Telas, HUD e opções

- **Telas:** menu principal (Novo jogo, Placar, Opções, Sair) → jogo → pausa / inventário → fim de partida.
- **HUD:** manômetro de vida (barra estilizada de latão), nível e barra de XP, arma na mão (sprite grande no centro
  embaixo, com animação de golpe, disparo e recarga), carga da arma e munição de reserva, engrenagens, andar atual,
  mensagens curtas ("Inventário cheio", "Comporta trancada", "Sem munição").
- **Texto:** fonte bitmap pixel art (atlas de caracteres), com acentos do português.
- **Opções** (em `%APPDATA%\Roguelike\config.txt`): sensibilidade do mouse, volume de efeitos, volume da música,
  tela cheia ou janela, semente fixa (vazio = aleatória).

## 9. Som

Efeitos: passos em metal e pedra, golpe no ar, acerto, disparo de cada arma de fogo, recarga, clique de arma vazia,
dano no jogador, inimigo destruído, comporta abrindo (vapor), pegar item, subir de nível, elevador descendo.
Uma música ambiente industrial em loop. Tudo de pacotes livres, registrado em `assets\LICENCAS.txt`.

## 10. Arte

Fontes previstas (pedir ok antes de cada download): OpenGameArt.org e Kenney.nl, com preferência por CC0.
Necessário: texturas das 3 zonas (parede, chão, teto, comporta; 64×64); sprites dos 4 inimigos com animação;
sprites de itens e munição; sprites das 6 armas na mão; ícones do inventário; fonte bitmap.

**Risco conhecido:** pixel art steampunk gratuita é mais rara que a medieval; provavelmente será preciso misturar
pacotes de autores diferentes e ajustar cores para combinar. Onde faltar sprite, usar um placeholder gerado
(silhueta colorida), avisar o Nidragnnar e registrar no `LICENCAS.txt` o que falta.

## 11. Erros e robustez

- Arquivo de arte/som ausente ou inválido: textura xadrez rosa ou silêncio + linha em `log.txt` (ao lado do `.exe`). O jogo não fecha.
- Sem OpenGL 3.3: caixa de mensagem do Windows explicando que a placa de vídeo/driver não é suportado.
- `placar.txt` ou `config.txt` corrompido: renomear para `.corrompido` e recriar com os valores padrão.
- A janela perde o foco: pausa automática e solta o mouse.

## 12. Testes

- **Automáticos (doctest, `testes\`):** conectividade do gerador para 1.000 sementes; elevador e válvula-chave alcançáveis;
  determinismo pela semente; fórmulas de dano, XP e subida de nível; munição (gastar, recarregar, máximo, sem munição);
  tiro instantâneo contra parede e inimigo; inventário (empilhar, cheio, equipar/trocar); colisão contra parede e quina;
  caminho na grade.
- **Roteiro de capturas:** `Roguelike.exe --demo <semente>` anda sozinho por um caminho até o elevador,
  salva prints em `capturas\` e fecha. Usado pelo Claude para conferir o desenho a cada etapa.
- **Cenários para o Nidragnnar:** sementes documentadas (comporta trancada, sala cheia, andar 5 com obra-prima)
  e a opção `--andar N` para começar direto num andar, com uma tabela do que testar em cada um.

## 13. Entrega por etapas

Cada etapa termina jogável, compilada e testada pelo Claude; o Nidragnnar testa antes de qualquer commit.

1. **Janela e masmorra fixa:** projeto CMake, SDL2 + OpenGL, framebuffer 480×270, masmorra escrita à mão em 3D
   com texturas de esgoto, andar com colisão, neblina e lanterna.
2. **Masmorra procedural:** gerador com semente, 3 zonas, comportas, comporta trancada e válvula-chave,
   elevador para o próximo andar, automapa.
3. **Inimigos e combate básico:** 4 inimigos, linha de visão, perseguição, chave inglesa e revólver com recarga,
   tiros dos Atiradores, dano, morte permanente, tela de fim e placar.
4. **Itens e progressão:** demais armas brancas e de fogo, munição no chão, itens soltos por inimigos,
   qualidade e modificações, inventário, XP e níveis, HUD completo.
5. **Acabamento e pacote:** sons e música, menus e opções, `--demo`, `.zip` com `Roguelike.exe` + DLLs + `assets\`
   + `COMO_JOGAR.txt` (guia simples para amigos).

## 14. Distribuição

Pasta `Roguelike\` com `Roguelike.exe`, `SDL2.dll`, `SDL2_mixer.dll`, `assets\`, `COMO_JOGAR.txt` e `LICENCAS.txt`,
compactada em `Roguelike-v1.zip`. Compilado em Release, sem dependência de instalar Visual C++ Redistributable
(runtime estático `/MT`). Publicação (GitHub Release etc.) só com o ok do Nidragnnar.
