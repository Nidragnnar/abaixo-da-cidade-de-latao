# Etapa 5 — Som, menus, opções, `--demo` e pacote: Plano de implementação

> **Para agentes:** SUB-SKILL OBRIGATÓRIA: usar superpowers:subagent-driven-development (recomendado) ou superpowers:executing-plans para executar este plano tarefa por tarefa. Os passos usam caixas (`- [ ]`) para acompanhamento.

**Objetivo:** transformar o protótipo em um jogo que um amigo abre com dois cliques: **"Abaixo da Cidade de Latão"**, com efeitos sonoros e música, menu principal, pausa, tela de placar, opções salvas em `config.txt`, o modo `--demo` que anda sozinho até o elevador tirando capturas, e o pacote `AbaixoDaCidadeDeLatao-v1.zip` com o guia `COMO_JOGAR.txt`.

**Arquitetura:** a regra continua em código puro e testado sem janela. Em `jogo/` ficam os eventos de som que a `Partida` produz a cada passo e a mixagem por distância, as opções (`Config`, com leitura e gravação seguras, que reaproveitam a gravação atômica do placar) e o piloto automático do `--demo`. A navegação das telas vai para `ui/Menus`, também pura. O `render/` só monta as telas. Em `plataforma/` entram o `Audio` (SDL_mixer), o único lugar que toca som, e as novas teclas da `Entrada`. O `main` liga as telas ao laço.

**Tecnologia:** a mesma das etapas anteriores, mais **SDL_mixer 2.8.1** (FetchContent, como DLL; OGG pelo stb_vorbis embutido e WAV).

**Spec:** `docs/superpowers/specs/2026-11-01-roguelike-design.md` (seções 2, 7, 8, 9, 11, 12 "roteiro de capturas", 13 etapa 5, 14)

## Restrições globais

