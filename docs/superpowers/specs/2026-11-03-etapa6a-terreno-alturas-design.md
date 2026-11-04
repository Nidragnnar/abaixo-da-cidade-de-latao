# Etapa 6A — Terreno com alturas, pulo e iluminação: Especificação de design

Data: 2026-11-03 · Autor: Nidragnnar · Status: aguardando revisão
Complementa a especificação geral (`2026-11-01-roguelike-design.md`, seção 13, item 6). A Etapa 6 foi dividida em
**6A (terreno, pulo, combate em alturas)** e **6B (inimigos voadores)**; esta especificação cobre só a 6A.

## 1. Objetivo

Dar aos andares desníveis de verdade — degraus, rampas, plataformas, mezaninos, poços e caixas — com pulo e dano de
queda, inimigos que seguem o terreno e combate em 3D, sem perder o que já funciona na v1.

O Nidragnnar quer as quatro coisas que a verticalidade pode trazer: **combate em alturas**, **exploração e cenário**,
**pulo e plataformas** e **voadores** (estes na 6B).

**Critério de sucesso:** um andar gerado tem salas em níveis diferentes ligadas por rampas e escadas, plataformas,
poços e caixas; dá para atravessá-lo inteiro a pé até a válvula-chave e o elevador; pular abre atalhos e alcança
itens escondidos; inimigos sobem e descem atrás do jogador; tiros e visão respeitam as alturas; o jogo mantém
60 quadros por segundo.

### Decisões tomadas

| Tema | Decisão |
|---|---|
| Representação | **Grade com alturas**: cada célula tem altura de chão e de teto; rampas inclinadas nas 4 direções |
| Camadas | **Um chão por célula** (nada passa por baixo de passarela) |
| Desnível | **Moderado**: chão de 0 a 3 blocos, teto de até 5 blocos |
| Pulo | **Sim** (Espaço), ~1,1 bloco |
| Queda | **Dano em queda alta** (a partir de 2 blocos) |
| Inimigos de chão | Sobem só degraus e rampas e descem qualquer altura; **o Rato pula 1 bloco** |
| Caminho obrigatório | Sempre **a pé, sem pular e sem queda com dano** |

## 2. Terreno

- **Unidade de altura:** 1/4 de bloco (`QUARTO = 0,25`). Alturas guardadas como inteiros de quartos.
- **Por célula de chão:** `chao` de 0 a 12 quartos (0 a 3 blocos) e `teto` de até 20 quartos (5 blocos), com
  `teto ≥ chao + 5` quartos (1,25 bloco de espaço livre).
- **Rampa:** a célula pode ter uma direção (N, S, L, O). O chão sobe linearmente 4 quartos (1 bloco) ao longo dela,
  de `chao` na borda de entrada até `chao + 4` na borda de saída. Rampas nunca ficam em comportas nem no elevador.
- **Caixa:** célula de chão comum cujo chão é 4 quartos mais alto que o das vizinhas e o teto é o da sala.
- Paredes e comportas continuam como hoje (células sólidas do chão ao teto).
- **Mapas de teste em texto** ganham uma segunda grade opcional, de mesmo tamanho: dígito `0`–`9` = chão em quartos
  (`0` = nível base), letras `n s l o` = rampa subindo para norte, sul, leste, oeste a partir do chão `0` e
  `N S L O` = a mesma rampa a partir de 1 bloco (4 quartos), e uma terceira grade opcional para o teto (dígitos em
  quartos acima de 0; `0` = padrão, 4 quartos acima do chão). Sem essas grades, tudo fica no nível 0 com teto 4 quartos (o mapa de hoje).

## 3. Movimento do jogador

- `Jogador` ganha `altura` (dos pés) e `velocidadeVertical`. Olhos em `altura + 0,5`. Altura do corpo 0,7.
- **Gravidade** 24 blocos/s². **Pulo** (Espaço, evento): velocidade inicial que alcança ~1,1 bloco
  (`√(2 · 24 · 1,1) ≈ 7,27`), só com os pés no chão.
- **Degrau automático:** andando, sobe sozinho até 1 quarto (0,25). Na rampa, a altura acompanha o chão.
- **Passagem:** uma célula vizinha bloqueia o movimento horizontal se o chão dela estiver mais de 1 quarto acima dos
  pés, ou se o teto dela estiver abaixo de `pés + 0,7`. Pulando, os pés sobem e a borda deixa de bloquear.
