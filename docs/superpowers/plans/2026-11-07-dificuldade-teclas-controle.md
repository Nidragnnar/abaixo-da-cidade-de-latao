# Dificuldade, trocar teclas e controle — Plano de implementação

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** quatro dificuldades, todas as teclas do jogo trocáveis (teclado ou mouse) e jogo inteiro no controle de
videogame.

**Architecture:** uma camada de ações (`jogo::AcaoJogo` + `jogo::Atalhos`) fica entre a entrada e o jogo; teclado,
mouse e controle alimentam os mesmos `Comandos`/`EntradaMenu`. Tudo que decide (multiplicadores, atalhos, tradução
do controle, menus) é puro no `nucleo` e testado; `plataforma::Entrada` só lê o SDL e junta as fontes.

**Tech Stack:** C++20, SDL2 2.32 (`SDL_GameController`), doctest; nada novo.

**Spec:** `docs/superpowers/specs/2026-11-07-dificuldade-teclas-controle-design.md`

## Global Constraints

- O Normal com as teclas de fábrica é exatamente o jogo de hoje (todos os testes atuais continuam passando).
- Multiplicadores de vida e dano: Fácil 0,6 · Normal 1 · Difícil 1,4 · Pesadelo 1,8; pontuação 0,5 · 1 · 1,5 · 2;
  letras F · N · D · P; arredondar com `std::lround`, nunca abaixo de 1.
- Config: `dificuldade=facil|normal|dificil|pesadelo`, `sensibilidade_controle=1..10` (padrão 5),
  `inverter_vertical=0|1`, `tecla_<acao>=<id>`; tecla inválida ou repetida volta ao padrão **só daquela ação**.
- Fixos: Esc, teclas dos menus (setas, W/S/A/D, Enter, Esc, Backspace), roda do mouse. Esc e roda não são aceitos na
  troca.
- Controle: zona morta 20%; esquema fixo da tabela da spec (seção 4); sem vibração, sem dois controles.
- `nucleo` continua sem SDL (os números de scancode do SDL são constantes da tabela USB, copiados na tabela de nomes).
- Comentários curtos em português, mesma densidade do código atual. Compilar sempre em Release.
- **Sem commit por tarefa:** regra do Nidragnnar (commit e envio só com o "sim" dele, no fim). O progresso fica no
  ledger.
- Testes: `powershell -NoProfile -ExecutionPolicy Bypass -File ferramentas\compilar.ps1 release` (tudo) ou
  `build\Release\testes_roguelike.exe -tc="<nome>"` (um caso).

## Review Focus

1. **Listas longas na tela baixa 480×270:** Opções com 11 itens e Teclas com 18 linhas cabem sem sair da tela nem
   sobrepor (teste `listas longas cabem na tela`, Tarefa 3).
2. **O clique ou Enter que abre a espera de tecla não vira a tecla** da ação no mesmo quadro (teste
   `abrir a espera não captura a própria tecla`, Tarefa 5).
3. **Atacar em tecla do teclado ou em outro botão do mouse:** segurar continua atacando, e o clique que captura o
   mouse continua sem atacar (teste `apertada lê teclado e mouse`, Tarefa 4; `ignorarBotao_` vale para o botão do
   atacar, Tarefa 5).
4. **Config editado à mão com duas ações na mesma tecla e o padrão também ocupado:** nenhuma ação fica sem tecla
   nem com tecla repetida (teste `repetidas são corrigidas sem perder tecla`, Tarefa 4).
5. **Controle com analógico gasto (drift abaixo de 20%) ou desligado no meio:** o jogador fica parado e nenhum evento
   dispara (testes `zona morta ignora drift` e `controle desligado não comanda`, Tarefa 6).

---

### Task 1: Dificuldade no núcleo

**Files:**
- Create: `src/jogo/Dificuldade.h`
- Modify: `src/jogo/Inimigos.h/.cpp` (`criarInimigo`, `gerarInimigos`), `src/jogo/Partida.h/.cpp`
- Test: `testes/teste_inimigos.cpp`, `testes/teste_partida.cpp`