- Tudo das etapas 1–4 continua valendo. Em especial: testes que podem gravar placar ou opções usam `--dados <pasta no scratchpad>`; nada é apagado no `%APPDATA%`; programas de teste próprios são compilados em Release.
- **Nome do jogo:** "Abaixo da Cidade de Latão" no título da janela e no menu. Pacote: `AbaixoDaCidadeDeLatao-v1.zip`, com a pasta `AbaixoDaCidadeDeLatao\` dentro. O executável do pacote se chama `AbaixoDaCidadeDeLatao.exe`, uma cópia de `Roguelike.exe`. Por dentro (alvo CMake, `build\`, pasta `%APPDATA%\Roguelike`) o nome continua `Roguelike`, para não perder o placar já gravado nem quebrar os cenários `.bat`.
- **Telas (seção 8):** menu principal (Novo jogo, Placar, Opções, Sair) → jogo → pausa / inventário → fim de partida.
- **Opções (seção 8)** em `pastaDeDados()/config.txt`: sensibilidade do mouse, volume de efeitos, volume da música, tela cheia ou janela, semente fixa (vazio = aleatória).
- **Arquivo corrompido (seção 11):** `config.txt` vira `config.txt.corrompido` e é recriado com os valores padrão. Arquivo que existe mas não abre (bloqueado): usar o padrão e **não** gravar por cima (mesma regra do placar).
- **Foco (seção 11):** a janela perde o foco durante o jogo → pausa e solta o mouse.
- **Efeitos (seção 9):** passos em metal e pedra, golpe no ar, acerto, disparo de cada arma de fogo, recarga, clique de arma vazia, dano no jogador, inimigo destruído, comporta abrindo (vapor), pegar item, subir de nível, elevador descendo. Música ambiente industrial em loop.
- **Som ausente ou inválido (seção 11):** silêncio e uma linha em `log.txt`; o jogo não fecha. Sem placa de som: o jogo roda mudo.
- **`--demo <semente>` (seção 12):** anda sozinho até o elevador, salva capturas em `capturas\` e fecha.
- **Pacote (seção 14):** exe, `SDL2.dll`, `SDL2_mixer.dll`, `assets\`, `COMO_JOGAR.txt` e `LICENCAS.txt`; Release com `/MT`, sem precisar instalar o Visual C++ Redistributable.
- **Downloads:** aprovar este plano vale como ok para o CMake baixar o **SDL_mixer `release-2.8.1`**. Sons e música de pacotes externos pedem **outro ok** (Tarefa 9); até lá, os sons são provisórios, gerados por script, como foi feito com a arte.
- **Constantes desta etapa** (não fixadas pela spec; ajustar depois do teste):
  - sensibilidade de 1 a 10, padrão 5; radianos por pixel = `0,0025 × 1,25^(s − 5)`;
  - volumes de 0 a 10; padrão: efeitos 8, música 5;
  - som posicional: volume 1 até 2 células, cai em linha reta até 0 a 16 células; pan limitado a ±0,8;
  - um passo a cada 1,7 de distância andada: metal na fábrica, pedra no esgoto e nas minas;
  - 32 canais de mixagem; áudio em 44.100 Hz, estéreo, buffer de 1024;
  - `--demo`: uma captura a cada 120 passos de lógica (2 s) e outra ao descer; desiste depois de 180 s de jogo.
- **Commit:** um único, no fim, com o "sim" do Nidragnnar.

## Decisões de projeto

- **Eventos de som saem da lógica, não do desenho.** A cada `atualizar()`, a `Partida` monta a lista `sons()` daquele passo, com o som e, quando faz sentido, a posição de origem. O `main` lê a lista depois de cada passo e entrega ao `Audio`. Assim, os sons ficam testáveis sem placa de som. Os sons sem origem (dano, nível, pegar) entram **no máximo uma vez por passo**: sete chumbos acertando não tocam sete vezes. Os sons com origem (comportas, tiros dos Atiradores) entram todos.
- **O tiro dos Atiradores também faz som** (`TiroInimigo`, posicional). A spec não lista, mas sem ele um tiro vindo do escuro fica mudo.
- **Esc no jogo abre a pausa** (antes só soltava o mouse). Na pausa: Continuar, Opções, Abandonar partida. Abandonar pede um segundo Enter ("Confirmar: abandonar?"), volta ao menu e **não grava no placar**. Esc com o inventário aberto fecha o inventário (pendência da Etapa 4).
- **Fim de partida:** Enter começa outra partida; Esc volta ao menu.
- **Navegação:** teclado e mouse. W/S ou setas movem, Enter confirma, Esc volta. Nas opções, A/D, setas ou clique (esquerdo +, direito −) mudam o valor. A semente é digitada (só dígitos; Backspace apaga). As opções valem na hora e são gravadas ao sair da tela de opções.
- **Fundo do menu:** o andar 1 de uma semente fixa (1), com a câmera girando devagar e um painel escuro por cima. Reaproveita o desenho que já existe e mostra o clima do jogo.
- **Começar direto no jogo:** se a linha de comando tiver `--semente`, `--andar`, `--posicao`, `--captura`, `--invencivel`, `--inventario` ou `--demo`, o menu é pulado. Assim os cenários `.bat` e os roteiros de captura continuam funcionando.
- **Semente da partida:** `--semente` tem prioridade; depois vem a semente fixa das opções; sem nenhuma das duas, a semente é aleatória.
- **Sons provisórios por programa:** o alvo `gerar_sons` (C++, fora do jogo) sintetiza os 18 efeitos e uma música de 32 s em WAV com semente fixa. Os arquivos ficam em `assets/sons/`. O `Audio` procura `<nome>.ogg` e, se não houver, `<nome>.wav`; trocar por um pacote baixado é só pôr o `.ogg` no lugar e apagar o `.wav`.
- **Roteiros automáticos rodam sem som e sem caixa de erro:** `capturar.ps1`, o `--demo` e os testes do pacote definem `SDL_AUDIODRIVER=dummy`. O caminho inteiro do áudio roda, mas nada sai nos alto-falantes do Nidragnnar.

## Foco de revisão

1. **Sem placa de som, driver de áudio inválido ou som ausente/corrompido:** o jogo abre mudo e registra no log. → passo de verificação com `SDL_AUDIODRIVER=inexistente` e com um `.wav` truncado (Tarefa 6).
2. **`config.txt` corrompido, com valor fora da faixa, chave desconhecida ou bloqueado:** o corrompido vira `.corrompido` e o padrão é recriado; chave desconhecida é ignorada; o arquivo bloqueado não é sobrescrito. → testes `config: …` (Tarefa 1).
3. **Alt-Tab no meio da luta, com o inventário aberto, na tela de fim ou no menu:** no jogo, abre a pausa; nas outras telas, nada muda e nada se perde. → teste `foco perdido só pausa o jogo` (Tarefa 3).
4. **Tela cheia em monitor com escala de 125%/150% e troca ao vivo entre janela e tela cheia:** o clique cai no item certo. → teste `item sob o mouse` (Tarefa 4) e linha na tabela de testes do Nidragnnar.
5. **Semente digitada:** número maior que 4294967295, letras, apagar até ficar vazio. → teste `semente: só dígitos, até 4294967295` (Tarefa 3).

---

## Estrutura de arquivos

```
src/jogo/ArquivoTexto.h/.cpp  gravação atômica e renomear corrompido (extraído de Placar.cpp)
src/jogo/Config.h/.cpp        opções: padrão, ler, gravar, sensibilidade em rad/px
src/jogo/Sons.h/.cpp          enum Som, nomes dos arquivos, EventoSom, mixagem por distância, ganhos estéreo
src/jogo/Armas.h/.cpp         ResultadoAtaque + golpeou, vazia, comecouRecarga
src/jogo/Partida.h/.cpp       sons() do passo, passos andados
src/jogo/Demo.h/.cpp          PilotoDemo
src/ui/Menus.h/.cpp           telas, itens, seleção, ajuste das opções, semente digitada
src/render/Letterbox.h/.cpp   + contem(Retangulo, ponto)
src/render/TelasMenu.h/.cpp   menu/pausa/opções, placar, layout e item sob o mouse
src/render/Hud.cpp            rodapé da tela de fim
src/plataforma/Audio.h/.cpp   SDL_mixer
src/plataforma/Entrada.*      pausa, Esc no inventário, modo menu, sensibilidade, texto
src/plataforma/Janela.*       título, tela cheia
src/main.cpp                  telas, áudio, opções, --demo, --mudo
cmake/Dependencias.cmake      SDL_mixer 2.8.1
CMakeLists.txt                novos arquivos, ui/, SDL2_mixer.dll, alvo gerar_sons
ferramentas/gerar_sons.cpp    sons e música provisórios
ferramentas/empacotar.ps1     pacote .zip com verificação
ferramentas/capturar.ps1      SDL_AUDIODRIVER=dummy
docs/COMO_JOGAR.txt           guia para os amigos
assets/sons/*.wav, assets/LICENCAS.txt, .gitignore (pacote/), spec seção 14
testes/teste_config.cpp, teste_sons.cpp, teste_demo.cpp, teste_menus.cpp, teste_telas_menu.cpp
(+ casos em teste_armas, teste_partida, teste_placar, teste_hud)
```

---

### Tarefa 1: Arquivos de texto seguros e `Config`

**Arquivos:**
- Criar: `src/jogo/ArquivoTexto.h/.cpp`, `src/jogo/Config.h/.cpp`, `testes/teste_config.cpp`
- Modificar: `src/jogo/Placar.cpp` (usa `ArquivoTexto`), `CMakeLists.txt`

**Interfaces:**
- Produz:
  - `bool jogo::gravarAtomico(const std::filesystem::path&, std::string_view conteudo)` — cria a pasta, grava num `.tmp` e troca (o código que hoje está em `gravarPlacar`).
  - `bool jogo::renomearCorrompido(const std::filesystem::path&)` — vira `<nome>.corrompido`, substituindo um anterior; devolve se conseguiu.
  - ```cpp
    struct Config {
        int sensibilidade = 5;     // 1–10
        int volumeEfeitos = 8;     // 0–10
        int volumeMusica = 5;      // 0–10
        bool telaCheia = false;
        std::optional<uint32_t> semente;  // vazio = aleatória
    };
    struct LeituraConfig { Config config; bool estavaCorrompido = false, erroDeLeitura = false, renomeado = false; };
    LeituraConfig lerConfig(const std::filesystem::path&);
    bool gravarConfig(const std::filesystem::path&, const Config&);
    float radianosPorPixel(int sensibilidade);  // 0,0025 × 1,25^(s − 5)
    ```
- Formato gravado (UTF-8 sem BOM, BOM aceito na leitura):
  ```
  # Abaixo da Cidade de Latão — opções (apague este arquivo para voltar ao padrão)
  sensibilidade=5
  volume_efeitos=8
  volume_musica=5
  tela_cheia=nao
  semente=
  ```
- Regras de leitura: linha vazia e `#` são ignoradas; chave desconhecida é ignorada; chave ausente fica com o padrão; **corrompido** = linha sem `=`, número inválido, valor fora da faixa ou `tela_cheia` diferente de `sim`/`nao`. Quando está corrompido, o arquivo é renomeado e a leitura devolve o `Config{}` padrão (quem chama grava o novo).

- [ ] **Passo 1: Escrever os testes que falham** (em `testes/teste_config.cpp`, numa pasta temporária por caso, como em `teste_placar.cpp`)

```cpp
TEST_CASE("config: ausente devolve o padrão sem erro") { /* lerConfig(pasta/"config.txt"): config == Config{}, nenhuma flag */ }
TEST_CASE("config: gravar e ler de volta") {
    jogo::Config c{9, 0, 10, true, 4294967295u};
    REQUIRE(jogo::gravarConfig(arq, c));
    auto l = jogo::lerConfig(arq);
    CHECK(l.config.sensibilidade == 9); CHECK(l.config.volumeEfeitos == 0); CHECK(l.config.volumeMusica == 10);
    CHECK(l.config.telaCheia); CHECK(l.config.semente == 4294967295u);
}
TEST_CASE("config: semente vazia é aleatória") { /* "semente=" → !semente */ }
TEST_CASE("config: chave desconhecida é ignorada e chave ausente fica no padrão") {
    /* "volume_musica=2\ncor=azul\n" → volumeMusica 2, sensibilidade 5, estavaCorrompido falso */
}
TEST_CASE("config: valor fora da faixa corrompe") {
    /* "sensibilidade=11" → estavaCorrompido, renomeado, config == padrão, existe config.txt.corrompido */
}
TEST_CASE("config: linha sem igual, número inválido e tela_cheia estranha corrompem") { /* três arquivos */ }
TEST_CASE("config: BOM no começo é aceito") { /* "\xEF\xBB\xBFvolume_efeitos=3" → 3, sem corrompido */ }
TEST_CASE("config: arquivo bloqueado não é sobrescrito") {
    /* abrir com _wfsopen(..., _SH_DENYRW) como no teste do placar → erroDeLeitura, !estavaCorrompido */
}
TEST_CASE("sensibilidade em radianos por pixel") {
    CHECK(jogo::radianosPorPixel(5) == doctest::Approx(0.0025f));
    CHECK(jogo::radianosPorPixel(1) == doctest::Approx(0.001024f));
    CHECK(jogo::radianosPorPixel(10) == doctest::Approx(0.0076294f).epsilon(0.001));
}
```

- [ ] **Passo 2: Rodar e ver falhar** — `.\ferramentas\compilar.ps1` → erro de compilação (`jogo/Config.h` não existe).
- [ ] **Passo 3: Extrair `gravarAtomico` e `renomearCorrompido` de `Placar.cpp`** sem mudar o comportamento.
- [ ] **Passo 4: Rodar** — os testes do placar continuam passando.
- [ ] **Passo 5: Implementar `Config.cpp`** (lê linha a linha com `std::from_chars`, sem exceções).
- [ ] **Passo 6: Rodar** — `.\ferramentas\compilar.ps1` → tudo passa.

---

### Tarefa 2: Eventos de som na lógica

**Arquivos:**
- Criar: `src/jogo/Sons.h/.cpp`, `testes/teste_sons.cpp`
- Modificar: `src/jogo/Armas.h/.cpp`, `src/jogo/Partida.h/.cpp`, `testes/teste_armas.cpp`, `testes/teste_partida.cpp`, `CMakeLists.txt`

**Interfaces:**
- Produz:
  - ```cpp
    enum class Som { PassoPedra, PassoMetal, GolpeAr, Acerto, DisparoRevolver, DisparoBacamarte, DisparoTesla,
                     Recarga, ArmaVazia, Dano, InimigoDestruido, Comporta, Pegar, Nivel, Elevador, TiroInimigo,
                     MenuMover, MenuConfirmar, TOTAL };
    // Nome do arquivo sem extensão: "passo_pedra", "passo_metal", "golpe_ar", "acerto", "disparo_revolver",
    // "disparo_bacamarte", "disparo_tesla", "recarga", "arma_vazia", "dano", "inimigo_destruido", "comporta",
    // "pegar", "nivel", "elevador", "tiro_inimigo", "menu_mover", "menu_confirmar"
    std::string_view nomeDoSom(Som);
    struct EventoSom { Som som; std::optional<glm::vec2> origem; };  // sem origem = no ouvido do jogador
    struct Mixagem { float volume; float pan; };  // volume 0–1; pan −1 esquerda … +1 direita
    Mixagem mixar(const EventoSom&, glm::vec2 ouvinte, float angulo);
    struct GanhosEstereo { uint8_t esquerda, direita; };
    GanhosEstereo ganhosEstereo(float pan);  // esquerda = 255·min(1, 1 − pan), direita = 255·min(1, 1 + pan)
    ```
  - `ResultadoAtaque` ganha `bool golpeou` (arma branca atacou), `bool vazia` (onde hoje se escreve "Sem munição…"/"Sem células elétricas", já limitado pelo intervalo) e `bool comecouRecarga`.
  - `const std::vector<EventoSom>& Partida::sons() const` — os sons do último `atualizar()` (a lista é zerada no começo de cada chamada).
- Onde cada som nasce, na `Partida`:
  - passos a cada 1,7 de distância andada (`PassoMetal` na fábrica, `PassoPedra` nas outras zonas);
  - `GolpeAr` quando `golpeou && acertos == 0`; `Acerto` quando `acertos > 0`;
  - `DisparoRevolver`/`DisparoBacamarte`/`DisparoTesla` quando `disparou`, pelo tipo da arma de fogo equipada;
  - `Recarga` quando `comecouRecarga`; `ArmaVazia` quando `vazia`;
  - `Dano` em `receberDano`, inclusive invencível; `InimigoDestruido` em `recompensarDestruidos`;
  - `Comporta`, com origem no centro da célula, quando uma comporta **começa** a abrir (pelo jogador ou pela IA: `try_emplace(...).second`);
  - `Pegar` ao pegar item, munição, engrenagens ou a válvula; `Nivel` em `ganharXp` quando sobe; `Elevador` em `descer`;
  - `TiroInimigo` com origem para cada tiro novo de `eventos.tiros`.
- Sons sem origem: no máximo um de cada por passo. Sons com origem: todos.

- [ ] **Passo 1: Escrever os testes que falham**

```cpp
// teste_sons.cpp
TEST_CASE("cada som tem um nome de arquivo único") { /* todos os Som até TOTAL: não vazio, sem repetição */ }
TEST_CASE("mixagem: perto é volume cheio, longe é silêncio") {
    CHECK(jogo::mixar({jogo::Som::Acerto, glm::vec2(1, 0)}, {0, 0}, 0).volume == doctest::Approx(1));
    CHECK(jogo::mixar({jogo::Som::Acerto, glm::vec2(9, 0)}, {0, 0}, 0).volume == doctest::Approx(0.5));
    CHECK(jogo::mixar({jogo::Som::Acerto, glm::vec2(20, 0)}, {0, 0}, 0).volume == doctest::Approx(0));
    auto semOrigem = jogo::mixar({jogo::Som::Dano, std::nullopt}, {0, 0}, 0);
    CHECK(semOrigem.volume == doctest::Approx(1)); CHECK(semOrigem.pan == doctest::Approx(0));
}
TEST_CASE("mixagem: som à direita sai na direita") {
    // ângulo 0 olha para +X; a direita é +Z
    CHECK(jogo::mixar({jogo::Som::Comporta, glm::vec2(0, 3)}, {0, 0}, 0).pan == doctest::Approx(0.8));
    CHECK(jogo::mixar({jogo::Som::Comporta, glm::vec2(0, -3)}, {0, 0}, 0).pan == doctest::Approx(-0.8));
    CHECK(jogo::mixar({jogo::Som::Comporta, glm::vec2(3, 0)}, {0, 0}, 0).pan == doctest::Approx(0));
}
TEST_CASE("ganhos estéreo") {
    CHECK(jogo::ganhosEstereo(0).esquerda == 255); CHECK(jogo::ganhosEstereo(0).direita == 255);
    CHECK(jogo::ganhosEstereo(0.8f).esquerda == 51); CHECK(jogo::ganhosEstereo(0.8f).direita == 255);
}
// teste_armas.cpp
TEST_CASE("resultado do ataque informa golpe, arma vazia e recarga") {
    /* golpe da chave no vazio → golpeou; revólver com carga 0 e reserva 12 segurando o botão por 1 s
       → vazia em no máximo ceil(1 / 0,4) passos; R → comecouRecarga num único passo */
}
// teste_partida.cpp (mapas de texto como nos testes existentes)
TEST_CASE("andar faz passos de pedra no esgoto") { /* 1 s andando para frente → pelo menos 1 PassoPedra, nenhum PassoMetal */ }
TEST_CASE("disparo do bacamarte toca o som certo e acerto toca uma vez") {
    /* bacamarte equipado, Colosso à frente: o passo do disparo tem DisparoBacamarte e exatamente um Acerto */
}
TEST_CASE("comporta abrindo toca o som com origem na comporta") { /* E diante da comporta → Comporta com origem (x+0,5, y+0,5) */ }
TEST_CASE("descer e subir de nível tocam elevador e nível") { /* E no elevador → Elevador; ganharXp(20) → Nivel */ }
TEST_CASE("sons valem só para o passo em que aconteceram") { /* depois do E na comporta, um passo parado → sons() vazio */ }
```

- [ ] **Passo 2: Rodar e ver falhar** — compilação falha (`jogo/Sons.h`).
- [ ] **Passo 3: Implementar `Sons.cpp`, os campos em `Armas.cpp` e os eventos em `Partida.cpp`.** Um `void Partida::tocar(Som, std::optional<glm::vec2> = {})` privado aplica a regra de "um de cada por passo" para sons sem origem.
- [ ] **Passo 4: Rodar** — `.\ferramentas\compilar.ps1` → tudo passa.

---

### Tarefa 3: Navegação das telas (`ui/Menus`)

**Arquivos:**
- Criar: `src/ui/Menus.h/.cpp`, `testes/teste_menus.cpp`
- Modificar: `CMakeLists.txt` (`ui/` no `nucleo`)

**Interfaces:**
- Consome: `jogo::Config` (Tarefa 1).
- Produz:
  ```cpp
  namespace ui {
  enum class Tela { Principal, Jogo, Pausa, Placar, Opcoes, Fim };
  enum class Acao { Nenhuma, NovoJogo, Continuar, Abandonar, Sair, OpcoesMudaram, SalvarOpcoes, AbrirPlacar };
  struct EntradaMenu {
      int mover = 0;        // −1 cima, +1 baixo
      int ajustar = 0;      // −1 / +1 (A/D, setas, clique direito / esquerdo num valor)
      bool confirmar = false, voltar = false, apagar = false;
      std::optional<int> itemSobMouse;  // o mouse em cima seleciona
      bool clique = false, cliqueDireito = false;
      std::string texto;    // caracteres digitados (só dígitos contam)
  };
  class Menus {
  public:
      Tela tela() const;
      void abrir(Tela);                // Jogo, Fim e Principal vêm de fora (início, morte, fim do abandono)
      void focoPerdido();              // Jogo → Pausa; nas outras telas não faz nada
      Acao atualizar(const EntradaMenu&, jogo::Config&);
      std::vector<std::string> itens(const jogo::Config&) const;  // textos da tela atual
      int selecionado() const;
      std::string_view titulo() const;  // "Abaixo da Cidade de Latão", "Pausa", "Opções", "Placar"
  };
  }
  ```
- Itens (texto exato):
  - Principal: `Novo jogo`, `Placar`, `Opções`, `Sair`.
  - Pausa: `Continuar`, `Opções`, `Abandonar partida` (depois do primeiro Enter, `Confirmar: abandonar?`; mover a seleção desfaz).
  - Opções: `Sensibilidade do mouse: 5`, `Volume dos efeitos: 8`, `Volume da música: 5`, `Tela cheia: não`/`sim`, `Semente fixa: aleatória`/`Semente fixa: 12345`, `Voltar`.
  - Placar: `Voltar` (a lista é desenhada à parte).
- Regras: Esc no Jogo abre a Pausa (sem `Acao`). Esc na Pausa = Continuar. Esc ou `Voltar` nas Opções → `SalvarOpcoes` e volta para onde estava (Principal ou Pausa). Esc no Placar → Principal. Esc no Principal não faz nada. A seleção dá a volta (de cima para baixo e vice-versa). Os valores param nos limites (não dão a volta). Qualquer mudança de valor devolve `OpcoesMudaram`. Semente: dígitos são acrescentados enquanto o número continuar ≤ 4294967295; Backspace apaga o último; vazio = aleatória. `Placar` no menu principal devolve `AbrirPlacar` (quem chama lê o arquivo) e muda a tela.

- [ ] **Passo 1: Escrever os testes que falham**

```cpp
TEST_CASE("menu principal: mover dá a volta e confirmar devolve a ação") {
    /* começa em Principal, seleção 0; mover −1 → 3; confirmar em "Sair" → Acao::Sair; item 0 → NovoJogo */
}
TEST_CASE("Esc no jogo pausa; Esc na pausa continua") { /* abrir(Jogo); voltar → Pausa; voltar → Continuar e tela Jogo */ }
TEST_CASE("foco perdido só pausa o jogo") { /* Jogo → Pausa; Principal, Fim, Opções e Placar não mudam */ }
TEST_CASE("abandonar pede confirmação") {
    /* Pausa, item 2: confirmar → Nenhuma e o texto vira "Confirmar: abandonar?"; confirmar de novo → Abandonar;
       mover entre os dois Enter → volta a "Abandonar partida" */
}
TEST_CASE("opções ajustam dentro dos limites e voltam para quem abriu") {
    /* Pausa → Opções; ajustar +1 seis vezes na sensibilidade → 10 e OpcoesMudaram; volume dos efeitos −1 × 9 → 0;
       tela cheia ajustar → true; voltar → SalvarOpcoes e tela Pausa */
}
TEST_CASE("semente: só dígitos, até 4294967295") {
    /* texto "12a3" → 123; texto "4294967295" a partir do vazio → 4294967295; mais "1" → continua 4294967295;
       apagar 10 vezes → vazio ("Semente fixa: aleatória") */
}
TEST_CASE("mouse em cima seleciona e clique confirma") { /* itemSobMouse 2 → selecionado 2; clique → ação do item 2 */ }
TEST_CASE("clique direito diminui um valor") { /* Opções, itemSobMouse 1, cliqueDireito → volumeEfeitos 7 */ }
```

- [ ] **Passo 2: Rodar e ver falhar.**
- [ ] **Passo 3: Implementar `Menus.cpp`.**
- [ ] **Passo 4: Rodar** — tudo passa.

---

### Tarefa 4: Telas desenhadas (puras)

**Arquivos:**
- Criar: `src/render/TelasMenu.h/.cpp`, `testes/teste_telas_menu.cpp`
- Modificar: `src/render/Letterbox.h/.cpp` (+ `contem`), `src/render/TelaInventario.cpp` (usa `contem`), `src/render/Hud.cpp` (rodapé do fim), `testes/teste_hud.cpp`

**Interfaces:**
- Consome: `ui::Menus` (Tarefa 3), `jogo::RegistroPlacar`, `render::ElementosHud`, `construirTexto`, `larguraTexto`, `adicionarRetangulo`.
- Produz:
  ```cpp
  bool contem(const Retangulo&, glm::vec2 ponto);  // Letterbox.h
  struct VistaMenu { std::string_view titulo; std::vector<std::string> itens; int selecionado; };
  std::vector<Retangulo> layoutMenu(int quantidade, int telaL, int telaA);  // um retângulo por item
  std::optional<int> itemSob(const std::vector<Retangulo>&, glm::vec2 ponto);
  ElementosHud construirMenu(const VistaMenu&, int telaL, int telaA);
  ElementosHud construirPlacar(const std::vector<jogo::RegistroPlacar>&, int telaL, int telaA);
  ```
- Aparência: painel escuro translúcido (`0,06 0,05 0,04 0,85`) no centro; título em escala 2 na cor latão (`0,85 0,62 0,20`), centralizado; itens em escala 1, centralizados, com 14 px entre linhas; o selecionado ganha uma faixa `0,30 0,26 0,20` e o prefixo `> `. Nas telas de jogo (Pausa e Opções durante a partida) o painel fica sobre a cena congelada.
- Placar: até 10 linhas `1. 1234  andar 5  nível 3  Destruído por um Colosso`, com a causa cortada com `…` para caber; sem registros: `Nenhuma partida ainda`; o item `Voltar` embaixo usa `layoutMenu(1, …)` deslocado para o rodapé (o retângulo devolvido é o do `Voltar`).
- Tela de fim: acrescentar o rodapé `Enter: nova partida   Esc: menu`.

- [ ] **Passo 1: Escrever os testes que falham**

```cpp
TEST_CASE("título do jogo cabe na tela baixa") {
    CHECK(render::larguraTexto("Abaixo da Cidade de Latão", 2) <= 480 - 16);
}
TEST_CASE("itens do menu centralizados e sem sobreposição") { /* layoutMenu(6, 480, 270): todos dentro da tela, mesmo centro x, y crescente */ }
TEST_CASE("item sob o mouse") {
    /* centro do retângulo 2 → 2; ponto (0, 0) → nullopt; com janelaParaTelaBaixa de uma janela 1920×1080
       desenhável 2400×1350 (escala 125%), o centro do item 1 convertido cai no item 1 */
}
TEST_CASE("menu destaca o selecionado") { /* construirMenu com selecionado 1: a faixa de destaque cobre layoutMenu(...)[1] */ }
TEST_CASE("placar: textos não saem da tela, causa longa é cortada") {
    /* 10 registros com causa de 80 caracteres: todo vértice de texto com x ≤ 480 */
}
TEST_CASE("placar vazio avisa") { /* o texto montado contém os glifos de "Nenhuma partida ainda" (comparar com construirTexto) */ }
TEST_CASE("tela de fim mostra como continuar") { /* teste_hud.cpp: mais glifos que antes; largura do rodapé ≤ 480 */ }
```

- [ ] **Passo 2: Rodar e ver falhar.**
- [ ] **Passo 3: Implementar.**
- [ ] **Passo 4: Rodar** — tudo passa (inclusive os testes do inventário, que agora usam `contem`).

---

### Tarefa 5: Piloto automático do `--demo`

**Arquivos:**
- Criar: `src/jogo/Demo.h/.cpp`, `testes/teste_demo.cpp`
- Modificar: `src/jogo/Partida.h` (se faltar algum acesso só de leitura), `CMakeLists.txt`

**Interfaces:**
- Consome: `Partida` (`andar()`, `jogador()`, `temChave()`, `inimigos()`, `aberturaComporta`), `mundo::distanciasNaGrade`, `mundo::celulaAFrente`, `mundo::temLinhaDeVisao`.
- Produz:
  ```cpp
  // Comandos que levam o jogador à válvula-chave (se houver tranca e ainda não a tiver) e depois ao elevador
  class PilotoDemo {
  public:
      Comandos comandar(const Partida& p, float dt);
  };
  ```
- Algoritmo:
  - O alvo é a célula da válvula (se `andar().chave` existir) ou a do elevador.
  - A cada chamada, calcular `distanciasNaGrade(grade, alvo, atravessaTrancada = temChave() || !chave, true)`. Escolher, entre os 4 vizinhos da célula atual, o de menor distância e mirar no centro dele.
  - Girar no máximo 0,08 rad por chamada. Andar para frente só com erro de ângulo < 0,5 rad.
  - Comporta fechada (ou trancada com a chave) a até 1,5 à frente: `usar` (no máximo a cada 0,5 s). Vizinho do elevador: virar para ele e `usar`.
  - Inimigo vivo a até 1,5, à frente (±30°) e visível: `atacar` com a arma branca ativa.
  - Andou menos de 0,3 em 3 s: passo lateral de 1 s, alternando o lado a cada vez.

- [ ] **Passo 1: Escrever o teste que falha**

```cpp
TEST_CASE("piloto chega ao elevador em 50 sementes") {
    for (uint32_t s = 1; s <= 50; ++s) {
        jogo::Partida p(s, 1);
        p.tornarInvencivel();
        jogo::PilotoDemo piloto;
        const float dt = 1.0f / 60.0f;
        for (int i = 0; i < 180 * 60 && p.trocasDeAndar() == 0; ++i) p.atualizar(piloto.comandar(p, dt), dt);
        INFO("semente ", s);
        CHECK(p.trocasDeAndar() == 1);
    }
}
TEST_CASE("piloto pega a válvula antes de descer num andar trancado") {
    /* uma semente com tranca (achar com --descrever, como nos cenários da Etapa 4): ao descer, a chave foi pega */
}
```

- [ ] **Passo 2: Rodar e ver falhar.**
- [ ] **Passo 3: Implementar `Demo.cpp`.**
- [ ] **Passo 4: Rodar** — passa em Debug e Release. Anotar o tempo do caso de 50 sementes; se passar de 10 s em Debug, reduzir para 20 sementes em Debug (`#ifdef NDEBUG`).

