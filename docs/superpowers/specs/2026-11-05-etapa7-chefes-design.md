# Etapa 7 — Chefes e vitória: Especificação de design

Data: 2026-11-05 · Autor: Nidragnnar · Status: aguardando revisão
Complementa a especificação geral (`2026-11-01-roguelike-design.md`, "Fora da v1": chefes) e as da Etapa 6
(`2026-11-03-etapa6a-terreno-alturas-design.md`, `2026-11-05-etapa6b-voadores-design.md`).

## 1. Objetivo

Dar ao jogo começo, meio e fim: um chefe no fim de cada zona e a vitória ao derrotar o último.

**Critério de sucesso:** nos andares 2, 4 e 6 o elevador fica numa arena com um chefe; ao entrar, a arena tranca,
toca a música de chefe e aparece a barra de vida; cada chefe tem um padrão de ataque próprio que dá para aprender e
vencer; derrotá-lo libera o elevador e solta uma obra-prima; derrotar a Grande Máquina (andar 6) mostra a tela de
vitória e grava "Venceu a Grande Máquina" no placar; os andares sem chefe continuam iguais; o jogo mantém 60 quadros
por segundo.

### Decisões tomadas

| Tema | Decisão |
|---|---|
| Fim do jogo | **3 chefes e vitória**: andares 2 (esgoto), 4 (fábrica) e 6 (minas). Não existe andar 7 |
| Andar do chefe | **Andar normal + arena**: o elevador fica na arena com o chefe |
| Chefes | **3 chefes distintos**: Rainha dos Ratos, Capataz a Vapor e Grande Máquina |
| Tela e recompensa | Barra de vida do chefe, item garantido, tela de vitória e placar, música de chefe |
| Abordagem | **A**: chefe é um `Inimigo` com a marca `chefe`; lógica de cada um em `jogo/Chefes.cpp` |
| Arte e som | Gerados por roteiro (provisórios), sem download |

## 2. Os três chefes

Valores finais (não passam pela escala por andar). Vida/defesa/dano são estimativas para acertar jogando.

| | Rainha dos Ratos | Capataz a Vapor | Grande Máquina |
|---|---|---|---|
| Andar / zona | 2 / esgoto | 4 / fábrica | 6 / minas |
| Vida / defesa | 140 / 0 | 240 / 3 | 420 / 2 |
| Velocidade | 2,5 | 1,0 | 0 (fixa no centro) |
| Raio do corpo / altura | 0,5 / 0,8 | 0,6 / 1,4 | 0,8 / 1,8 |
| XP | 60 | 120 | 250 |

- **Rainha dos Ratos**
  - *Perseguição:* anda até o jogador como os inimigos de chão.
  - *Investida* (vendo o jogador a até 6 células, no máximo uma a cada 3 s): para 0,6 s mirando (Preparando) e
    corre em linha reta a 7 células/s na direção fixada, por até 1,2 s. Encostar no jogador causa 7–10 e encerra a
    investida. Bater na parede a deixa **tonta** 1,5 s (Recuperando), sem atacar.
  - *Ninhada:* a cada 8 s chama 3 ratos ao lado dela, contanto que não haja mais de 6 ratos vivos no andar.
- **Capataz a Vapor**
  - *Perseguição* lenta.
  - *Jato de vapor* (vendo o jogador a até 7 células, a cada 3 s): para 0,5 s e solta um leque de 5 projéteis
    (−20°, −10°, 0°, +10°, +20°), 4–6 cada, velocidade dos projéteis de hoje.
  - *Pancada no chão* (jogador a até 2,5 células, a cada 5 s): prepara 0,8 s e solta uma **onda** de raio 2,5 a
    partir dele: 9–13 no jogador que estiver **no chão** e dentro do raio; quem estiver no ar (pulando) não leva.
- **Grande Máquina**
  - *Fixa* no centro da arena; não é empurrada.
  - *Canhões:* vendo o jogador, uma rajada de 3 tiros (0,2 s entre eles), 4–6 cada, a cada 2 s.
  - *Fase 2* (vida ≤ 50%): rajadas a cada 1,2 s e, a cada 12 s, chama 2 vespas ou 1 dirigível (alternando), contanto
    que não haja mais de 4 voadores vivos no andar.