**Interfaces:**
- Produces:
  - `enum class Dificuldade : uint8_t { Facil, Normal, Dificil, Pesadelo };` `constexpr int TOTAL_DIFICULDADES = 4;`
  - `float multiplicadorInimigos(Dificuldade)`, `float multiplicadorPontos(Dificuldade)`, `char letraDa(Dificuldade)`,
    `std::string_view nomeDa(Dificuldade)` ("Fácil", "Normal", "Difícil", "Pesadelo"),
    `std::string_view idDa(Dificuldade)` ("facil"…), `std::optional<Dificuldade> dificuldadeDoId(std::string_view)`
  - `Inimigo criarInimigo(TipoInimigo, glm::vec2, int andar, Dificuldade d = Dificuldade::Normal)`
  - `std::vector<Inimigo> gerarInimigos(const mundo::Andar&, uint32_t semente, Dificuldade d = Dificuldade::Normal)`
  - `Partida(uint32_t semente, int andarInicial = 1, Dificuldade d = Dificuldade::Normal)`;
    `Dificuldade Partida::dificuldade() const`; `pontuacao()` já multiplicada

- [ ] **Step 1: Testes que falham**

```cpp
TEST_CASE("dificuldade multiplica vida e dano, nunca abaixo de 1") {
    const auto n = jogo::criarInimigo(jogo::TipoInimigo::Rato, {0, 0}, 3);
    const auto f = jogo::criarInimigo(jogo::TipoInimigo::Rato, {0, 0}, 3, jogo::Dificuldade::Facil);
    const auto p = jogo::criarInimigo(jogo::TipoInimigo::Rato, {0, 0}, 3, jogo::Dificuldade::Pesadelo);
    CHECK(f.vidaMaxima == std::max(1, (int)std::lround(n.vidaMaxima * 0.6)));
    CHECK(p.vidaMaxima == std::max(1, (int)std::lround(n.vidaMaxima * 1.8)));
    CHECK(p.danoMax == std::max(1, (int)std::lround(n.danoMax * 1.8)));
    CHECK(f.danoMin >= 1);
}
TEST_CASE("chefes também seguem a dificuldade") { /* Capataz: vida Difícil == lround(vida Normal * 1.4) */ }
TEST_CASE("partida guarda a dificuldade e multiplica a pontuação") {
    jogo::Partida n(5, 2), d(5, 2, jogo::Dificuldade::Dificil);
    CHECK(d.dificuldade() == jogo::Dificuldade::Dificil);
    CHECK(d.inimigos()[0].vidaMaxima > n.inimigos()[0].vidaMaxima);
    // mesma partida, mesmos números: pontuação × 1,5
    n.ganharXp(30); d.ganharXp(30);
    CHECK(d.pontuacao() == (int)std::lround(n.pontuacao() * 1.5));
}
TEST_CASE("descer mantém a dificuldade") { /* descer (comando ou usar no elevador) e conferir vida dos novos */ }
TEST_CASE("ids da dificuldade vão e voltam") { /* para as 4: dificuldadeDoId(idDa(d)) == d; "medio" → nullopt */ }
```

- [ ] **Step 2: Rodar** — Expected: não compila (`Dificuldade` não existe).
- [ ] **Step 3: Implementar.** `criarInimigo` aplica o multiplicador depois da escala do andar (chefes: sobre a vida
  e dano finais da tabela). `Partida` guarda `dificuldade_` e passa para `gerarInimigos` (construtor e `descer`) e
  para `acrescentarInimigo`. `pontuacao() = lround(calcularPontuacao(...) * multiplicadorPontos(d))`. O construtor de
  `mundo::Andar` (testes) fica Normal.
- [ ] **Step 4: Rodar tudo** — Expected: todos passam (os antigos inclusive).

### Task 2: Placar e salvamento com a dificuldade

**Files:**
- Modify: `src/jogo/Placar.h/.cpp`, `src/render/TelasMenu.cpp` (`construirPlacar`), `src/main.cpp:295`
  (registro), `src/jogo/Salvamento.h/.cpp`
- Test: `testes/teste_placar.cpp`, `testes/teste_salvamento.cpp`, `testes/teste_telas_menu.cpp`

**Interfaces:**
- Consumes: Task 1.
- Produces: `RegistroPlacar::dificuldade` (último campo, padrão Normal); `VERSAO_SALVAMENTO = 2`.