---

### Tarefa 6: Áudio (SDL_mixer) e sons provisórios

**Arquivos:**
- Criar: `src/plataforma/Audio.h/.cpp`, `ferramentas/gerar_sons.cpp`, `assets/sons/*.wav`
- Modificar: `cmake/Dependencias.cmake`, `CMakeLists.txt`, `assets/LICENCAS.txt`

**Interfaces:**
- Consome: `jogo::Som`, `nomeDoSom`, `ganhosEstereo` (Tarefa 2), `plataforma::caminhoAsset`, `plataforma::logar`.
- Produz:
  ```cpp
  class Audio {
  public:
      Audio();   // SDL_InitSubSystem(SDL_INIT_AUDIO), Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024), 32 canais;
                 // falhou: uma linha no log e o objeto fica mudo
      ~Audio();
      bool ativo() const;
      void carregar();  // assets/sons/<nome>.ogg, senão .wav; ausente ou inválido: log e silêncio para aquele som
      void tocar(jogo::Som, float volume, float pan);  // volume 0–1 × volume dos efeitos; canal livre; pan por Mix_SetPanning
      void tocarMusica();  // assets/sons/musica.ogg, senão .wav, em loop
      void definirVolumes(int efeitos, int musica);  // 0–10 → 0–128
  };
  ```