- **Teto:** a cabeça no teto zera a velocidade para cima.
- **Queda:** medida do ponto mais alto do pulo até o pouso. Até 2 blocos, sem dano; acima, **5 de dano por meio bloco
  a mais** (arredondado para baixo: 2,5 → 5; 3 → 10; 4 → 20). O dano de queda ignora a defesa da armadura e conta
  como causa de morte "Caiu de muito alto".
- **Comandos:** `Comandos` ganha `pular` (evento). Tecla Espaço. Guia, README e tabela de controles atualizados.

## 4. Gerador

Acrescenta alturas ao gerador atual (salas, árvore de Prim, corredores extras, comportas, tranca, elevador), sem
mudar a conectividade 2D.

- **Nível de cada sala:** sorteado de 0 a 3 blocos (0 a 1 no andar 1); teto da sala 2 a 3 blocos acima do nível,
  limitado a 5.
- **Corredores entre níveis diferentes:** a diferença vira **rampas** (1 célula por bloco) ou **escadas**
  (degraus de 1 quarto, 4 células por bloco) quando o corredor tem comprimento; se faltar espaço, o nível da sala de
  destino é rebaixado até caber. Teto do corredor: 1,5 bloco acima do chão mais alto do trecho.
- **Comportas e elevador** só em trechos planos (mesma altura dos dois lados).
- **Detalhes nas salas** (por sorteio, conforme o tamanho):
  - **Plataforma/mezanino:** faixa de 1 bloco de altura e 1–2 células de largura junto a uma parede, com escada ou
    rampa; em parte delas só se sobe pulando, e então guarda um item.
  - **Poço:** depressão de 1 a 3 blocos no interior da sala, nunca na sala inicial nem no caminho obrigatório, sempre
    com uma escada para sair.
  - **Caixas:** 1 a 4 blocos de 1 bloco de altura, soltos ou em pilha de 2 (pilha só alcançável pulando de outra).
- **Personalidade das zonas:** esgoto quase plano (desnível de até 1 bloco, poucas caixas); fábrica com mezaninos e
  caixas; minas com poços e rampas.
- **Itens, munição e engrenagens** podem cair em plataformas e no fundo de poços; inimigos nascem em qualquer nível.
- **Garantias (testadas em 1.000 sementes):** caminho a pé (sem pulo, sem queda acima de 2 blocos) do início até a
  válvula-chave e o elevador; todo poço com saída a pé; todo teto ≥ chão + 1,25; comportas e elevador em trecho plano;
  andar 1 com desnível de até 1 bloco; determinismo pela semente.

## 5. Inimigos e IA

- `Inimigo` ganha `altura` e `velocidadeVertical`; segue degraus e rampas como o jogador e cai pela borda.
  **Inimigos não levam dano de queda.**
- **Campos de fluxo:** dois por recálculo — **andarilhos** (sobem só degrau de 1 quarto e rampa, descem qualquer
  altura) e **saltadores** (o Rato: sobe também 1 bloco de uma vez). A busca usa as mesmas regras de passagem do
  movimento.
- O Rato pula sozinho quando o próximo passo do campo exige subir mais de 1 quarto.
- Separação entre inimigos e com o jogador continua, só entre quem está no mesmo nível (diferença < 0,75).

## 6. Visão e combate em 3D

- **Linha de visão 3D:** o raio vai dos olhos de quem vê aos olhos (ou ao centro) do alvo; além das paredes, é barrado
  quando passa abaixo do chão ou acima do teto de alguma célula do caminho (bordas de plataforma, degraus e caixas
  escondem).
- **Tiros do jogador:** saem da altura real dos olhos com a inclinação da mira; acertam o primeiro inimigo cuja altura
  o raio cruza (a tolerância vertical de 4° continua) ou param no chão, no teto ou numa face de degrau.
- **Projéteis dos Atiradores:** ganham altura e velocidade vertical, apontados para o centro do jogador; somem ao bater
  em chão, teto, parede ou face de degrau.
- **Corpo a corpo** (jogador e inimigos): só acerta com diferença de altura dos pés menor que 0,75.
- **Pegar itens:** só com diferença de altura menor que 0,75; munição e engrenagens também.
- **Barulho** que acorda inimigos continua 2D (som atravessa níveis).