Formato do placar: `pontuação;andar;nível;inimigos;engrenagens;semente;<F|N|D|P>;causa` (7 `;`). Uma linha com 6 `;`
é do formato antigo (a causa nunca tem `;`) e vale Normal. Letra desconhecida = linha inválida (regra atual).
Salvamento: `versao 2` e linha `dificuldade <0–3>` logo depois de `partida`; `versao 1` (sem a linha) = Normal;
`versao 2` sem a linha ou com valor fora da faixa = estragado.

- [ ] **Step 1: Testes que falham**

```cpp
TEST_CASE("placar grava e lê a dificuldade") { /* registro Pesadelo → gravar → ler: dificuldade == Pesadelo */ }
TEST_CASE("placar antigo vale Normal") { /* arquivo "500;2;1;3;10;7;Morto por Rato\n" → 1 registro, Normal, causa certa */ }
TEST_CASE("placar com letra desconhecida é corrompido") { /* "...;7;X;causa" → estavaCorrompido */ }
TEST_CASE("tela do placar mostra a letra") { /* construirPlacar com um Difícil: algum texto contém " D " */ }
TEST_CASE("salvamento guarda a dificuldade") { /* Partida(3,2,Dificil) → escrever → ler: dificuldade Dificil */ }
TEST_CASE("salvamento da v1.4 é lido como Normal") { /* texto da v1 (escrever, trocar "versao 2" por "versao 1" e
                                                       tirar a linha dificuldade) → lê, Normal */ }
TEST_CASE("versao 2 sem dificuldade é estragado") { /* tirar a linha → lerPartida vazio */ }
```

- [ ] **Step 2: Rodar** — Expected: falham (campo e linha não existem).
- [ ] **Step 3: Implementar** os formatos acima; a tela do placar mostra
  `"{}. {} {}  andar {}  nível {}  {}"` com a letra depois da pontuação; `main.cpp` passa `p.dificuldade()`.
- [ ] **Step 4: Rodar tudo** — Expected: passam (o teste antigo do formato do salvamento, se conferir `versao 1`,
  muda para `versao 2`; ledger com Ruling se mudar).

### Task 3: Dificuldade nas opções, layout de listas longas e cenários

**Files:**
- Modify: `src/jogo/Config.h/.cpp`, `src/ui/Menus.h/.cpp`, `src/render/TelasMenu.cpp` (`layoutMenu`,
  `construirMenu`), `src/main.cpp` (`novaPartida`, opção `--dificuldade`), `docs/COMO_JOGAR.txt`, `README.md`
- Create: `build\Release\21_Facil.bat` … `24_Pesadelo.bat` (fora do git, como os outros cenários)
- Test: `testes/teste_config.cpp`, `testes/teste_menus.cpp`, `testes/teste_telas_menu.cpp`

**Interfaces:**
- Consumes: Task 1.
- Produces: `Config::dificuldade` (padrão Normal), `Config::sensibilidadeControle` (1–10, padrão 5),
  `Config::inverterVertical` (padrão falso); itens das Opções, todos já nesta tarefa (índices usados pelas Tarefas
  5 e 7): `0` sensibilidade do mouse, `1` efeitos, `2` música, `3` tela cheia, `4` semente, `5` dificuldade,
  `6` "Sensibilidade do controle: N", `7` "Inverter eixo vertical: sim/não", `8` "Teclas…", `9` "Controle…",
  `10` Voltar (`ITEM_DIFICULDADE = 5` … `ITEM_VOLTAR_OPCOES = 10`). Confirmar em `8` e `9` não faz nada até as
  Tarefas 5 e 7 abrirem as telas.

- [ ] **Step 1: Testes que falham**

