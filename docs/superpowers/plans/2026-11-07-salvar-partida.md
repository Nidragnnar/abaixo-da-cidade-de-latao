# Salvar e sair — Plano de implementação

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Objetivo:** "Salvar e sair" (e fechar a janela) grava a partida; "Continuar" a remonta igual e apaga o arquivo.

**Arquitetura:** `jogo/Salvamento.*` escreve e lê um texto em linhas (testável sem disco). O andar é refeito por
`gerarAndar(semente, numero)`; o arquivo guarda só as células da grade que mudaram, a exploração e o estado da partida
(jogador, inventário, armas, inimigos, chão, contadores). `Salvamento` é amigo de `Partida` e de `Inventario`.

**Tecnologia:** C++20, doctest; MSVC, preset vs/release.

**Especificação:** `docs/superpowers/specs/2026-11-07-salvar-partida-design.md` (ler junto).

## Restrições globais

- Comentários e nomes em português, mesmo estilo do código existente.
- Compilar e testar com `.\ferramentas\compilar.ps1 release` (nunca Debug).
- Testes que mexem em arquivo usam pasta no scratchpad/temporária; nunca `%APPDATA%\Roguelike` real.
- Sem commit nem push sem o "sim" do Nidragnnar; um commit no fim; sem `Co-Authored-By`.
- Arquivo `partida.sav` na pasta de dados; primeira linha `versao 1`; números escritos e lidos sem depender do
  idioma do Windows (`std::format` / `std::from_chars`, nunca `stof` com vírgula).
- Mensagens exatas: "Partida salva não pôde ser lida", "Não foi possível salvar", "Apagar a partida salva?".

## Foco da revisão

1. Salvar dentro da arena trancada: ao continuar, a arena segue trancada, o chefe acordado e a música/barra de luta
   voltam. → teste na Tarefa 1 ("arena trancada volta trancada").
2. Salvar durante a contagem da vitória (Grande Máquina caída): não grava (conta como vitória). → teste na Tarefa 1
   ("contagem da vitória não grava").
3. Válvula-chave no bolso e comporta destrancada: voltam iguais. → teste na Tarefa 1 (dentro do teste de ida e volta).
4. Números com casas decimais (posição 12.375): leitura igual com o Windows em português. → teste na Tarefa 1
   ("números sem depender do idioma").
5. Arquivo com lixo, cortado no meio ou gigante: devolve nada, sem travar nem estourar memória. → teste na Tarefa 1
   ("arquivo estragado não trava").

---

### Tarefa 1: Escrever e ler a partida (texto)

**Arquivos:** criar `src/jogo/Salvamento.h/.cpp`, `testes/teste_salvamento.cpp`; mexer em `src/jogo/Partida.h`
(`friend struct Salvamento;`), `src/jogo/Inventario.h` (idem), `CMakeLists.txt`.

**Interfaces (produz):**
- `struct Salvamento { static std::optional<std::string> escrever(const Partida&);
  static std::optional<Partida> ler(std::string_view texto); };` — `escrever` devolve nada quando a partida não pode
  ser gravada (morto, venceu, contagem da vitória).
- Livres: `std::optional<std::string> escreverPartida(const Partida&)` e
  `std::optional<Partida> lerPartida(std::string_view)` chamando o struct.
- Formato (uma coisa por linha, campos separados por espaço): `versao 1`, `partida <semente> <andar>`,
  `jogador ...`, `contadores ...`, `municao ...`, `arma <ativa>`, `equipado <branca|fogo|armadura> <item>`,
  `espaco <i> <item>`, `celula <x> <y> <tipo>` (só as diferentes do andar gerado), `chave <x> <z>|sem`,
  `arena <lutando> <x> <y>...`, `explorado <64 linhas hex>`, `inimigo ...`, `chao ...`, `fim`.
  `<item>` = `tipo qualidade quantidade carga nbonus (tipoBonus valor)*`.