## 7. Desenho

- **Geometria:** chão e teto por célula nas suas alturas; faces verticais entre células de alturas diferentes (frente
  de degrau/plataforma embaixo, beiral de teto em cima); rampas como planos inclinados; paredes do menor chão ao maior
  teto da vizinhança. UV em unidades de mundo (a textura não estica; uma parede de 3 blocos repete 3 vezes).
- **Texturas novas:** caixa e degrau de escada, procuradas primeiro nos pacotes já baixados (Tiny Texture Pack e
  Zortch, CC0); na falta, provisórias por script. Registradas em `LICENCAS.txt`.
- **Sprites:** inimigos, itens, válvula, elevador e projéteis desenhados na sua altura.
- **Câmera:** olhos na altura dos pés + 0,5, com interpolação vertical entre passos.
- **Automapa:** tom da célula pela altura do chão (mais claro = mais alto), marca de seta em rampas e escadas,
  poços mais escuros.

## 8. Iluminação: lampiões nas salas e nos corredores

Pedido do Nidragnnar ao revisar esta especificação: o jogo está escuro demais (hoje só há a lanterna do jogador, de
alcance 6, e uma luz ambiente fraca por zona). Não depende das alturas; entra como **primeira tarefa** do plano.

- **Luzes fixas do andar**, geradas pela semente junto com o andar: `Luz { posição 3D, cor, alcance, fase }`.
  - **Salas:** 1 luz em salas pequenas, 2 a 3 nas grandes, presas a uma parede, 0,8 bloco acima do chão local.
  - **Corredores:** uma luz a cada 8 a 10 células de corredor, alternando os lados.
  - **Cor por zona:** esgoto amarelada, fábrica alaranjada (fornalha), minas branca-amarelada (lampião).
  - **Alcance** de 5 blocos, com atenuação suave até zero na borda; **tremulação** leve de chama (±10% de
    intensidade, ritmo próprio de cada luz).
- **No shader:** a cada quadro vão as **16 luzes mais próximas** da câmera (escolha pura e testada), somadas à
  lanterna e à luz ambiente; a neblina continua. Sem sombras (a luz pode vazar um pouco através de uma parede fina;
  o alcance curto limita isso).
- **Lampião visível:** sprite pequeno na posição de cada luz (arandela de latão com chama), desenhado sempre aceso
  (sem ser apagado pela neblina perto dele). Arte primeiro dos pacotes já baixados; na falta, provisória por script.
- **Testes:** toda sala tem pelo menos 1 luz; nenhum trecho de corredor com mais de 10 células sem luz; toda luz fica
  numa célula de chão encostada numa parede e acima do chão local; mesma semente, mesmas luzes; a escolha das 16 mais
  próximas devolve as certas e no máximo 16.
- **Desempenho:** 16 luzes por pixel na tela baixa de 480×270 é barato; confirmado com o `--demo` em Release.

## 9. Demo, testes e robustez

- **`--demo`:** o piloto segue o caminho a pé (sem pular) pelas mesmas regras de passagem; chegar ao elevador em
  50 sementes vira o teste do caminho obrigatório.
- **Testes automáticos novos:** movimento (degrau de 1/4 sim, de 1/2 não; pulo alcança 1 bloco e não 1,25; teto corta o
  pulo; rampa; queda de 2 sem dano e de 3 com 10), gerador (garantias da seção 4 em 1.000 sementes), IA (Autômato
  contorna pela rampa, Rato sobe 1 bloco), combate (tiro de cima acerta embaixo, borda esconde, corpo a corpo não
  alcança 1 bloco acima, projétil bate no degrau), demo.
- **Compatibilidade:** placar e `config.txt` sem mudança de formato. Todos os testes atuais continuam valendo (mapas
  sem camada de altura ficam no nível 0).
- **Desempenho:** 60 quadros por segundo medidos com o `--demo` em Release.

## 10. Entrega

- Cenários prontos (`.bat`): andar com poço, sala com mezanino, Atirador no alto, plataforma só por pulo, salas e
  corredores iluminados; tabela do que
  testar.
- Commit só com o "sim" do Nidragnnar; depois, se ele quiser, Release v1.1 no GitHub.
- **Fora da 6A:** inimigos voadores e projéteis voadores deles (6B), passarelas sobre caminhos, água, elevadores
  internos de plataforma, agachar.