```cpp
TEST_CASE("config: dificuldade vai e volta") { /* Pesadelo → gravar → ler == Pesadelo; ausente → Normal */ }
TEST_CASE("config: dificuldade inválida corrompe") { /* "dificuldade=medio" → estavaCorrompido (regra atual) */ }
TEST_CASE("opções: dificuldade muda com as setas e para nas pontas") {
    // seleciona o item 5: "Dificuldade: Normal"; ajustar(+1) → "Difícil", (+1) → "Pesadelo", (+1) fica; (−3) → "Fácil"
}
TEST_CASE("config: controle vai e volta") { /* sensibilidade_controle=8, inverter_vertical=1 → igual; 11 → corrompido */ }
TEST_CASE("opções do controle") { /* item 6 "Sensibilidade do controle: 5" ajusta 1–10; item 7 "Inverter eixo vertical: não" alterna */ }
TEST_CASE("opções na pausa avisam que vale na próxima partida") {
    // abrir Pausa → Opções: texto do item 5 termina com "(próxima partida)"; vindo do Principal, não
}
TEST_CASE("listas longas cabem na tela") {
    for (int n : {11, 18}) { auto r = render::layoutMenu(n, 480, 270);
        CHECK(r.back().y + r.back().altura <= 270 - 4);
        for (size_t i = 1; i < r.size(); ++i) CHECK(r[i].y >= r[i-1].y + r[i-1].altura); }
    // até 10 itens, a posição é a de hoje (Y_ITENS = 96, ENTRE_ITENS = 14)
}
```

- [ ] **Step 2: Rodar** — Expected: falham.
- [ ] **Step 3: Implementar.** Config lê/grava `dificuldade`, `sensibilidade_controle` e `inverter_vertical`
  (fora da faixa corrompe, regra atual). Menus: item "Dificuldade: <nome>" com ← → (Enter também
  avança, como os outros); com `origemOpcoes_ == Pausa`, acrescenta " (próxima partida)". `layoutMenu`: quando
  `Y_ITENS + n * ENTRE_ITENS` passa de `telaA - 24`, começa em `Y_TITULO + 20` e usa
  `entre = max(10, (telaA - 24 - inicio) / n)` e `altura = min(ALTURA_ITEM, entre)`; o painel e o aviso acompanham.
  `main.cpp`: `novaPartida` usa `config.dificuldade`; opção de teste `--dificuldade <id>` sobrepõe (sem gravar no
  config). Cenários 21–24: andar 3, semente 1, `--dados cenarios\dados_dificuldade`, cada um numa dificuldade.
  COMO_JOGAR e README: uma linha sobre a dificuldade e o placar.
- [ ] **Step 4: Rodar tudo** — Expected: passam. Captura da tela de Opções
  (`ferramentas\capturar.ps1 opcoes_dificuldade -Release --tela opcoes --dados <scratch>`) conferida a olho.

### Task 4: Atalhos no núcleo

**Files:**
- Create: `src/jogo/Atalhos.h/.cpp` (no `nucleo`, `CMakeLists.txt`)
- Modify: `src/jogo/Config.h/.cpp` (`Config::teclas`)
- Test: `testes/teste_atalhos.cpp` (novo, em `ARQUIVOS_TESTE`), `testes/teste_config.cpp`

**Interfaces:**
- Produces:
  - `enum class AcaoJogo : uint8_t { Frente, Tras, Esquerda, Direita, Pular, Usar, TrocarArma, Recarregar,
    Inventario, Mapa, Item1, Item2, Item3, Item4, Atacar };` `constexpr int TOTAL_ACOES = 15;`
  - Tecla física = `int`: scancode do SDL (1–511) ou `BOTAO_MOUSE + b` (`BOTAO_MOUSE = 1000`, b = 1 esquerdo,
    2 meio, 3 direito, 4 e 5 laterais, os números do SDL).
  - `struct Atalhos { std::array<int, TOTAL_ACOES> tecla; static Atalhos padrao(); int de(AcaoJogo) const;
    void definir(AcaoJogo, int tecla); std::optional<AcaoJogo> acaoDa(int tecla) const;
    void corrigirRepetidas(); }` — `definir` com tecla já usada por outra ação troca as duas.
  - `bool teclaAceita(int)`, `std::string_view nomeDaTecla(int)`, `std::string_view idDaTecla(int)`,
    `std::optional<int> teclaDoId(std::string_view)`, `std::string_view idDaAcao(AcaoJogo)` (`frente`, `tras`,
    `esquerda`, `direita`, `pular`, `usar`, `trocar_arma`, `recarregar`, `inventario`, `mapa`, `item1`…`item4`,
    `atacar`), `std::string_view nomeDaAcao(AcaoJogo)` ("Andar para frente", …, "Atacar").
  - `bool apertada(int tecla, const uint8_t* teclado, uint32_t botoesMouse)` (teclado do `SDL_GetKeyboardState`,
    máscara do `SDL_GetMouseState`: bit `b − 1`).
  - `std::string comTeclas(std::string_view texto, const std::function<std::string(AcaoJogo)>& nome)` — troca
    `{recarregar}`, `{inventario}` etc. (o id da ação entre chaves) pelo nome dado.

