# Dificuldade, trocar teclas e controle — Especificação de design

Data: 2026-11-07 · Autor: Nidragnnar · Status: aguardando revisão
Complementa a especificação geral (`2026-11-01-roguelike-design.md`, seção 8: opções) e a do salvamento
(`2026-11-07-salvar-partida-design.md`).

## 1. Objetivo

Deixar o jogo mais fácil de dividir com amigos que jogam menos ou de outro jeito: escolher a dificuldade, trocar as
teclas e jogar com um controle de videogame.

**Critério de sucesso:** nas Opções dá para escolher Fácil, Normal, Difícil ou Pesadelo, e a próxima partida usa
essa dificuldade, que aparece no placar; toda ação do jogo pode ir para outra tecla ou botão do mouse, e isso fica
gravado; com um controle ligado dá para jogar uma partida inteira, menus e inventário inclusive, sem tocar no mouse
nem no teclado. O Normal com as teclas de fábrica é exatamente o jogo de hoje.

### Decisões tomadas

| Tema | Decisão |
|---|---|
| Dificuldade | 4 níveis; muda só **vida e dano dos inimigos** (chefes inclusive) |
| Quando vale | Escolhida nas Opções, **vale no próximo Novo jogo** e fica fixa na partida (vai no salvamento) |
| Placar | **Um placar**, cada linha com a dificuldade; pontuação multiplicada pela dificuldade |
| Teclas | **Todas as ações do jogo** trocáveis, por tecla ou **botão do mouse**; conflito **troca as duas** |
| Controle | **Tudo no controle**, inventário com cursor; **esquema fixo**; sensibilidade própria e eixo invertível |
| Abordagem | **A**: camada de ações entre a entrada e o jogo; teclado, mouse e controle alimentam as mesmas ações |
| Entrega | Três partes testáveis em ordem: dificuldade → teclas → controle |

## 2. Dificuldade

| Nível | Vida e dano dos inimigos | Pontuação | Marca no placar |
|---|---|---|---|
| Fácil | × 0,6 | × 0,5 | F |
| Normal | × 1 (o jogo de hoje) | × 1 | N |
| Difícil | × 1,4 | × 1,5 | D |
| Pesadelo | × 1,8 | × 2 | P |

- Vale para todos os inimigos e os três chefes; vida e dano arredondados, nunca abaixo de 1.
- *Os multiplicadores são estimativa inicial; o Nidragnnar ajusta depois de jogar.*
- **Opções:** item "Dificuldade" (← → muda). Mudar no meio da partida não afeta a partida atual; a tela mostra
  "vale na próxima partida" quando há partida em andamento.
- **Config:** `dificuldade=normal` (`facil`, `normal`, `dificil`, `pesadelo`).
- **Salvamento:** passa a `versao 2`, com a linha `dificuldade <n>`; um arquivo `versao 1` é lido como Normal.
- **Placar:** o registro ganha a dificuldade (linhas antigas = Normal); a tela do placar mostra a letra em cada linha;
  a pontuação final é `calcularPontuacao(...)` × multiplicador, arredondada.
- **Fora:** quantidade de inimigos, munição e tônicos não mudam.

## 3. Trocar teclas

**Ações trocáveis** (padrão entre parênteses): andar para frente (W), para trás (S), esquerda (A), direita (D),
pular (Espaço), usar (E), trocar arma (Q), recarregar (R), inventário (I), mapa (Tab), itens rápidos 1–4 (1, 2, 3, 4)
e atacar (botão esquerdo do mouse).

- **Fixas:** Esc (pausa, fechar), as teclas dos menus (setas, W/S/A/D nos menus, Enter, Esc, Backspace) e a roda do
  mouse (troca de arma, além da tecla). O mouse continua mirando.
- **Tela "Teclas…"** (item novo nas Opções): uma linha por ação, com o nome da tecla atual; Enter (ou clique) numa
  linha mostra "Aperte a tecla ou o botão (Esc cancela)"; a próxima tecla ou botão do mouse (esquerdo, direito, meio,
  laterais 1 e 2) vira a da ação. No fim: "Restaurar padrão" e "Voltar".
- **Conflito:** se a tecla já é de outra ação, as duas trocam; nenhuma ação fica sem tecla.
- **Não aceitas:** Esc (cancela) e a roda do mouse.
- **Config:** uma linha por ação, `tecla_<acao>=<nome>`, com nomes próprios do jogo, sem acento (`w`, `espaco`,
  `shift_esq`, `f1`, `mouse1`…`mouse5`); a tela mostra nomes legíveis ("Espaço", "Shift esq.", "Botão esquerdo").
  Teclas fora da tabela de nomes (raras, de teclados especiais) não são aceitas na troca. **Nome inválido ou repetido** volta ao padrão só daquela ação (sem estragar o resto
  do config, ao contrário das outras opções); ausente = padrão.
