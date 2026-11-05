# Etapa 6B — Inimigos voadores: Especificação de design

Data: 2026-11-05 · Autor: Nidragnnar · Status: aguardando revisão
Complementa a especificação geral (`2026-11-01-roguelike-design.md`, seção 13, item 6) e a da 6A
(`2026-11-03-etapa6a-terreno-alturas-design.md`), que já deu ao andar chão e teto com alturas, pulo e combate em 3D.

## 1. Objetivo

Pôr no ar dois inimigos que usam a altura do andar: uma vespa a corda que mergulha para morder e um dirigível a vapor
que paira perto do teto e solta bombas.

**Critério de sucesso:** a partir do andar 2 aparecem vespas, a partir do 3 dirigíveis; os dois voam por cima de
poços, caixas e degraus, desviam de paredes e comportas fechadas e nunca atravessam o teto; subir numa plataforma
não protege o jogador deles; a chave inglesa só acerta a vespa quando ela desce, tiros acertam os dois; a bomba é
visível caindo e dá para fugir da explosão; o jogo mantém 60 quadros por segundo.

### Decisões tomadas

| Tema | Decisão |
|---|---|
| Quantos | **Dois tipos**: vespa a corda (corpo a corpo) e dirigível a vapor (bombas) |
| Voo | **Livre sob o teto**: ignoram a altura do chão; só paredes e comportas fechadas barram |
| Ataque do alto | **Bombas** que caem com gravidade e explodem em área |
| Andares | Vespa a partir do **2**, dirigível a partir do **3**, dentro do total sorteado |
| Arte e som | **Gerados por roteiro** (provisórios), sem download novo |
| Abordagem | **A**: inimigo comum com a marca "voa"; busca em largura própria; altura controlada à parte |

## 2. Os dois voadores

Valores do andar 1; vida e dano escalam com o andar como os demais (`× (1 + 0,15 × (andar − 1))`).

| | Vespa a corda | Dirigível a vapor |
|---|---|---|
| Vida / defesa | 6 / 0 | 18 / 1 |
| Dano | 2–4 (mordida) | 5–8 (bomba, no centro da explosão) |
| Velocidade | 3,0 células/s | 1,0 célula/s |
| XP | 5 | 12 |
| Preparo / recuperação | 0,3 s / 0,8 s | — (intervalo entre bombas 2,5 s) |
| Altura do corpo | 0,30 | 0,60 |
| Altura de voo (dos pés) | pés do jogador + 1,2 | teto − 1,2 (topo do corpo a 0,6 do teto) |

- **Vespa:** persegue no alto. A 1,5 célula do jogador (na horizontal), vendo-o, entra em *Preparando*: desce até os
  pés do jogador + 0,3 (altura do peito). No fim do preparo, morde se a distância 3D entre os centros for
  ≤ `ALCANCE_GOLPE` (1,1). Em *Recuperando*, sobe de volta à altura de voo.
- **Dirigível:** persegue tentando ficar em cima do jogador. Vendo o jogador e a até 2 células na horizontal, solta
  uma bomba se já passaram 2,5 s da última. Não tem golpe corpo a corpo.
- **Destruídos:** passam a ter gravidade (sem dano de queda) e os destroços caem até o chão. Recompensa como os
  outros: XP e a mesma chance de item (25%) e munição (35%).
- **Sorteio por andar** (pesos; a soma não precisa dar 100):

| Andar | Rato | Autômato | Atirador | Colosso | Vespa | Dirigível |
|---|---|---|---|---|---|---|
| 1 | 60 | 40 | — | — | — | — |
| 2 | 35 | 30 | 20 | — | 15 | — |
| 3+ | 25 | 25 | 20 | 12 | 10 | 8 |

  Vespas vêm sozinhas ou em dupla (1–2 na mesma sala, sem passar do total do andar). O total por andar não muda.
- **Mapas de teste:** `v` vespa, `d` dirigível.