- `Dependencias.cmake`: `FetchContent_Declare(SDL2_mixer GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git GIT_TAG release-2.8.1 GIT_SHALLOW TRUE)`. Opções:
  - desligadas: `SDL2MIXER_VENDORED`, `SDL2MIXER_SAMPLES`, `SDL2MIXER_INSTALL`, `SDL2MIXER_OPUS`, `SDL2MIXER_MOD`, `SDL2MIXER_MIDI`, `SDL2MIXER_FLAC`, `SDL2MIXER_MP3`, `SDL2MIXER_WAVPACK`;
  - ligadas: `SDL2MIXER_VORBIS=STB` e `SDL2MIXER_WAVE`.

  Os nomes das opções devem ser conferidos no `CMakeLists.txt` do SDL_mixer baixado. O `Roguelike` liga com `SDL2_mixer::SDL2_mixer`, e o POST_BUILD copia `$<TARGET_FILE:SDL2_mixer::SDL2_mixer>` junto com o `SDL2.dll`.
- `gerar_sons.cpp`: alvo `gerar_sons` com `EXCLUDE_FROM_ALL`. Uso: `gerar_sons <pasta>`. Grava WAV PCM 16 bits mono a 22.050 Hz, com `std::mt19937` de semente fixa. Cada som é ruído filtrado ou senoide com envelope:
  - passos: estalo grave curto (metal com ressonância aguda);
  - golpe no ar: ruído com varredura;
  - acerto: batida metálica;
  - disparos: ruído forte com decaimento (o bacamarte é mais longo e grave; a Tesla é zumbido com estalos);
  - recarga: dois cliques;
  - arma vazia: clique seco;
  - dano: baque grave;
  - destruído: chiado de vapor e tilintar;
  - comporta: sopro de vapor de 0,5 s;
  - pegar: dois tons subindo;
  - nível: arpejo de três notas;
  - elevador: rangido grave de 1 s;
  - tiro do inimigo: disparo mais abafado;
  - menu: um clique curto (mover) e dois tons (confirmar);
  - `musica.wav`: 32 s, que fecham o loop sem emenda (zumbido de 55 Hz com harmônicos, batida metálica a cada 2 s, chiado de vapor a cada 8 s).