- **Textos que citam teclas** usam a tecla atual: "Inventário (I fecha)", "Sem munição — R para recarregar",
  "Enter: nova partida   Esc: menu" (este é de menu, fica fixo), dicas do HUD.
- `COMO_JOGAR.txt` e README explicam as teclas de fábrica e onde trocá-las.

## 4. Controle de videogame

Pela API de controles do SDL (Xbox, PlayStation, genéricos com mapeamento conhecido). Ligar ou desligar com o jogo
aberto funciona; vale o primeiro controle ligado. Teclado e mouse continuam funcionando juntos.

| Controle (Xbox / PlayStation) | No jogo | Nos menus |
|---|---|---|
| Analógico esquerdo | andar (proporcional) | mover seleção |
| Analógico direito | olhar (mira) | — |
| RT / R2 | atacar | — |
| A / ✕ | pular | confirmar |
| X / □ | usar | — |
| B / ○ | recarregar | voltar |
| Y / △ | trocar arma | — |
| Cruz | itens rápidos 1 (cima), 2 (direita), 3 (baixo), 4 (esquerda) | mover seleção e ajustar (← →) |
| View / Select | inventário | — |
| LB / L1 | mapa | — |
| Start / Options | pausa | voltar ao jogo (na pausa) |

- **Zona morta** de 20% nos analógicos; a mira gira proporcional à inclinação, com aceleração leve nas pontas.
- **Inventário:** um cursor na tela, movido pelo analógico esquerdo (ou direito); A = clique esquerdo (pegar, soltar,
  arrastar), X = clique direito (largar no chão; o clique esquerdo usa ou equipa), B ou View = fechar. O cursor do controle e o mouse são o mesmo:
  quem mexer por último manda.
- **Opções:** "Sensibilidade do controle" (1–10, padrão 5) e "Inverter eixo vertical" (Não/Sim); tela "Controle…"
  só mostra o esquema acima (não troca botões).
- **Config:** `sensibilidade_controle=5`, `inverter_vertical=0`.
- **Textos:** quando a última entrada veio do controle, os textos que citam teclas mostram o botão ("B para
  recarregar", "Inventário (View fecha)").
- **Fora:** vibração, trocar botões do controle, dois controles ao mesmo tempo.

## 5. Arquitetura

- **`jogo::Dificuldade`** (enum + multiplicadores): pura; aplicada ao criar os inimigos (`Partida` guarda a
  dificuldade com que começou).
- **`jogo::Atalhos`**: ação → tecla/botão (`Entrada` física: scancode ou botão do mouse), com padrão, troca com
  conflito, nome para mostrar e leitura/gravação nas linhas do config. Puro, sem SDL além dos números de scancode.
- **`ui::Controle`** (ou equivalente): traduz o estado de um controle (eixos, botões, já lido do SDL) em `Comandos`
  do jogo, `EntradaMenu` e movimento do cursor do inventário. Pura, testável.
- **`plataforma::Entrada`**: lê o SDL (teclado, mouse, controle), consulta `Atalhos` e junta as fontes; guarda qual
  foi a última usada (teclado/mouse ou controle) para os textos.
- **Menus:** Opções ganha "Dificuldade", "Sensibilidade do controle", "Inverter eixo vertical", "Teclas…" e
  "Controle…"; duas telas novas (Teclas, Controle). A tela de Teclas tem o estado "esperando tecla".

## 6. Erros e casos estranhos

- Config com tecla inválida: só aquela ação volta ao padrão; o resto do config é aproveitado.
- Controle desligado no meio da partida: os comandos dele zeram (o jogador para), sem pausar sozinho.
- Duas ações na mesma tecla vindas de um config editado à mão: a segunda volta ao padrão (se o padrão também estiver
  ocupado, troca).
- Esperando tecla e a janela perde o foco: cancela a espera.
- Salvamento `versao 1`: lido como Normal; `versao 2` com dificuldade desconhecida: arquivo estragado (regra atual).

## 7. Testes

- **Automáticos:** multiplicadores e arredondamento; pontuação por dificuldade e placar com a letra (e linhas
  antigas); salvamento v1 → Normal e ida e volta da dificuldade; atalhos (padrão, troca com conflito, nome,
  config com tecla inválida e repetida); menus novos (dificuldade, tela de teclas esperando e cancelando, restaurar);
  tradução do controle em comandos (zona morta, inversão, cruz → itens) e cursor do inventário; textos com a tecla
  atual.
- **Do Nidragnnar:** o controle de verdade (não há como apertar botões aqui), a sensação de cada dificuldade e a
  troca de teclas na tela. Cenários prontos: uma partida em cada dificuldade no mesmo andar.