- **Todos:** recompensa = XP da tabela + 1 obra-prima (arma ou armadura, sorteio do andar) + 30 engrenagens +
  munição. Os inimigos chamados dão a recompensa normal.

## 3. Arena

- `bool andarDeChefe(int andar)`: andares 2, 4 e 6.
- Nesses andares o gerador põe o elevador na **maior sala** (fora a sala inicial) e a marca como arena
  (`Andar::arena`, índice da sala). O relevo deixa a arena **plana** (sem plataforma, poço nem caixas, nível da sala)
  e com **teto alto** (nível + 16 quartos, até o máximo do andar).
- O chefe nasce no centro da arena, Parado. Os inimigos comuns não nascem na arena.
- **Luta começa** quando o jogador entra no interior da arena: todas as comportas nas aberturas da arena viram
  trancadas (e fechadas); o chefe passa a Perseguindo; a música troca para a de chefe; a barra aparece.
- **Luta termina** quando o chefe é destruído: as comportas voltam a comportas comuns (fechadas), a música volta, a
  barra some; o elevador funciona.
- Com o chefe vivo, usar o elevador mostra "Derrote o chefe primeiro".
- Morrer na arena termina a partida como hoje.
- Os andares 1, 3 e 5 não mudam (impressão digital do gerador mantida); os andares 2, 4 e 6 mudam onde está o
  elevador.

## 4. Vitória

- Quando a Grande Máquina é destruída: depois de 3 s (para ver a explosão), a partida acaba como **vitória**.
- A tela de fim mostra "Vitória!", o tempo de jogo (minutos:segundos) e a pontuação.
- **Pontuação:** a da spec geral + 500 por chefe derrotado.
- **Placar:** causa "Venceu a Grande Máquina"; o formato do arquivo não muda (o placar atual continua valendo).

## 5. Interfaces novas

- `DadosInimigo` ganha `bool chefe` e `float raio` (0,25 nos inimigos de hoje). Colisão, separação, armas e
  projéteis usam o raio de cada inimigo no lugar de `RAIO_INIMIGO`.
- `EventosIA` ganha `invocacoes` (tipo e posição) e `ondas` (centro, raio, dano).
- `Partida` ganha `chefe()` (o chefe vivo do andar ou nada), `lutaComChefe()`, `venceu()`, `tempoDeJogo()`.
- Os tiros dos chefes são projéteis comuns (autor = o chefe).

## 6. Desenho e som

- `ferramentas/gerar_chefes.ps1`: 7 quadros de cada chefe (parado, andar 1–2, atacar 1–2, morrer, destroços).
- HUD: barra larga no alto com o nome do chefe e a vida, só durante a luta.
- `gerar_sons`: `musica_chefe` (32 s, em loop, mais rápida e tensa que a atual); `Audio` toca a música pedida.
- `assets/LICENCAS.txt` atualizado.

## 7. Testes

- Gerador: andares 2/4/6 com o elevador na maior sala fora a inicial; arena plana; nenhum inimigo comum na arena;
  andares 1/3/5 com a mesma impressão digital.
- Arena: entrar tranca as comportas e começa a luta; destruir o chefe destranca; elevador bloqueado com o chefe vivo.
- Rainha: investida em linha reta com dano ao encostar; tonta ao bater na parede; nunca mais de 6 ratos vivos.
- Capataz: leque de 5 projéteis com os ângulos certos; onda fere no chão e não fere no ar; não fere fora do raio.
- Grande Máquina: não sai do lugar; rajada de 3; fase 2 com ≤ 50%; nunca mais de 4 voadores vivos.
- Raio do corpo: tiro e golpe acertam o chefe pela borda do corpo.
- Vitória: tela, causa no placar, bônus de 500 por chefe.

**Cenários prontos** em `build\Release`: `16_Rainha_dos_Ratos.bat`, `17_Capataz_a_Vapor.bat`,
`18_Grande_Maquina.bat` (de frente para cada arena) e `19_Vitoria.bat` (Grande Máquina quase sem vida), com a tabela
do que olhar em cada um.

## 8. Fora da Etapa 7

Modo infinito depois da vitória, chefes em outros andares, falas/cenas, arte e música definitivas, conquistas.