- `LICENCAS.txt`: "Sons e música provisórios gerados por `ferramentas/gerar_sons.cpp` (feitos para este projeto)".

- [ ] **Passo 1: Escrever o teste que falha** (em `teste_sons.cpp`; o CMake passa `PASTA_ASSETS="${CMAKE_SOURCE_DIR}/assets"` aos testes)

```cpp
TEST_CASE("cada som tem arquivo em assets/sons") {
    for (int i = 0; i < static_cast<int>(jogo::Som::TOTAL); ++i) {
        const std::string nome(jogo::nomeDoSom(static_cast<jogo::Som>(i)));
        INFO(nome);
        CHECK((std::filesystem::exists(std::filesystem::path(PASTA_ASSETS) / "sons" / (nome + ".wav")) ||
               std::filesystem::exists(std::filesystem::path(PASTA_ASSETS) / "sons" / (nome + ".ogg"))));
    }
}
```

- [ ] **Passo 2: Rodar e ver falhar** (os arquivos não existem).
- [ ] **Passo 3: Escrever `gerar_sons.cpp`, compilar e rodar:** `cmake --build --preset release --target gerar_sons`, depois `build\Release\gerar_sons.exe assets\sons`. Ouvir não dá; conferir o tamanho e a duração de cada arquivo com um script (cabeçalho WAV válido, 0,05 a 1,5 s; música de 32 s).
- [ ] **Passo 4: Implementar `Audio.cpp` e a dependência.** Rodar `.\ferramentas\compilar.ps1` e `.\ferramentas\compilar.ps1 release`; tudo passa e o `SDL2_mixer.dll` aparece em `build\Debug` e em `build\Release`.
- [ ] **Passo 5: Robustez (Foco 1).** Com o laço da Tarefa 7 pronto, executar o jogo com `--captura` em três condições e conferir o `log.txt` em cada uma:
  - (a) `SDL_AUDIODRIVER=dummy`: sem erro;
  - (b) `SDL_AUDIODRIVER=inexistente`: uma linha "Áudio indisponível…" e a captura sai normal;
  - (c) uma cópia truncada de `acerto.wav` dentro de `build\Release\assets`: uma linha citando o arquivo e a captura sai normal.

  Restaurar o arquivo no fim. (Este passo roda no fim da Tarefa 7.)