- [ ] Testes (escrever primeiro):
  - "ida e volta dá a mesma partida": Partida(42, 4) jogada alguns passos, com item no inventário (obra-prima com 2
    bônus), arma de fogo equipada com carga 3, válvula pega, uma comporta aberta, um inimigo destruído; `ler(escrever(p))`
    tem os mesmos: posição/ângulo/pés, vida, nível, XP, engrenagens, inventário (tipo, qualidade, bônus, carga),
    arma ativa e munição, células da grade, exploração, inimigos (tipo, posição, vida, estado), chão, contadores, tempo.
  - "arena trancada volta trancada": partida de teste com arena trancada e chefe Perseguindo → depois de ler,
    `lutaComChefe()` e as comportas da arena trancadas.
  - "morto ou vencido não grava" e "contagem da vitória não grava": `escrever` devolve vazio.
  - "números sem depender do idioma": posição 12.375 / 7.125 volta exata.
  - "arquivo estragado não trava": `ler("")`, `ler("versao 2...")`, texto cortado pela metade, linha com lixo, 1 MB
    de lixo → `nullopt`.
- [ ] Ver falhar; implementar (a leitura monta `Partida(semente, andar)` e sobrescreve o estado; inimigo em
  Preparando/Recuperando volta Perseguindo; projéteis e explosões vazios; comportas abrindo ficam fechadas).
- [ ] Suite passa.

### Tarefa 2: Arquivo no disco

**Arquivos:** `src/jogo/Salvamento.h/.cpp`, `testes/teste_salvamento.cpp`.

**Interfaces (produz):** `enum class ResultadoCarga { Nenhum, Carregou, Estragado };`
`bool gravarPartida(const std::filesystem::path&, const Partida&)` (usa `gravarAtomico`; falso se não gravou ou se a
partida não pode ser gravada); `std::pair<ResultadoCarga, std::optional<Partida>> continuarPartida(const path&)`
(carregou → apaga o arquivo; estragado → `renomearCorrompido`); `bool existePartidaSalva(const path&)`.

- [ ] Testes: "gravar e continuar apaga o arquivo"; "continuar sem arquivo dá Nenhum"; "arquivo estragado vira
  .corrompido e dá Estragado"; "gravar morto não cria arquivo".
- [ ] Ver falhar; implementar; suite passa.

### Tarefa 3: Menus

**Arquivos:** `src/ui/Menus.h/.cpp`, `testes/teste_menus.cpp`.

**Interfaces (produz):** `void Menus::definirPartidaSalva(bool)`; `Acao` ganha `ContinuarSalva` e `SalvarESair`.
Principal com salvamento: `{"Continuar partida", "Novo jogo", "Placar", "Opções", "Sair"}`; "Novo jogo" com salvamento
pede confirmação (texto do item vira "Apagar a partida salva?"; confirmar de novo devolve `NovoJogo`). Pausa:
`{"Continuar", "Salvar e sair", "Opções", "Abandonar partida"}`.

- [ ] Testes: "com partida salva, o primeiro item é Continuar partida e devolve ContinuarSalva"; "Novo jogo com partida
  salva pede confirmação"; "sem partida salva o menu é o de antes"; "pausa tem Salvar e sair e devolve SalvarESair";
  ajustar os testes de pausa existentes aos novos índices.
- [ ] Ver falhar; implementar; suite passa.

### Tarefa 4: Ligar no jogo, cenário e pacote

**Arquivos:** `src/main.cpp`, `docs/COMO_JOGAR.txt`, `README.md`, `build\Release\20_Continuar_partida_salva.bat`
(+ `build\Release\cenarios\partida_salva\partida.sav` gerado pelo jogo).

- [ ] `main.cpp`: na abertura, `definirPartidaSalva(existePartidaSalva(...))`; `ContinuarSalva` → `continuarPartida`
  (Estragado → mensagem "Partida salva não pôde ser lida" e menu sem Continuar); `SalvarESair` → `gravarPartida`
  (falhou → mensagem "Não foi possível salvar" e fica; gravou → volta ao menu principal com "Continuar partida");
  `NovoJogo` com salvamento confirmado apaga o arquivo; fechar a janela com partida em andamento (não fundo, não
  terminada, sem `--demo`/`--captura`/`--invencivel`) grava antes de sair.
- [ ] Cenário 20: gerar o `partida.sav` com o próprio jogo (andar 4 perto do Capataz) numa pasta `--dados` de teste e
  o `.bat` abrir com `--dados` nessa pasta (copiando o arquivo antes, para poder repetir).
- [ ] COMO_JOGAR e README: "Salvar e sair" na pausa; "Continuar partida" no menu.
- [ ] `.\ferramentas\empacotar.ps1` → "teste de fumaça ok"; abrir o cenário 20; tabela do que olhar.
- [ ] Commit único só com o "sim" do Nidragnnar.