Tabela de nomes (scancodes USB do SDL): A–Z (4–29: `a`…`z`, "A"…"Z"), 1–0 (30–39), Enter 40 (`enter`), Backspace 42
(`backspace`, "Backspace"), Tab 43 (`tab`), Espaço 44 (`espaco`, "Espaço"), `- = [ ] \ ; ' ` , . /` (45–56, ids
`menos igual abre_colchete fecha_colchete barra_inv ponto_virgula aspas crase virgula ponto barra`), Caps 57 (`caps`),
F1–F12 (58–69), Insert 73, Home 74, PgUp 75, Delete 76, End 77, PgDn 78, setas 79–82 (`seta_direita`, `seta_esquerda`,
`seta_baixo`, `seta_cima`), teclado numérico 84–99 (`num_barra`… `num_0`, `num_ponto`), Ctrl/Shift/Alt esquerdos e
direitos 224–226 e 228–230 (`ctrl_esq`, "Ctrl esq."…), botões do mouse (`mouse1`…`mouse5`, "Botão esquerdo",
"Botão do meio", "Botão direito", "Botão lateral 1/2"). Esc (41) fica fora (não aceita).

Padrão: W S A D, Espaço, E, Q, R, I, Tab, 1 2 3 4, `BOTAO_MOUSE + 1`.

- [ ] **Step 1: Testes que falham**

```cpp
TEST_CASE("padrão é o jogo de hoje") { /* de(Frente)==26 (W), de(Pular)==44, de(Atacar)==BOTAO_MOUSE+1, de(Item4)==33 */ }
TEST_CASE("definir tecla já usada troca as duas") {
    auto a = jogo::Atalhos::padrao(); a.definir(AcaoJogo::Pular, 8 /* E */);
    CHECK(a.de(AcaoJogo::Pular) == 8); CHECK(a.de(AcaoJogo::Usar) == 44);
}
TEST_CASE("Esc e teclas fora da tabela não são aceitas") { /* teclaAceita(41) falso; (0) falso; (500) falso; (BOTAO_MOUSE+3) verdadeiro */ }
TEST_CASE("nomes e ids vão e voltam") { /* toda tecla aceita: teclaDoId(idDaTecla(t)) == t; nomeDaTecla(44) == "Espaço" */ }
TEST_CASE("repetidas são corrigidas sem perder tecla") {
    // tecla[Usar] = tecla[Pular] = 44 e tecla[Pular] padrão ocupado por outra: depois de corrigirRepetidas,
    // todas as 15 diferentes e nenhuma zero
}
TEST_CASE("apertada lê teclado e mouse") { /* teclado[44]=1 → apertada(44); máscara 0b100 → apertada(BOTAO_MOUSE+3) */ }
TEST_CASE("comTeclas troca o id pelo nome") { /* "Sem munição — {recarregar} para recarregar" com nome "R" */ }
TEST_CASE("config: teclas vão e voltam") { /* definir Pular=mouse4 → gravar → ler: igual; arquivo tem "tecla_pular=mouse4" */ }
TEST_CASE("config: tecla inválida volta ao padrão só dela") {
    // "sensibilidade=7\ntecla_pular=banana\n" → não corrompido, sensibilidade 7, Pular = 44
}
TEST_CASE("config: duas ações na mesma tecla") { /* tecla_usar=espaco → Usar 44 e Pular volta a ter tecla própria */ }
```

- [ ] **Step 2: Rodar** — Expected: não compila.
- [ ] **Step 3: Implementar.** No config, `tecla_<id>` com id de ação desconhecido é ignorado (como chave
  desconhecida); valor inválido ou não aceito deixa o padrão daquela ação; no fim da leitura, `corrigirRepetidas()`
  (a ação que repete volta ao padrão; se o padrão também estiver ocupado, ganha a primeira tecla livre do padrão das
  outras ações). Gravação: as 15 linhas sempre.
- [ ] **Step 4: Rodar tudo** — Expected: passam.

### Task 5: Tela Teclas e a entrada pelos atalhos

**Files:**
- Modify: `src/ui/Menus.h/.cpp` (`Tela::Teclas`, espera), `src/plataforma/Entrada.h/.cpp`, `src/main.cpp`,
  `src/render/TelaInventario.cpp:107`, `src/jogo/Armas.cpp:200`, `src/render/Hud.cpp` (texto da mensagem passa por
  `comTeclas`), `docs/COMO_JOGAR.txt`, `README.md`
- Test: `testes/teste_menus.cpp`, `testes/teste_telas_menu.cpp`, `testes/teste_armas.cpp` (mensagem com `{recarregar}`)

**Interfaces:**
- Consumes: Task 4 (`Atalhos`, nomes, `apertada`, `comTeclas`), Task 3 (índices das Opções).
- Produces: `ui::Tela::Teclas`; `EntradaMenu::teclaFisica` (`std::optional<int>`: a primeira tecla ou botão do mouse
  que desceu no quadro, com o número da Tarefa 4); `Menus::esperandoTecla() const -> bool`;
  `Entrada::definirAtalhos(const jogo::Atalhos&)`.

Tela Teclas: 15 linhas `"<nomeDaAcao>: <nomeDaTecla>"`, depois "Restaurar padrão" e "Voltar" (17 itens; com a linha
de aviso, a lista longa da Tarefa 3). Enter ou clique numa ação → `esperandoTecla()`, a linha vira
`"<nomeDaAcao>: aperte a tecla ou o botão"` e o aviso "Esc cancela". No quadro seguinte em diante: `voltar` cancela;
`teclaFisica` aceita → `c.teclas.definir(...)`, devolve `Acao::OpcoesMudaram`; não aceita → aviso
"Essa tecla não pode ser usada" e continua esperando. Voltar → Opções no item 8, devolve `Acao::SalvarOpcoes`.
"Restaurar padrão" → `c.teclas = Atalhos::padrao()`, `OpcoesMudaram`. Foco perdido (`focoPerdido()`) cancela a espera.

- [ ] **Step 1: Testes que falham**

```cpp
TEST_CASE("Teclas… abre a tela das teclas") { /* Opções, item 8, confirmar → Tela::Teclas, 17 itens, [0] == "Andar para frente: W" */ }
TEST_CASE("trocar uma tecla e cancelar") {
    // confirmar no item 4 (Pular) → esperando; EntradaMenu{teclaFisica = 8 (E)} → Pular=E, Usar=Espaço, não esperando
    // confirmar de novo → esperando; voltar → cancela, nada muda, continua na tela Teclas
}
TEST_CASE("abrir a espera não captura a própria tecla") {
    // um único EntradaMenu com clique no item 4 E teclaFisica = BOTAO_MOUSE+1 → esperando, Pular continua Espaço
}
TEST_CASE("tecla não aceita avisa e continua esperando") { /* teclaFisica = 0 → aviso, esperando */ }
TEST_CASE("restaurar padrão") { /* depois de trocar, confirmar em "Restaurar padrão" → teclas == padrão */ }
TEST_CASE("mensagem sem munição cita a tecla") { /* r.mensagem == "Sem munição — {recarregar} para recarregar" */ }
```

- [ ] **Step 2: Rodar** — Expected: falham.
- [ ] **Step 3: Implementar.**
  - `Entrada`: guarda os `Atalhos`; `coletar()` usa `apertada` para Frente/Trás/Esquerda/Direita/Atacar; o
    `KEYDOWN` (sem repetição) e o `MOUSEBUTTONDOWN` com o mouse capturado procuram `acaoDa(tecla)` para os eventos
    (pular, usar, trocar, recarregar, itens, inventário, mapa); Esc e roda como hoje. `ignorarBotao_` passa a valer
    para o botão do mouse que for o do Atacar (o clique que captura o mouse não ataca). No modo menu, cada
    `KEYDOWN`/`MOUSEBUTTONDOWN` preenche `menu_.teclaFisica` (se ainda vazio) além da navegação de hoje.
  - `main.cpp`: `entrada.definirAtalhos(config.teclas)` no início e em `OpcoesMudaram`/`SalvarOpcoes`; textos
    "Inventário ({inventario} fecha)" e a mensagem do HUD passam por `comTeclas` com o nome do teclado
    (a Tarefa 7 troca pelo do controle quando for o caso).
  - COMO_JOGAR e README: onde trocar as teclas.
- [ ] **Step 4: Rodar tudo** — Expected: passam. Captura da tela Teclas (`--tela teclas`, opção nova de captura) a olho.

### Task 6: Tradução do controle (puro)

**Files:**
- Create: `src/ui/Controle.h/.cpp` (no `nucleo`)
- Test: `testes/teste_controle.cpp` (novo)

**Interfaces:**
- Produces:
  - `enum class BotaoControle : uint8_t { A, B, X, Y, View, Start, LB, RB, Cima, Baixo, Esquerda, Direita };`
    `constexpr int TOTAL_BOTOES_CONTROLE = 12;`
  - `struct EstadoControle { bool ligado = false; float esqX = 0, esqY = 0, dirX = 0, dirY = 0; /* −1…1, y para
    baixo positivo, como o SDL */ float gatilhoDir = 0; /* 0…1 */ std::array<bool, TOTAL_BOTOES_CONTROLE> botoes{}; };`
  - `constexpr float ZONA_MORTA = 0.2f; float aplicarZonaMorta(float)` (|v| < 0,2 → 0; senão sinal × (|v| − 0,2) / 0,8)
  - `struct MovimentoControle { float frente = 0, lado = 0, giro = 0, inclinar = 0; bool atacar = false; };`
    `MovimentoControle movimentoDoControle(const EstadoControle&, float dt, int sensibilidade, bool inverter);`
    — frente = −esqY, lado = esqX; giro = curva(dirX) × VELOCIDADE_GIRO × 1,25^(s − 5) × dt, inclinar =
    −curva(dirY) × o mesmo (× −1 com `inverter`), com `curva(v) = sinal(v) × v²` depois da zona morta e
    `VELOCIDADE_GIRO = 3,0` rad/s; atacar = gatilhoDir > 0,5.
  - `struct EventosControle { bool pular = false, usar = false, recarregar = false, trocarArma = false,
    inventario = false, mapa = false, pausa = false; int usarRapido = 0; };`
    `EventosControle eventosDoControle(const EstadoControle& antes, const EstadoControle& agora);` (botão que desceu:
    A pular, X usar, B recarregar, Y trocar, View inventário, LB mapa, Start pausa, cruz 1 cima / 2 direita /
    3 baixo / 4 esquerda)
  - `ui::EntradaMenu menuDoControle(const EstadoControle& antes, const EstadoControle& agora);` (cruz ou analógico
    esquerdo cruzando 0,5: mover/ajustar; A confirmar; B ou Start voltar; uma vez por cruzamento)
  - `glm::vec2 moverCursor(glm::vec2 cursor, const EstadoControle&, float dt, glm::vec2 tamanhoJanela);`
    (analógico esquerdo, ou o direito se o esquerdo estiver parado; 700 px/s × curva; preso dentro da janela)
  - `std::string_view nomeNoControle(jogo::AcaoJogo)` ("Analógico esquerdo" para andar, "A", "X", "Y", "B", "View",
    "LB", "Cima"/"Direita"/"Baixo"/"Esquerda" para os itens, "RT" para atacar)

- [ ] **Step 1: Testes que falham**

```cpp
TEST_CASE("zona morta ignora drift") { /* esqX = 0.15, dirY = −0.19 → movimento todo zero */ }
TEST_CASE("controle desligado não comanda") { /* ligado=false com eixos e botões cheios → movimento zero, eventos vazios, menu vazio */ }
TEST_CASE("analógico anda e mira") { /* esqY = −1 → frente 1; dirX = 1, dt 0.5, s 5 → giro 1.5; inverter troca o sinal de inclinar */ }
TEST_CASE("botões disparam uma vez") { /* A desceu → pular; A segurado (antes e agora) → nada; cruz Direita → usarRapido 2 */ }
TEST_CASE("gatilho ataca a partir da metade") { /* 0.4 → não; 0.6 → sim */ }
TEST_CASE("menu: analógico move uma vez por cruzamento") { /* esqY 0→0.8: mover 1; 0.8→0.9: 0; volta a 0 e 0.8: 1 */ }
TEST_CASE("menu: A confirma, B e Start voltam") {}
TEST_CASE("cursor anda e fica na janela") { /* (10,10), esqX −1, dt 1 → x == 0; esqX 1, dt 0.1 → x == 70 */ }
```

- [ ] **Step 2: Rodar** — Expected: não compila.
- [ ] **Step 3: Implementar** (`menuDoControle` usa só as transições; não guarda estado).
- [ ] **Step 4: Rodar tudo** — Expected: passam.

### Task 7: Controle no jogo, nos menus e no inventário

**Files:**
- Modify: `src/plataforma/Janela.cpp:18` (`SDL_INIT_GAMECONTROLLER`), `src/plataforma/Entrada.h/.cpp`,
  `src/main.cpp`, `src/ui/Menus.h/.cpp` (item 9 abre `Tela::Controle`), `docs/COMO_JOGAR.txt`, `README.md`
- Test: `testes/teste_menus.cpp`

**Interfaces:**
- Consumes: Task 6 (tudo), Task 5 (`definirAtalhos`, `comTeclas`, `teclaFisica`), Task 3 (índices).
- Produces: `Entrada::atualizarControle(float dt)` (chamado uma vez por quadro), `Entrada::ultimoFoiControle() const`,
  `Entrada::definirControle(int sensibilidade, bool inverter)`; `ui::Tela::Controle`.

Tela Controle: só leitura, uma linha por ação no formato `"<botão>: <ação>"` da tabela da spec (com o nome do
PlayStation entre parênteses: "A (PS: X)", "B (PS: Círculo)", "X (PS: Quadrado)", "Y (PS: Triângulo)", "RT (PS: R2)",
"LB (PS: L1)", "View (PS: Share)", "Start (PS: Options)") e "Voltar" (volta às Opções no item 9).

- [ ] **Step 1: Testes que falham**

```cpp
TEST_CASE("Controle… mostra o esquema e volta") { /* item 9 → Tela::Controle; algum item contém "RT (PS: R2)"; voltar → Opções, item 9 */ }
```

- [ ] **Step 2: Rodar** — Expected: falham.
- [ ] **Step 3: Implementar.**
  - `Entrada`: abre o primeiro controle em `SDL_CONTROLLERDEVICEADDED` (e na criação, se já houver), fecha em
    `REMOVED` (estado volta a `ligado = false`). `atualizarControle(dt)` lê eixos e botões, guarda `antes`/`agora` e:
    no jogo, acumula `movimentoDoControle` e os `eventosDoControle` nos pendentes (somando com teclado e mouse,
    `frente`/`lado` presos em −1…1; atacar = teclado/mouse OU gatilho); `pausa` vira `pausaPendente_`, `inventario`
    abre; nos menus, junta `menuDoControle` em `menu_`; com o inventário aberto, `moverCursor` move
    `posicaoMouse_` e `SDL_WarpMouseInWindow` leva o mouse junto, A desce/sobe = `cliques_.pressionou`/`soltou`,
    X = `cliques_.direito`, B ou View fecham. `ultimoFoiControle_` liga com qualquer botão ou eixo fora da zona
    morta e desliga com tecla ou mouse.
  - `main.cpp`: `entrada.atualizarControle(dt)` a cada quadro antes de coletar; `definirControle` no início e quando
    as opções mudam; nomes dos textos: `ultimoFoiControle()` ? `nomeNoControle` : `nomeDaTecla`.
  - COMO_JOGAR e README: tabela do controle e as duas opções.
- [ ] **Step 4: Rodar tudo** — Expected: passam. Pacote de teste (`empacotar.ps1`) passa no teste de fumaça. O
  controle de verdade fica para o Nidragnnar (cenários e tabela no fim).