---

### Tarefa 7: Entrada, janela e laço com telas, som e opções

**Arquivos:**
- Modificar: `src/plataforma/Entrada.h/.cpp`, `src/plataforma/Janela.h/.cpp`, `src/main.cpp`, `ferramentas/capturar.ps1`

**Interfaces:**
- Consome: tudo das Tarefas 1–6.
- Produz:
  - `Entrada`:
    - `void definirSensibilidade(float radPorPixel)` substitui a constante.
    - `bool pediuPausa()`: Esc com o mouse capturado e sem inventário, ou foco perdido durante o jogo; consome o pedido.
    - `void modoMenu(bool)`: no modo menu, o mouse fica livre e o clique não captura.
    - `ui::EntradaMenu coletarMenu()`: setas/W/S, A/D/setas laterais, Enter, Esc, Backspace, `SDL_TEXTINPUT`, e os cliques esquerdo e direito; tudo acumulado desde a última chamada.
    - `void entrarNoJogo()`: sai do modo menu e captura o mouse.
    - Esc com o inventário aberto fecha o inventário.
  - `Janela::definirTelaCheia(bool)`: `SDL_SetWindowFullscreen(janela_, sim ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)`.
  - Opções novas do `main`:
    - `--demo <semente>`: começa direto no jogo, invencível, um passo por quadro, sem ler comandos;
    - `--mudo`: não abre o áudio.