## 3. Voo

- **Caminho:** nova regra de passagem `VOADOR` (subida e descida ilimitadas) em `distanciasComAltura`; só paredes,
  comportas fechadas e o elevador barram. A Partida mantém um terceiro campo de distâncias (`campoVoador_`),
  recalculado como os outros, **só quando há algum voador vivo** no andar.
- **Altura:** sem gravidade enquanto vivo. `corpo.pes` anda em direção a um alvo a até **2,5 blocos/s**. O alvo é a
  altura de voo do estado atual, limitada a:
  - no mínimo, o chão mais alto entre a célula atual e a próxima do caminho, + 0,1;
  - no máximo, o teto mais baixo entre essas duas células, − altura do corpo − 0,1.
  Se o mínimo passar do máximo, vale o mínimo (a célula seguinte é baixa demais e a colisão o segura até descer).
- **Movimento horizontal:** `moverComColisao` com os pés na altura de voo e subida 0. Um chão mais alto que o corpo
  barra até ele subir; teto baixo barra pela regra de hoje.
- **Visão:** `temLinhaDeVisao3D` dos olhos do voador ao jogador, **sem** a condição de mesmo nível que vale para os
  inimigos de chão.
- **Empurrão entre inimigos e contra o jogador:** a regra de hoje (só quem está no mesmo nível se empurra).

## 4. Bomba

- `Projetil` ganha `gravidade` (0 nos tiros; 9 nas bombas) e `raioExplosao` (0 nos tiros; 1,0 nas bombas).
- A bomba nasce sob o dirigível com velocidade zero e cai acelerando. Desenhada piscando.
- Explode ao tocar o chão, a parede, o teto ou o jogador. Dano no jogador: `dano × (1 − d / raio)`, arredondado, se
  `d < raio`, com `d` a distância 3D do ponto da explosão ao centro do jogador; atravessa paredes; não fere inimigos.
  A defesa vale como em qualquer golpe.
- `ResultadoProjeteis` ganha a lista de explosões (posições), usada pelo som e por um clarão curto.

## 5. Desenho e som

- `ferramentas/gerar_voadores.ps1` desenha os sprites: vespa (2 quadros de asa), dirigível, destroços dos dois,
  bomba e explosão. Provisórios, como o lampião e o lustre.
- Sons sintetizados por `gerar_sons` e preparados por `preparar_sons.ps1`: zumbido da vespa, motor do dirigível
  (tocados uma vez quando o voador percebe o jogador, para avisar que ele vem do alto) e explosão.
- `assets/LICENCAS.txt` atualizado.

## 6. Testes

Novos em `teste_voadores` e acréscimos nos existentes:
- voador atravessa poço, caixa e degrau; parede e comporta fechada o barram;
- a altura nunca passa do teto nem fica abaixo do chão, inclusive num corredor baixo;
- vespa: mergulha, morde e sobe; chave inglesa não a acerta no alto e acerta quando ela desce; tiro acerta no alto;
- dirigível: solta bomba com o intervalo certo; não solta sem ver o jogador;
- bomba: cai com gravidade, explode no chão, dano diminui com a distância, nada fora do raio;
- voador destruído cai até o chão;
- sorteio: nenhuma vespa no andar 1, nenhum dirigível antes do 3; o desenho do andar não muda (impressão digital);
- o tiro do Atirador continua reto (gravidade 0).

**Cenários prontos** em `build\Release`: `14_Vespas_sobre_poco.bat` (minas) e `15_Dirigivel_sala_alta.bat`
(fábrica), com uma tabela do que olhar em cada um. Para pôr os voadores no lugar certo, o jogo ganha a opção de
teste `--inimigo <letra> <x> <y>`.

## 7. Fora da 6B

Enxames, voadores que atravessam paredes baixas, bombas que ferem inimigos, sombra da bomba no chão e arte definitiva.