- Laço:
  1. Ao abrir, ler o `config.txt` da `pastaDeDados()`. Se estava corrompido, gravar o padrão. Aplicar a sensibilidade, os volumes e a tela cheia (o `--demo` e o `--captura` ignoram a tela cheia).
  2. O `Audio` toca a música desde o menu.
  3. Título fixo: `Abaixo da Cidade de Latão`.
  4. **Principal, Placar, Opções (fora da partida):** a partida de fundo (semente 1, andar 1) com o ângulo girando 0,15 rad/s, sem HUD, com o menu por cima.
  5. **Jogo:** como hoje. `pediuPausa()` → `menus.abrir(Pausa)` via `focoPerdido()` ou Esc. Os sons de `partida->sons()` são lidos depois de cada passo e mixados com a posição e o ângulo do jogador.
  6. **Pausa / Opções na partida:** a lógica para (como no inventário) e a cena aparece congelada sob o painel.
  7. **Fim:** como hoje. Esc → Principal.
  8. **Ações:**
     - `NovoJogo`: semente pela regra das decisões, `entrarNoJogo()`;
     - `Continuar`: `entrarNoJogo()`;
     - `Abandonar`: volta à partida de fundo e ao Principal;
     - `Sair`: fecha;
     - `OpcoesMudaram`: aplica na hora (sensibilidade, volumes, tela cheia);
     - `SalvarOpcoes`: `gravarConfig` e log se falhar (o arquivo que estava bloqueado na leitura não é gravado);
     - `AbrirPlacar`: `lerPlacar`.
  9. **Sons do menu:** `MenuMover` quando a seleção muda; `MenuConfirmar` em confirmar ou clique.
  10. **`--demo`:**
      - a cada 120 passos, salvar `capturas\demo_<semente>_<nn>.bmp` (na pasta do exe; criar a pasta);
      - ao descer, salvar mais uma, registrar no log "demo: chegou ao elevador em X s" e fechar com código 0;
      - sem descer em 180 s, registrar no log e fechar com código 2;
      - não grava placar nem opções.
- `capturar.ps1`: definir `$env:SDL_AUDIODRIVER = "dummy"` antes de abrir o jogo.

- [ ] **Passo 1: Implementar a `Entrada` e a `Janela`; compilar.**
- [ ] **Passo 2: Implementar o laço; compilar em Debug e Release, com todos os testes passando.**
- [ ] **Passo 3: Rodar o `--demo` em 3 sementes**, com `SDL_AUDIODRIVER=dummy` e `--dados <scratchpad>`: `build\Release\Roguelike.exe --demo 12345 --dados <scratchpad>\dados`. Esperado: código 0, de 3 a 90 capturas em `build\Release\capturas`, e a linha "demo: chegou ao elevador" no log. Abrir 3 capturas de cada semente e conferir o desenho.
- [ ] **Passo 4: Capturas das telas** (`capturar.ps1` ganha a opção `--tela principal|opcoes|placar|pausa`, que só vale com `--captura` e abre a tela pedida): menu principal, opções, placar (com o placar de teste em `--dados`), pausa sobre a cena e a tela de fim com o rodapé. Conferir na imagem: textos dentro da tela, acentos e o título inteiro.
- [ ] **Passo 5: Rodar o Passo 5 da Tarefa 6** (robustez do áudio) e um teste de `config.txt` corrompido de verdade: gravar `sensibilidade=99` em `<scratchpad>\dados\config.txt`, abrir com `--dados` e `--captura`, e conferir que surgiram o `.corrompido` e o novo padrão.

---

### Tarefa 8: Pacote para os amigos

**Arquivos:**
- Criar: `ferramentas/empacotar.ps1`, `docs/COMO_JOGAR.txt`
- Modificar: `.gitignore` (`pacote/`), `docs/superpowers/specs/2026-11-01-roguelike-design.md` (seção 14: nomes do pacote, da pasta e do exe), `assets/LICENCAS.txt` (SDL_mixer: zlib; stb_vorbis: domínio público/MIT)

**Interfaces:**
- `empacotar.ps1` (com BOM, como os outros `.ps1`):
  1. Roda `.\ferramentas\compilar.ps1 release` e para se falhar.
  2. Recria `pacote\AbaixoDaCidadeDeLatao\` com:
     - `AbaixoDaCidadeDeLatao.exe` (cópia de `build\Release\Roguelike.exe`);
     - `SDL2.dll` e `SDL2_mixer.dll`;
     - `assets\` (cópia de `build\Release\assets`);
     - `COMO_JOGAR.txt` (de `docs\`, gravado em UTF-8 **com BOM** para o Bloco de Notas);
     - `LICENCAS.txt` (de `assets\`).
  3. Confere com `dumpbin /dependents` (do VS 2026) que o exe e as duas DLLs não dependem de `VCRUNTIME*`, `MSVCP*` nem `api-ms-win-crt-*`; se dependerem, para com a lista.
  4. Gera `pacote\AbaixoDaCidadeDeLatao-v1.zip` com `Compress-Archive`.
  5. Teste de fumaça: extrai o zip numa pasta temporária, roda `AbaixoDaCidadeDeLatao.exe --demo 12345 --dados <temp>\dados` com `SDL_AUDIODRIVER=dummy`, exige código 0 e capturas criadas ali, e apaga a pasta temporária.
  6. Imprime o tamanho do zip.
- `COMO_JOGAR.txt` (texto final; o executor só corrige se algo mudar na implementação):

```
ABAIXO DA CIDADE DE LATÃO
=========================

Sob uma cidade vitoriana movida a vapor, túneis de esgoto, fábricas e minas
estão cheios de autômatos que perderam o juízo. Desça o mais fundo que conseguir.
Se morrer, recomeça do zero — o placar guarda as 10 melhores descidas.

COMO ABRIR
1. Clique com o botão direito no arquivo .zip e escolha "Extrair tudo".
2. Abra a pasta extraída e dê dois cliques em AbaixoDaCidadeDeLatao.exe.
3. Se o Windows mostrar "O Windows protegeu o computador", clique em
   "Mais informações" e depois em "Executar assim mesmo". (Aparece porque o
   jogo é caseiro e não tem assinatura digital.)
Não precisa instalar nada. Para desinstalar, é só apagar a pasta.

CONTROLES
W A S D ........... andar
Mouse ............. olhar e mirar
Clique esquerdo ... atacar / atirar
Q ou roda ......... trocar entre arma branca e arma de fogo
R ................. recarregar
E ................. abrir comporta, pegar item, descer no elevador
I ................. inventário (clique: usar/equipar; direito: largar; arrastar: trocar)
1 2 3 4 ........... usar os itens da primeira linha do inventário
Tab ............... mapa
Esc ............... pausa (opções e sair)

DICAS
- A munição é pouca: use a chave inglesa nos inimigos fracos.
- Comporta com cadeado precisa da válvula-chave, que está em algum lugar do andar.
- O elevador leva ao próximo andar. Quanto mais fundo, melhores os itens.
- Itens em azul são modificados; em dourado, obras-primas.

OPÇÕES, PLACAR E PROBLEMAS
- Sensibilidade do mouse, volumes e tela cheia ficam no menu Opções.
- O placar e as opções ficam em %APPDATA%\Roguelike (cole isso na barra do Explorador).
- Se o jogo avisar que a placa de vídeo não é suportada, atualize o driver de vídeo.
- Se algo der errado, o arquivo log.txt, ao lado do jogo, ajuda a descobrir o motivo.

Créditos e licenças de arte, som e bibliotecas: LICENCAS.txt.
```

- [ ] **Passo 1: Escrever `COMO_JOGAR.txt` e `empacotar.ps1`.**
- [ ] **Passo 2: Rodar `.\ferramentas\empacotar.ps1`.** Esperado: "dependências ok", "teste de fumaça ok" e o tamanho do zip. Listar o conteúdo do zip.
- [ ] **Passo 3: Atualizar a seção 14 da spec e o `.gitignore`.**

---

### Tarefa 9: Sons de pacotes livres (só com ok)

Esta tarefa só começa depois de perguntar ao Nidragnnar, já com nomes, links, licenças e tamanhos. Candidatos, a confirmar na hora (preferência CC0):
- Kenney.nl: "Impact Sounds" (passos e acertos), "Sci-Fi Sounds" (Tesla, vapor), "Interface Sounds" (menu), "RPG Audio";
- OpenGameArt.org: disparos de pistola e escopeta e uma música ambiente industrial em loop.

Sem ok, ou para o que faltar, ficam os provisórios da Tarefa 6.

- [ ] **Passo 1: Perguntar, com a lista concreta.**
- [ ] **Passo 2: Com o ok, baixar para `arte_fonte\sons\`** (fora do git), escolher os arquivos e convertê-los ou cortá-los se preciso. Pôr o `.ogg` em `assets/sons/` e apagar o `.wav` correspondente. Registrar cada um em `LICENCAS.txt` com autor, link e licença.
- [ ] **Passo 3: Compilar** — o teste "cada som tem arquivo" continua passando. Gerar o pacote de novo.

---

### Tarefa 10: Verificação, cenários, revisão e entrega

- [ ] **Passo 1:** `.\ferramentas\compilar.ps1` e `.\ferramentas\compilar.ps1 release`, com todos os testes passando (anotar a contagem).
- [ ] **Passo 2:** `--demo` em 5 sementes, código 0 em todas. Conferir as capturas.
- [ ] **Passo 3: Revisão independente** (agente em segundo plano, programas próprios em Release, `--dados` no scratchpad, `SDL_AUDIODRIVER=dummy`). Pedir atenção ao Foco de revisão. Reclassificar os achados por impacto, corrigir os que valem a pena (com teste) e listar o resto como pendência.
- [ ] **Passo 4: Backup** de `%APPDATA%\Roguelike` (placar e config) numa pasta do scratchpad antes de deixar o jogo com o Nidragnnar; avisar que o jogo agora grava `config.txt` lá.
- [ ] **Passo 5: Cenários** em `build\Release` (os `.bat` da Etapa 4 continuam valendo), mais:
  - `7_Menu_normal.bat`: abre pelo menu, como um amigo;
  - `8_Demo.bat`: `--demo 12345`;
  - `9_Config_corrompido.bat`: usa `--dados` numa pasta de teste com um `config.txt` estragado.

  O pacote fica pronto em `pacote\`.
- [ ] **Passo 6: Tabela do que testar**, por tela e cenário. Itens:
  - sons conferidos um a um, com volume e lado do som;
  - música em loop sem emenda audível;
  - Esc e pausa, Alt-Tab no meio da luta;
  - Esc fecha o inventário;
  - opções que valem na hora e voltam depois de reabrir;
  - tela cheia com escala de 125%/150%;
  - semente fixa repete o andar;
  - abandonar não grava no placar;
  - o zip aberto em outra pasta (ou outro PC) funciona.
- [ ] **Passo 7:** Deixar o jogo aberto pelo `7_Menu_normal.bat` e perguntar "Depois de testar, faço o commit e envio?". Listar o que foi testado, o que não foi (o ouvido é dele: a qualidade dos sons provisórios, o volume relativo, o pan) e o que é estimativa (as constantes desta etapa).
