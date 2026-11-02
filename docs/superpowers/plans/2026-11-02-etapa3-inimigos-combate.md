# Etapa 3 — Inimigos e combate básico: Plano de implementação

> **Para agentes:** SUB-SKILL OBRIGATÓRIA: usar superpowers:subagent-driven-development (recomendado) ou superpowers:executing-plans para executar este plano tarefa por tarefa. Os passos usam caixas (`- [ ]`) para acompanhamento.

**Objetivo:** povoar os andares com os 4 inimigos da spec, que percebem, perseguem e atacam o jogador; dar ao jogador a chave inglesa e o revólver (com carga, recarga e munição de reserva); aplicar dano nos dois sentidos, morte permanente, tela de fim e placar dos 10 melhores.

**Arquitetura:** a lógica nova mora em `jogo/` e é testada sem janela: `Inimigos` (tabela, população, IA), `Armas` (estado e ataques do jogador), `Projeteis` e `Placar` (pontuação e arquivo). A `Partida` passa a orquestrar tudo isso e ganha os estados "jogando" e "morto". No `render/` entram um pipeline 2D com textura (arma na mão, ícones e texto), uma fonte bitmap e os construtores puros do HUD, da tela de fim e da animação dos inimigos.

**Tecnologia:** a mesma das etapas 1 e 2.

**Spec:** `docs/superpowers/specs/2026-11-01-roguelike-design.md` (seções 4, 5, 7, 8 "HUD" e "Texto", 11, 13 etapa 3)

## Restrições globais

- Tudo das etapas 1 e 2 continua valendo.
- **Jogador:** vida 30; dano recebido = `max(1, dano − defesa)`; começa com chave inglesa + revólver carregado (6) + **12 balas** de reserva.
- **Chave inglesa:** dano 4–6, intervalo 0,35 s, alcance 1,1, arco de 60° à frente; empurra o inimigo e interrompe o ataque dele; barulho até **4** células.
- **Revólver:** dano 7–10, intervalo 0,4 s, carga 6, recarga 1,5 s, balas, tiro instantâneo com dispersão de **2°**; barulho até **10** células; reserva máxima de balas **48**.
- **Inimigos** (vida / dano / velocidade / XP): Rato mecânico 6 / 2–3 / rápida / 3, bandos de 2–4; Autômato operário 16 / 4–6 / média / 8; Atirador da Guilda 10 / 3–5 (projétil a **10 células/s**) / média / 8, mantém **3 a 6** células; Colosso a vapor 40 / 8–12 / lenta / 20, **defesa 2**, golpe demorado.
- **Escala por andar:** vida e dano × `(1 + 0,15 × (andar − 1))` (arredondar para o inteiro mais próximo, mínimo 1).
- **Quantidade:** de `6 + 2 × andar` a `10 + 2 × andar`; Atiradores a partir do andar 2, Colossos a partir do 3; nunca na sala inicial.
- **Estados:** Parado → (vê o jogador por linha de visão até **10** células, ou ouve barulho no alcance) → Perseguindo (busca em largura recalculada a cada **0,5 s**) → Atacando → Destruído (some depois de **10 s**). Abrem comportas comuns, não as trancadas.
- **Pontuação** = `100 × andar + 10 × inimigos destruídos + engrenagens + 50 × nível`; **10 melhores** em `%APPDATA%\Roguelike\placar.txt`; arquivo corrompido vira `.corrompido` e é recriado.
- Ao levar dano, a tela pisca em vermelho e a arma na mão balança; o clarão do disparo ilumina a cena por **0,05 s**.
- **Constantes desta etapa** (não fixadas pela spec; ajustar após o teste do Nidragnnar):
  - velocidades: rápida **3,5**, média **2,0**, lenta **1,2** células/s (jogador: 3,0);
  - corpo a corpo dos inimigos: começam o golpe a **0,9** de distância; preparo / recuperação: rato 0,3 / 0,6 s, autômato 0,5 / 0,7 s, colosso 0,9 / 1,0 s; o dano sai no fim do preparo se o jogador ainda estiver a até **1,1** e visível;
  - Atirador: preparo 0,4 s, intervalo entre tiros 1,5 s, projétil com raio 0,1;
  - raio dos inimigos 0,25 (spec); inimigos a menos de 0,5 um do outro se afastam;
  - empurrão da chave inglesa **0,3** células;
  - defesa do jogador **1** (casaco de couro inicial) até o inventário da etapa 4; nível **1** e engrenagens **0** até a etapa 4;
  - pesos de sorteio: andar 1 rato 60 / autômato 40; andar 2 rato 40 / autômato 35 / atirador 25; andar 3+ rato 30 / autômato 30 / atirador 25 / colosso 15. Um sorteio de "rato" gera um bando de 2–4 (sem passar do total).
- **Texto na tela:** fonte bitmap 8×8 com os acentos do português, desenhada na tela baixa (480×270).
- **Downloads:** a fonte e, se o Nidragnnar escolher, pacotes de sprites de inimigos, **só com ok**. Sem arte aprovada, sprites provisórios gerados por script.
- **Dados do jogador:** testes automáticos usam a opção `--dados <pasta>` apontando para o scratchpad; nunca escrever no `%APPDATA%` real durante os testes (se for inevitável, fazer backup antes e restaurar depois).
- **Commit:** um único, no fim, com o "sim" do Nidragnnar.

## Decisões de projeto (registradas aqui)

- **HUD mínimo nesta etapa:** barra de vida, carga/reserva de balas, arma na mão e a mensagem curta; o HUD completo (XP, engrenagens, manômetro estilizado) fica para a etapa 4. As mensagens saem do título da janela e vão para a tela.
- **Tela de fim sem menu:** Enter começa nova partida (semente aleatória); fechar a janela sai. Menu principal e tela de placar são da etapa 5.
- **Percepção sem cone de visão:** o inimigo "vê" em qualquer direção (como a exploração). Depois de acordar, não perde o jogador de vista.
- **Campo de fluxo:** uma única busca em largura a partir da célula do jogador, recalculada a cada 0,5 s, serve a todos os inimigos (cada um vai para a vizinha com distância menor).
- **Atacar com o botão segurado:** enquanto o botão esquerdo estiver apertado, a arma ataca sempre que o intervalo permitir. O clique que captura o mouse não ataca.

## Foco de revisão

1. **Engarrafamento em porta e corredor de 1 célula** (vários inimigos, comporta fechada no caminho): ninguém atravessa parede nem fica tremendo para sempre; a comporta é aberta e todos passam. → testes `inimigo abre a comporta e passa` e `dois inimigos no corredor não se sobrepõem` (Tarefa 4).
2. **Morrer com vários golpes no mesmo passo / continuar levando dano depois de morto:** a morte é registrada uma vez, a causa é a do golpe fatal, o placar é gravado uma vez. → teste `morte registra uma vez e congela a partida` (Tarefa 6).
3. **Placar ausente, corrompido ou pasta sem permissão de escrita:** sem travar; corrompido vira `.corrompido`; falha de escrita vai para o log. → testes do placar (Tarefa 7).
4. **Revólver sem munição, recarregar cheio, trocar de arma no meio da recarga, atacar segurando o botão:** → testes da Tarefa 5.
5. **Descer de andar com inimigos atacando e projéteis no ar:** tudo é limpo; o novo andar recebe a sua população. → teste `descer limpa inimigos e projéteis` (Tarefa 6).

---

## Estrutura de arquivos

```
src/mundo/Consultas.cpp         correção da linha de visão (pendência da etapa 2)
src/mundo/Aleatorio.h/.cpp      sortear / sortear01 / sortearReal (tirados do Gerador para reaproveitar)
src/mundo/Grade.*, Gerador.*    marcas de texto r a g c (inimigos nos mapas de teste)
src/jogo/Inimigos.h/.cpp        tipos, tabela, escala, Inimigo, gerarInimigos
src/jogo/IA.h/.cpp              percepção, campo de fluxo, movimento, ataques dos inimigos
src/jogo/Projeteis.h/.cpp       tiros do Atirador
src/jogo/Armas.h/.cpp           estado das armas do jogador, golpe e tiro
src/jogo/Placar.h/.cpp          pontuação, leitura/gravação do placar
src/jogo/Comandos.h             + atacar, trocarArma, recarregar
src/jogo/Partida.h/.cpp         vida, inimigos, projéteis, armas, morte, clarão, dano recebido
src/plataforma/Caminhos.*       pastaDeDados() (%APPDATA%\Roguelike ou --dados)
src/plataforma/Entrada.*        botão esquerdo, Q, roda, R, Enter
src/render/Desenho2D.*          cor com alfa (vec4) + mistura; Desenho2DTextura (quadros com textura)
src/render/Texto.h/.cpp         fonte bitmap: glifos e layout (puro)
src/render/Hud.h/.cpp           barra de vida, munição, mensagem, arma na mão, flash vermelho, tela de fim (puro)
src/render/AnimacaoInimigos.h/.cpp  camada do sprite pelo tipo, estado e tempo (puro)
src/render/Texturas.*           + lado da textura como parâmetro
src/main.cpp                    liga tudo; --dados, --invencivel
assets/shaders/tex2d.vert/.frag, masmorra.frag (+ uClarao)
assets/fonte/fonte.png          16 colunas de glifos 8×8 na ordem de render::CARACTERES_FONTE
assets/sprites/*.png            inimigos (7 quadros cada), projétil
assets/armas/*.png              chave inglesa e revólver na mão (128×128)
ferramentas/gerar_sprites.ps1   + inimigos, projétil e armas provisórios
ferramentas/gerar_fonte.ps1     rasteriza a fonte escolhida em assets/fonte/fonte.png
testes/teste_inimigos.cpp, teste_ia.cpp, teste_armas.cpp, teste_projeteis.cpp, teste_placar.cpp,
testes/teste_texto.cpp, teste_hud.cpp, teste_animacao.cpp (+ casos novos em teste_consultas.cpp e teste_partida.cpp)
```

---

### Tarefa 1: Linha de visão correta e sorteio compartilhado

**Arquivos:**
- Modificar: `src/mundo/Consultas.cpp`, `src/mundo/Gerador.cpp`
- Criar: `src/mundo/Aleatorio.h/.cpp`
- Testar: `testes/teste_consultas.cpp`

**Interfaces:**
- Produz:
  - `int mundo::sortear(std::mt19937& rng, int min, int max)`; `float mundo::sortear01(std::mt19937& rng)`; `float mundo::sortearReal(std::mt19937& rng, float min, float max)` (os dois primeiros saem do `namespace` anônimo do Gerador, sem mudar o resultado — os testes do gerador continuam passando com as mesmas sementes)
  - `temLinhaDeVisao`: quando a trava de arredondamento dispara, devolve `cx == alvoX && cy == alvoY` (em vez de `true`)

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("linha de visão entre pontos quaisquer, não só centros") {
    auto m = mundo::carregarDeTexto({"########", "#@.....#", "#..#...#", "#......#", "########"});
    CHECK_FALSE(mundo::temLinhaDeVisao(m.grade, {1.2f, 2.7f}, {5.9f, 2.2f}));   // passa pela parede (3,2)
    CHECK(mundo::temLinhaDeVisao(m.grade, {1.2f, 1.3f}, {6.8f, 1.6f}));
}
TEST_CASE("linha de visão é simétrica em 2000 pares aleatórios") {
    auto m = mundo::carregarDeTexto({"##########", "#@...#...#", "#.##...#.#", "#....#...#", "##########"});
    std::mt19937 rng(7);
    for (int i = 0; i < 2000; ++i) {
        glm::vec2 a{mundo::sortearReal(rng, 1.01f, 8.99f), mundo::sortearReal(rng, 1.01f, 3.99f)};
        glm::vec2 b{mundo::sortearReal(rng, 1.01f, 8.99f), mundo::sortearReal(rng, 1.01f, 3.99f)};
        if (m.grade.solida(int(a.x), int(a.y)) || m.grade.solida(int(b.x), int(b.y))) continue;
        REQUIRE(mundo::temLinhaDeVisao(m.grade, a, b) == mundo::temLinhaDeVisao(m.grade, b, a));
    }
}
TEST_CASE("sortear fica dentro dos limites") {
    std::mt19937 rng(1);
    for (int i = 0; i < 1000; ++i) {
        int v = mundo::sortear(rng, 2, 4);
        REQUIRE((v >= 2 && v <= 4));
        float r = mundo::sortearReal(rng, -1.0f, 1.0f);
        REQUIRE((r >= -1.0f && r <= 1.0f));
    }
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação (`sortearReal` não existe).
- [ ] **Passo 3: Implementar `Aleatorio` e a correção da linha de visão; o Gerador passa a usar `Aleatorio.h`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa (inclusive os 1000 andares do gerador, prova de que o sorteio não mudou). Se a simetria falhar, corrigir a linha de visão (não o teste).

---

### Tarefa 2: Inimigos — tabela, escala e população

**Arquivos:**
- Modificar: `src/mundo/Grade.h/.cpp` (marcas), `src/mundo/Gerador.h/.cpp` (`Andar::marcas`)
- Criar: `src/jogo/Inimigos.h/.cpp`
- Testar: `testes/teste_inimigos.cpp`

**Interfaces:**
- Consome: `Andar`, `Sala`, `sementeDoAndar` (etapa 2); `Aleatorio` (Tarefa 1).
- Produz:
  - `MapaCarregado::marcas` e `Andar::marcas`: `std::vector<std::pair<char, glm::vec2>>` — letras `r` `a` `g` `c` no texto viram chão + marca no centro da célula (só mapas de teste)
  - `enum class jogo::TipoInimigo { Rato, Automato, Atirador, Colosso };`
  - `enum class jogo::EstadoInimigo { Parado, Perseguindo, Preparando, Recuperando, Destruido };` (Preparando/Recuperando = "Atacando" da spec, dividido em preparo do golpe e recuperação)
  - `struct jogo::DadosInimigo { const char* nome; int vida; int danoMin, danoMax; float velocidade; int defesa; int xp; float preparo, recuperacao; bool aDistancia; };`
  - `const jogo::DadosInimigo& jogo::dados(TipoInimigo t);` (nomes: "Rato mecânico", "Autômato operário", "Atirador da Guilda", "Colosso a vapor")
  - `int jogo::escalar(int valor, int andar);` = `max(1, round(valor * (1 + 0.15 * (andar - 1))))`
  - `struct jogo::Inimigo { TipoInimigo tipo; glm::vec2 posicao; int vida; int vidaMaxima; int danoMin, danoMax; EstadoInimigo estado = EstadoInimigo::Parado; float tempoNoEstado = 0; float tempoAteAtaque = 0; glm::vec2 empurrao{0}; };`
  - `jogo::Inimigo jogo::criarInimigo(TipoInimigo t, glm::vec2 posicao, int andar);` (vida e dano já escalados)
  - `std::vector<jogo::Inimigo> jogo::gerarInimigos(const mundo::Andar& andar, uint32_t semente);` — rng = `mt19937(sementeDoAndar(semente, andar.numero) ^ 0x51EDu)`; quantidade, pesos e bandos das Restrições globais; posição: célula de `Chao` sorteada dentro de uma sala que não é a sala 0, longe 0,6 de outro inimigo, nunca no elevador; até 200 tentativas por inimigo (se falhar, desiste daquele).
  - `std::vector<jogo::Inimigo> jogo::inimigosDasMarcas(const mundo::Andar& andar);` (`r` Rato, `a` Automato, `g` Atirador, `c` Colosso)

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("escala por andar") {
    CHECK(jogo::escalar(10, 1) == 10);
    CHECK(jogo::escalar(10, 3) == 13);    // 10 × 1,30
    CHECK(jogo::escalar(40, 5) == 64);    // 40 × 1,60
    CHECK(jogo::escalar(2, 1) == 2);
}
TEST_CASE("inimigo criado com a tabela da spec") {
    auto c = jogo::criarInimigo(jogo::TipoInimigo::Colosso, {5, 5}, 1);
    CHECK(c.vida == 40);
    CHECK(jogo::dados(jogo::TipoInimigo::Colosso).defesa == 2);
    CHECK(jogo::dados(jogo::TipoInimigo::Rato).xp == 3);
    CHECK(c.estado == jogo::EstadoInimigo::Parado);
}
TEST_CASE("quantidade de inimigos por andar") {
    for (uint32_t s = 1; s <= 100; ++s)
        for (int andar : {1, 2, 5}) {
            auto a = mundo::gerarAndar(s, andar);
            auto v = jogo::gerarInimigos(a, s);
            INFO("semente " << s << " andar " << andar);
            REQUIRE(int(v.size()) >= 6 + 2 * andar - 2);   // tolera até 2 que não acharam lugar
            REQUIRE(int(v.size()) <= 10 + 2 * andar);
        }
}
TEST_CASE("tipos respeitam o andar mínimo") {
    for (uint32_t s = 1; s <= 100; ++s) {
        for (auto& i : jogo::gerarInimigos(mundo::gerarAndar(s, 1), s)) {
            REQUIRE(i.tipo != jogo::TipoInimigo::Atirador);
            REQUIRE(i.tipo != jogo::TipoInimigo::Colosso);
        }
        for (auto& i : jogo::gerarInimigos(mundo::gerarAndar(s, 2), s)) REQUIRE(i.tipo != jogo::TipoInimigo::Colosso);
    }
}
TEST_CASE("nenhum inimigo na sala inicial, em parede ou no elevador") {
    for (uint32_t s = 1; s <= 100; ++s) {
        auto a = mundo::gerarAndar(s, 3);
        for (auto& i : jogo::gerarInimigos(a, s)) {
            REQUIRE_FALSE(a.salas[0].contem(int(i.posicao.x), int(i.posicao.y)));
            REQUIRE(a.grade.em(int(i.posicao.x), int(i.posicao.y)) == mundo::Celula::Chao);
        }
    }
}
TEST_CASE("mesma semente, mesmos inimigos") {
    auto a = mundo::gerarAndar(9, 2);
    auto v1 = jogo::gerarInimigos(a, 9), v2 = jogo::gerarInimigos(a, 9);
    REQUIRE(v1.size() == v2.size());
    for (size_t i = 0; i < v1.size(); ++i) CHECK(v1[i].posicao == v2[i].posicao);
}
TEST_CASE("marcas do texto viram inimigos") {
    auto a = mundo::andarDeTexto({"######", "#@rac#", "#..g.#", "######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    REQUIRE(v.size() == 4);
    CHECK(v[0].tipo == jogo::TipoInimigo::Rato);
    CHECK(v[0].posicao == glm::vec2(2.5f, 1.5f));
    CHECK(a.grade.em(2, 1) == mundo::Celula::Chao);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar marcas e `Inimigos`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 3: Projéteis do Atirador

**Arquivos:**
- Criar: `src/jogo/Projeteis.h/.cpp`
- Testar: `testes/teste_projeteis.cpp`

**Interfaces:**
- Consome: `mundo::Grade`, `temLinhaDeVisao`.
- Produz:
  - `constexpr float jogo::VELOCIDADE_PROJETIL = 10.0f, jogo::RAIO_PROJETIL = 0.1f;`
  - `struct jogo::Projetil { glm::vec2 posicao; glm::vec2 velocidade; int dano; jogo::TipoInimigo autor; };`
  - `struct jogo::ResultadoProjeteis { int dano = 0; std::optional<TipoInimigo> autor; };`
  - `jogo::ResultadoProjeteis jogo::atualizarProjeteis(std::vector<Projetil>& p, const mundo::Grade& g, glm::vec2 jogador, float raioJogador, float dt)` — move cada um em subpassos de no máximo 0,1; some ao entrar em célula sólida ou ao encostar no jogador (`distância < raioJogador + RAIO_PROJETIL`), somando o dano bruto (a defesa é aplicada pela Partida)

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("projétil anda 10 células por segundo") {
    auto m = mundo::carregarDeTexto({"####################", "#@.................#", "####################"});
    std::vector<jogo::Projetil> p{{{2.5f, 1.5f}, {10.0f, 0.0f}, 4, jogo::TipoInimigo::Atirador}};
    jogo::atualizarProjeteis(p, m.grade, {1.5f, 1.5f}, 0.25f, 0.5f);
    REQUIRE(p.size() == 1);
    CHECK(p[0].posicao.x == doctest::Approx(7.5f));
}
TEST_CASE("projétil some na parede") {
    auto m = mundo::carregarDeTexto({"######", "#@...#", "######"});
    std::vector<jogo::Projetil> p{{{2.5f, 1.5f}, {10.0f, 0.0f}, 4, jogo::TipoInimigo::Atirador}};
    jogo::atualizarProjeteis(p, m.grade, {1.5f, 1.5f}, 0.25f, 1.0f);
    CHECK(p.empty());
}
TEST_CASE("projétil acerta o jogador e informa o autor") {
    auto m = mundo::carregarDeTexto({"#######", "#@....#", "#######"});
    std::vector<jogo::Projetil> p{{{5.5f, 1.5f}, {-10.0f, 0.0f}, 4, jogo::TipoInimigo::Atirador}};
    auto r = jogo::atualizarProjeteis(p, m.grade, {1.5f, 1.5f}, 0.25f, 1.0f);
    CHECK(r.dano == 4);
    CHECK(r.autor == jogo::TipoInimigo::Atirador);
    CHECK(p.empty());
}
TEST_CASE("projétil rápido não atravessa o jogador num passo grande") {
    auto m = mundo::carregarDeTexto({"#########", "#@......#", "#########"});
    std::vector<jogo::Projetil> p{{{7.5f, 1.5f}, {-10.0f, 0.0f}, 3, jogo::TipoInimigo::Atirador}};
    auto r = jogo::atualizarProjeteis(p, m.grade, {3.5f, 1.5f}, 0.25f, 0.6f);   // anda 6 células de uma vez
    CHECK(r.dano == 3);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Projeteis`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 4: IA dos inimigos

**Arquivos:**
- Criar: `src/jogo/IA.h/.cpp`
- Testar: `testes/teste_ia.cpp`

**Interfaces:**
- Consome: `Inimigo`, `dados` (Tarefa 2); `Projetil` (Tarefa 3); `temLinhaDeVisao`, `distanciasNaGrade`, `moverComColisao` (etapas 1 e 2).
- Produz:
  - `constexpr float jogo::ALCANCE_VISAO = 10.0f, jogo::RECALCULO_CAMINHO = 0.5f, jogo::INICIO_GOLPE = 0.9f, jogo::ALCANCE_GOLPE = 1.1f, jogo::RAIO_INIMIGO = 0.25f, jogo::TEMPO_DESTROCOS = 10.0f;`
  - `struct jogo::MundoIA { const mundo::Grade& grade; glm::vec2 jogador; const std::vector<int>& campo; std::mt19937& rng; };` (`campo` = `distanciasNaGrade` a partir da célula do jogador, sem atravessar tranca)
  - `struct jogo::EventosIA { int dano = 0; std::optional<TipoInimigo> autor; std::vector<glm::ivec2> comportasParaAbrir; std::vector<Projetil> tiros; };`
  - `void jogo::acordarPorBarulho(std::vector<Inimigo>& v, glm::vec2 origem, float alcance);` (Parado → Perseguindo dentro do alcance, mesmo sem linha de visão)
  - `jogo::EventosIA jogo::atualizarInimigos(std::vector<Inimigo>& v, const MundoIA& m, float dt);`
- **Regras por passo, para cada inimigo:**
  - `Destruido`: só conta o tempo (a Partida remove depois de `TEMPO_DESTROCOS`).
  - `Parado`: acorda se `distância ≤ ALCANCE_VISAO` e houver linha de visão.
  - Empurrão: se `empurrao` não é zero, aplica com `moverComColisao` (raio do inimigo) e zera.
  - **Corpo a corpo** (Rato, Autômato, Colosso): `Perseguindo` → se `distância ≤ INICIO_GOLPE` e linha de visão, vai para `Preparando`; senão anda pelo campo de fluxo. `Preparando` dura `preparo`; no fim, se `distância ≤ ALCANCE_GOLPE` e linha de visão, sorteia o dano e devolve em `EventosIA`; vai para `Recuperando` (`recuperacao`) e volta a `Perseguindo`.
  - **Atirador**: com linha de visão e distância entre 3 e 6, fica parado e prepara (0,4 s) e atira (projétil na direção do jogador, velocidade 10, dano sorteado), respeitando 1,5 s entre tiros; mais perto que 3, recua (vizinha com distância de campo maior); mais longe que 6 ou sem linha de visão, aproxima-se pelo campo.
  - **Andar pelo campo**: entre as 4 vizinhas da célula atual, escolhe a de menor distância (≥ 0) e anda para o centro dela com a velocidade do tipo (`moverComColisao` com `bloqueiaPassagem`). Se a vizinha escolhida for `Comporta` fechada, adiciona em `comportasParaAbrir` e espera parado. `ComportaTrancada` nunca entra no campo.
  - **Separação**: depois de mover todos, pares a menos de `2 × RAIO_INIMIGO` se afastam metade cada (com `moverComColisao`).

- [ ] **Passo 1: Escrever os testes** (helper `simular(v, grade, jogador, segundos)` que recalcula o campo a cada 0,5 s e aplica `comportasParaAbrir` virando `ComportaAberta` na hora)

```cpp
TEST_CASE("parado acorda ao ver o jogador e se aproxima") {
    auto a = mundo::andarDeTexto({"############", "#@........r#", "############"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 1.0f);
    CHECK(v[0].estado != jogo::EstadoInimigo::Parado);
    CHECK(v[0].posicao.x < 10.5f - 2.0f);              // rato rápido: 3,5 células/s
}
TEST_CASE("parede impede ver; barulho acorda mesmo assim") {
    auto a = mundo::andarDeTexto({"#########", "#@..#..a#", "#########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 0.5f);
    CHECK(v[0].estado == jogo::EstadoInimigo::Parado);
    jogo::acordarPorBarulho(v, a.inicio, 10.0f);
    CHECK(v[0].estado == jogo::EstadoInimigo::Perseguindo);
}
TEST_CASE("autômato prepara o golpe e acerta quem ficou perto") {
    auto a = mundo::andarDeTexto({"#####", "#@a.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    auto eventos = simular(v, a.grade, a.inicio, 0.7f);  // aproxima até 0,9 e prepara 0,5 s
    CHECK(eventos.dano >= 4);
    CHECK(eventos.dano <= 6);
    CHECK(eventos.autor == jogo::TipoInimigo::Automato);
}
TEST_CASE("golpe não acerta se o jogador se afastou durante o preparo") {
    auto a = mundo::andarDeTexto({"#########", "#@a.....#", "#########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 0.1f);                 // começou a preparar
    REQUIRE(v[0].estado == jogo::EstadoInimigo::Preparando);
    auto eventos = simular(v, a.grade, {6.5f, 1.5f}, 0.5f);   // jogador longe no fim do preparo
    CHECK(eventos.dano == 0);
}
TEST_CASE("atirador mantém distância e atira") {
    auto a = mundo::andarDeTexto({"##############", "#@...g.......#", "##############"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    auto eventos = simular(v, a.grade, a.inicio, 2.0f);
    CHECK(glm::distance(v[0].posicao, a.inicio) >= 3.0f - 0.3f);
    CHECK(eventos.tiros.size() >= 1);
    CHECK(glm::length(eventos.tiros[0].velocidade) == doctest::Approx(10.0f));
}
TEST_CASE("inimigo abre a comporta e passa") {
    auto a = mundo::andarDeTexto({"###########", "#@...C...a#", "###########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simular(v, a.grade, a.inicio, 5.0f);
    CHECK(a.grade.em(5, 1) == mundo::Celula::ComportaAberta);
    CHECK(v[0].posicao.x < 5.0f);
}
TEST_CASE("comporta trancada segura o inimigo") {
    auto a = mundo::andarDeTexto({"###########", "#@...T...a#", "###########"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simular(v, a.grade, a.inicio, 5.0f);
    CHECK(v[0].posicao.x > 6.0f);
}
TEST_CASE("dois inimigos no corredor não se sobrepõem") {
    auto a = mundo::andarDeTexto({"############", "#@.......aa#", "############"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::acordarPorBarulho(v, a.inicio, 20.0f);
    simular(v, a.grade, {1.5f, 1.5f}, 3.0f);
    CHECK(glm::distance(v[0].posicao, v[1].posicao) >= 2 * jogo::RAIO_INIMIGO - 0.05f);
}
TEST_CASE("empurrão interrompe e desloca") {
    auto a = mundo::andarDeTexto({"#######", "#@a...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    simular(v, a.grade, a.inicio, 0.1f);
    REQUIRE(v[0].estado == jogo::EstadoInimigo::Preparando);
    const float antes = v[0].posicao.x;
    v[0].empurrao = {0.3f, 0.0f};
    v[0].estado = jogo::EstadoInimigo::Perseguindo;      // quem interrompe é a arma (Tarefa 5)
    simular(v, a.grade, a.inicio, 1.0f / 60);
    CHECK(v[0].posicao.x > antes + 0.2f);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `IA`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 5: Armas do jogador

**Arquivos:**
- Modificar: `src/jogo/Comandos.h` (+ `bool atacar = false; bool trocarArma = false; bool recarregar = false;`)
- Criar: `src/jogo/Armas.h/.cpp`
- Testar: `testes/teste_armas.cpp`

**Interfaces:**
- Consome: `Inimigo`, `dados`, `acordarPorBarulho` (Tarefas 2 e 4); `temLinhaDeVisao`.
- Produz:
  - `enum class jogo::ArmaAtiva { ChaveInglesa, Revolver };`
  - `struct jogo::Armas { ArmaAtiva ativa = ArmaAtiva::ChaveInglesa; int carga = 6; int reserva = 12; float intervalo = 0; float recarga = 0; float animacao = 0; };` (`animacao` > 0 enquanto o golpe/tiro aparece na mão; `recarga` > 0 enquanto recarrega)
  - `constexpr int jogo::RESERVA_MAXIMA_BALAS = 48;`
  - `struct jogo::ResultadoAtaque { int acertos = 0; int destruidos = 0; float barulho = 0; bool disparou = false; std::string mensagem; };`
  - `jogo::ResultadoAtaque jogo::atualizarArmas(Armas& a, const Comandos& c, glm::vec2 posicao, float angulo, const mundo::Grade& g, std::vector<Inimigo>& inimigos, std::mt19937& rng, float dt);`
- **Regras:**
  - `trocarArma` alterna e cancela a recarga. `recarregar` com revólver, carga < 6 e reserva > 0 inicia 1,5 s; ao terminar move `min(6 − carga, reserva)`.
  - `atacar` só age com `intervalo ≤ 0`, sem recarga em andamento.
  - **Chave inglesa:** acerta todo inimigo não destruído com `distância − RAIO_INIMIGO ≤ 1,1`, ângulo até 30° da frente e linha de visão; dano 4–6 menos a defesa do tipo (mínimo 1); empurrão de 0,3 na direção jogador→inimigo; se estava `Preparando`, volta a `Perseguindo`; acorda se estava `Parado`; intervalo 0,35; barulho 4.
  - **Revólver:** com carga 0, mensagem `"Sem munição — R para recarregar"` (ou `"Sem munição"` se reserva 0) e intervalo 0,4 (para não repetir a mensagem a cada passo); com carga: gasta 1, sorteia desvio em ±2°, acerta o inimigo não destruído mais próximo cujo círculo (raio 0,25) o raio cruza antes de uma parede (confere com `temLinhaDeVisao` até o ponto de contato); dano 7–10 menos defesa; não empurra nem interrompe; intervalo 0,4; barulho 10; `disparou = true`.
  - Vida ≤ 0 → `Destruido`, `tempoNoEstado = 0`, conta em `destruidos`.
  - `animacao` = 0,2 s a cada golpe ou tiro.

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("chave inglesa acerta à frente, empurra e interrompe") {
    auto a = mundo::andarDeTexto({"#######", "#@a...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].estado = jogo::EstadoInimigo::Preparando;
    jogo::Armas armas; jogo::Comandos c; c.atacar = true; std::mt19937 rng(1);
    auto r = jogo::atualizarArmas(armas, c, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(r.acertos == 1);
    CHECK(v[0].vida <= 16 - 4);
    CHECK(v[0].vida >= 16 - 6);
    CHECK(v[0].estado == jogo::EstadoInimigo::Perseguindo);
    CHECK(glm::length(v[0].empurrao) == doctest::Approx(0.3f));
    CHECK(r.barulho == doctest::Approx(4.0f));
}
TEST_CASE("chave inglesa não acerta atrás nem longe") {
    auto a = mundo::andarDeTexto({"########", "#a@...a#", "########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas; jogo::Comandos c; c.atacar = true; std::mt19937 rng(1);
    CHECK(jogo::atualizarArmas(armas, c, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60).acertos == 0);
}
TEST_CASE("defesa do colosso reduz o dano") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas; jogo::Comandos c; c.atacar = true; std::mt19937 rng(1);
    jogo::atualizarArmas(armas, c, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(v[0].vida >= 40 - 4);   // 4–6 menos 2
    CHECK(v[0].vida <= 40 - 2);
}
TEST_CASE("segurar o botão respeita o intervalo") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas; jogo::Comandos c; c.atacar = true; std::mt19937 rng(1);
    int golpes = 0;
    for (int i = 0; i < 60; ++i) golpes += jogo::atualizarArmas(armas, c, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60).acertos;
    CHECK(golpes == 3);   // 1 s ÷ 0,35 s → golpes em 0; 0,35; 0,70
}
TEST_CASE("revólver acerta o primeiro inimigo e não atravessa parede") {
    auto a = mundo::andarDeTexto({"###########", "#@...a#..a#", "###########"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Armas armas; armas.ativa = jogo::ArmaAtiva::Revolver;
    jogo::Comandos c; c.atacar = true; std::mt19937 rng(1);
    auto r = jogo::atualizarArmas(armas, c, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(r.disparou);
    CHECK(armas.carga == 5);
    CHECK(v[0].vida <= 16 - 7);
    CHECK(v[1].vida == 16);
    CHECK(r.barulho == doctest::Approx(10.0f));
}
TEST_CASE("revólver vazio avisa e recarga move balas da reserva") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Armas armas; armas.ativa = jogo::ArmaAtiva::Revolver; armas.carga = 0; armas.reserva = 4;
    jogo::Comandos c; c.atacar = true; std::mt19937 rng(1);
    auto r = jogo::atualizarArmas(armas, c, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK_FALSE(r.disparou);
    CHECK(r.mensagem == "Sem munição — R para recarregar");
    jogo::Comandos rec; rec.recarregar = true;
    jogo::atualizarArmas(armas, rec, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    for (int i = 0; i < 90; ++i) jogo::atualizarArmas(armas, {}, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.carga == 4);
    CHECK(armas.reserva == 0);
}
TEST_CASE("recarregar cheio não faz nada; trocar de arma cancela a recarga") {
    auto a = mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1);
    std::vector<jogo::Inimigo> v;
    jogo::Armas armas; armas.ativa = jogo::ArmaAtiva::Revolver;
    jogo::Comandos rec; rec.recarregar = true; std::mt19937 rng(1);
    jogo::atualizarArmas(armas, rec, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.recarga == 0.0f);
    armas.carga = 2;
    jogo::atualizarArmas(armas, rec, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.recarga > 0.0f);
    jogo::Comandos troca; troca.trocarArma = true;
    jogo::atualizarArmas(armas, troca, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.ativa == jogo::ArmaAtiva::ChaveInglesa);
    CHECK(armas.recarga == 0.0f);
    CHECK(armas.carga == 2);
}
TEST_CASE("inimigo com vida zero é destruído") {
    auto a = mundo::andarDeTexto({"#####", "#@r.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    v[0].vida = 1;
    jogo::Armas armas; jogo::Comandos c; c.atacar = true; std::mt19937 rng(1);
    auto r = jogo::atualizarArmas(armas, c, a.inicio, 0.0f, a.grade, v, rng, 1.0f / 60);
    CHECK(r.destruidos == 1);
    CHECK(v[0].estado == jogo::EstadoInimigo::Destruido);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Armas`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 6: Partida com combate, morte e troca de andar

**Arquivos:**
- Modificar: `src/jogo/Partida.h/.cpp`
- Testar: `testes/teste_partida.cpp`

**Interfaces:**
- Consome: Tarefas 2–5.
- Produz (além do que já existe):
  - `constexpr int jogo::VIDA_INICIAL = 30, jogo::DEFESA_INICIAL = 1;`
  - Construtores: o de semente popula com `gerarInimigos`; o de `Andar` usa `inimigosDasMarcas` (mapas de teste).
  - `int vida() const; int vidaMaxima() const; int defesa() const;`
  - `const std::vector<Inimigo>& inimigos() const; const std::vector<Projetil>& projeteis() const; const Armas& armas() const;`
  - `bool morto() const; std::string_view causaDaMorte() const; int inimigosDestruidos() const; int nivel() const { return 1; } int engrenagens() const { return 0; } int pontuacao() const;`
  - `float clarao() const;` (segundos restantes do clarão, 0,05 ao disparar) `float tempoDesdeDano() const;` (cresce a partir do último dano; começa grande)
  - `void tornarInvencivel();` (opção de teste `--invencivel`: dano não reduz a vida)
- **Ordem em `atualizar`** (nada acontece se `morto()`): jogador anda → usar → armas (barulho acorda inimigos) → campo de fluxo (a cada 0,5 s) → IA (abre comportas pedidas, cria projéteis) → projéteis → dano recebido (`max(1, bruto − defesa)` por golpe/projétil) → se vida ≤ 0: `morto = true`, causa `"Destruído por um <nome>"` / `"Destruído por uma"` conforme o gênero ("um Rato mecânico", "um Autômato operário", "um Atirador da Guilda", "um Colosso a vapor") — o último golpe do passo define a causa → comportas → chave → exploração → mensagem (inclui a das armas) → remove destroços com mais de 10 s.
  - `descer()`: além do que já faz, gera a nova população e limpa projéteis; vida e armas continuam.

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("partida pela semente tem inimigos") {
    jogo::Partida p(42, 2);
    CHECK(p.inimigos().size() >= 8);
    CHECK(p.vida() == 30);
}
TEST_CASE("dano recebido desconta a defesa") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@a.#", "#####"}, 1), 1);
    passos(p, 50);   // autômato chega a 0,9, prepara (0,5 s) e acerta 4–6 → 3–5; a recuperação (0,7 s) evita o 2º golpe
    CHECK(p.vida() <= 30 - 3);
    CHECK(p.vida() >= 30 - 5);
    CHECK(p.tempoDesdeDano() < 0.2f);
}
TEST_CASE("morte registra uma vez e congela a partida") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@cc..#", "#######"}, 5), 1);
    passos(p, 60 * 30);
    CHECK(p.morto());
    CHECK(p.causaDaMorte() == "Destruído por um Colosso a vapor");
    auto pos = p.jogador().posicao;
    passos(p, 60, {1, 0, 0});
    CHECK(p.jogador().posicao == pos);
    CHECK(p.vida() <= 0);
}
TEST_CASE("invencível não morre") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@cc..#", "#######"}, 5), 1);
    p.tornarInvencivel();
    passos(p, 60 * 10);
    CHECK_FALSE(p.morto());
}
TEST_CASE("disparo faz clarão e conta destruídos") {
    jogo::Partida p(mundo::andarDeTexto({"#########", "#@....r.#", "#########"}, 1), 1);
    jogo::Comandos troca; troca.trocarArma = true;
    passos(p, 1, troca);
    jogo::Comandos tiro; tiro.atacar = true;
    p.atualizar(tiro, 1.0f / 60);
    CHECK(p.clarao() > 0.0f);
    CHECK(p.armas().carga == 5);
    passos(p, 60, tiro);   // segura o gatilho: o rato (6 de vida) cai com 1 ou 2 tiros
    CHECK(p.inimigosDestruidos() == 1);
    CHECK(p.pontuacao() == 100 * 1 + 10 * 1 + 0 + 50 * 1);
}
TEST_CASE("descer limpa inimigos e projéteis e mantém vida e armas") {
    jogo::Partida p(mundo::andarDeTexto({"#########", "#@E....g#", "#########"}, 1), 7);
    passos(p, 90);                                 // o atirador já atirou
    int vida = p.vida();
    int carga = p.armas().carga;
    passos(p, 1, apertarE());
    CHECK(p.andar().numero == 2);
    CHECK(p.projeteis().empty());
    CHECK(p.inimigos().size() >= 8);              // população nova do andar 2
    CHECK(p.vida() == vida);
    CHECK(p.armas().carga == carga);
}
TEST_CASE("destroços somem depois de 10 s") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@r.#", "#####"}, 1), 1);
    jogo::Comandos golpe; golpe.atacar = true;
    passos(p, 60, golpe);
    REQUIRE(p.inimigosDestruidos() == 1);
    passos(p, 60 * 11);
    CHECK(p.inimigos().empty());
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar.** Os testes antigos da `Partida` (mapas sem inimigos) continuam passando sem mudança.
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 7: Placar e pasta de dados

**Arquivos:**
- Criar: `src/jogo/Placar.h/.cpp`
- Modificar: `src/plataforma/Caminhos.h/.cpp`
- Testar: `testes/teste_placar.cpp`

**Interfaces:**
- Produz:
  - `int jogo::calcularPontuacao(int andar, int inimigos, int engrenagens, int nivel);`
  - `struct jogo::RegistroPlacar { int pontuacao; int andar; int nivel; int inimigos; int engrenagens; uint32_t semente; std::string causa; };`
  - `constexpr size_t jogo::TAMANHO_PLACAR = 10;`
  - `std::vector<RegistroPlacar> jogo::inserirNoPlacar(std::vector<RegistroPlacar> placar, RegistroPlacar r, int* posicao)` — ordena por pontuação decrescente (empate: o mais antigo fica na frente), corta em 10; `*posicao` = colocação 1–10 ou 0 se ficou de fora
  - `struct jogo::LeituraPlacar { std::vector<RegistroPlacar> registros; bool estavaCorrompido = false; };`
  - `jogo::LeituraPlacar jogo::lerPlacar(const std::filesystem::path& arquivo);` — ausente: vazio; linha inválida: renomeia o arquivo para `placar.txt.corrompido` (substituindo um anterior) e devolve vazio com `estavaCorrompido`
  - `bool jogo::gravarPlacar(const std::filesystem::path& arquivo, const std::vector<RegistroPlacar>& r);` — cria a pasta; grava em `.tmp` e renomeia (não corrompe se o jogo fechar no meio); `false` se não conseguir
  - Formato: uma linha por registro, campos separados por `;` na ordem da struct; `;` e quebras de linha na causa viram espaço; UTF-8
  - `std::filesystem::path plataforma::pastaDeDados();` — `%APPDATA%\Roguelike` (via `SDL_GetPrefPath("", "Roguelike")` ou `SHGetKnownFolderPath`), a não ser que `definirPastaDeDados(path)` tenha sido chamada (opção `--dados`)

- [ ] **Passo 1: Escrever os testes** (pasta temporária única por caso, criada em `std::filesystem::temp_directory_path()` e apagada no fim)

```cpp
TEST_CASE("pontuação da spec") {
    CHECK(jogo::calcularPontuacao(3, 12, 40, 2) == 300 + 120 + 40 + 100);
}
TEST_CASE("placar guarda os 10 melhores em ordem") {
    std::vector<jogo::RegistroPlacar> p;
    int pos = 0;
    for (int i = 1; i <= 12; ++i) p = jogo::inserirNoPlacar(p, {i * 100, 1, 1, 0, 0, 1u, "x"}, &pos);
    REQUIRE(p.size() == 10);
    CHECK(p.front().pontuacao == 1200);
    CHECK(p.back().pontuacao == 300);
    p = jogo::inserirNoPlacar(p, {50, 1, 1, 0, 0, 1u, "x"}, &pos);
    CHECK(pos == 0);
    p = jogo::inserirNoPlacar(p, {1000, 1, 1, 0, 0, 1u, "x"}, &pos);
    CHECK(pos == 4);   // empate com o 1000 antigo (3º): o antigo fica na frente
}
TEST_CASE("gravar e ler de volta, com acentos e ponto e vírgula na causa") {
    auto pasta = pastaTemporaria();
    std::vector<jogo::RegistroPlacar> p{{560, 3, 1, 12, 0, 4242u, "Destruído por um Colosso; a vapor"}};
    REQUIRE(jogo::gravarPlacar(pasta / "placar.txt", p));
    auto lido = jogo::lerPlacar(pasta / "placar.txt");
    REQUIRE(lido.registros.size() == 1);
    CHECK(lido.registros[0].semente == 4242u);
    CHECK(lido.registros[0].causa == "Destruído por um Colosso  a vapor");
}
TEST_CASE("placar ausente é vazio") {
    auto pasta = pastaTemporaria();
    auto lido = jogo::lerPlacar(pasta / "nao_existe.txt");
    CHECK(lido.registros.empty());
    CHECK_FALSE(lido.estavaCorrompido);
}
TEST_CASE("placar corrompido vira .corrompido") {
    auto pasta = pastaTemporaria();
    { std::ofstream f(pasta / "placar.txt"); f << "isto não é um placar\n"; }
    auto lido = jogo::lerPlacar(pasta / "placar.txt");
    CHECK(lido.estavaCorrompido);
    CHECK(lido.registros.empty());
    CHECK(std::filesystem::exists(pasta / "placar.txt.corrompido"));
    CHECK_FALSE(std::filesystem::exists(pasta / "placar.txt"));
}
TEST_CASE("gravar em lugar impossível devolve falso sem lançar") {
    CHECK_FALSE(jogo::gravarPlacar("Z:\\<invalido>\\placar.txt", {}));
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Placar` e `pastaDeDados`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 8: Texto, HUD, tela de fim e animação dos inimigos (puros)

**Arquivos:**
- Criar: `src/render/Texto.h/.cpp`, `src/render/Hud.h/.cpp`, `src/render/AnimacaoInimigos.h/.cpp`
- Modificar: `src/render/Automapa.*` (cor `vec4`), `src/render/Sprites.h` (camadas novas)
- Testar: `testes/teste_texto.cpp`, `testes/teste_hud.cpp`, `testes/teste_animacao.cpp`

**Interfaces:**
- Produz:
  - `VerticeCor::cor` passa a ser `glm::vec4` (o automapa usa alfa 1)
  - `struct render::VerticeTex { glm::vec2 posicao; glm::vec2 uv; float camada; glm::vec4 cor; };`
  - `constexpr int render::LADO_GLIFO = 8;`
  - `extern const char32_t render::CARACTERES_FONTE[];` = ASCII 32–126 seguido de `ÁÂÃÀÇÉÊÍÓÔÕÚáâãàçéêíóôõú—º` (a camada do glifo é o índice nesta lista)
  - `int render::indiceGlifo(char32_t c);` (desconhecido → índice de `'?'`)
  - `std::vector<VerticeTex> render::construirTexto(std::string_view utf8, glm::vec2 posicao, float escala, glm::vec4 cor);` — um quadrado de `8 × escala` por caractere, espaço também avança; UTF-8 inválido vira `'?'`
  - `float render::larguraTexto(std::string_view utf8, float escala);`
  - `enum render::CamadaSprite : int { SpriteValvulaChave = 0, SpriteElevador = 1, SpriteProjetil = 2, PRIMEIRO_INIMIGO = 3 };` e `constexpr int render::QUADROS_POR_INIMIGO = 7;` (parado, andar 1, andar 2, atacar 1, atacar 2, morrer 1, destroços)
  - `int render::camadaDoInimigo(jogo::TipoInimigo t, jogo::EstadoInimigo e, float tempoNoEstado);` — andar alterna a cada 0,25 s; Preparando = atacar 1; Recuperando = atacar 2; Destruido: morrer 1 nos primeiros 0,3 s e depois destroços
  - `glm::vec2 render::tamanhoDoInimigo(jogo::TipoInimigo t);` — rato 0,4 × 0,3; autômato 0,6 × 0,85; atirador 0,6 × 0,85; colosso 0,9 × 1,0
  - `struct render::ElementosHud { std::vector<VerticeCor> cores; std::vector<VerticeTex> texto; std::vector<VerticeTex> arma; };`
  - `struct render::EstadoHud { int vida, vidaMaxima; jogo::ArmaAtiva arma; int carga, reserva; bool recarregando; float animacao; float tempoDesdeDano; std::string_view mensagem; int andar; float tempo; };`
  - `render::ElementosHud render::construirHud(const EstadoHud& e, int telaL, int telaA);`
    - barra de vida em baixo à esquerda: fundo `(0.15, 0.10, 0.05, 1)` de 100 × 8 px em `(8, telaA − 16)`, preenchimento latão `(0.85, 0.62, 0.20, 1)` proporcional (`max(0, vida) / vidaMaxima`), texto `"30/30"` à direita da barra
    - revólver ativo: texto `"6 | 12"` (carga | reserva) em baixo à direita; `"RECARREGANDO"` no lugar enquanto recarrega
    - `"Andar N"` no canto de cima à esquerda; mensagem (se houver) centralizada em cima
    - arma na mão: quadrado 128 × 128 px centralizado em baixo, camada pelo estado (`0` chave parada, `1` chave golpe, `2` revólver parado, `3` revólver tiro, `4` revólver recarga); balanço vertical de até 6 px por 0,3 s depois de levar dano
    - flash vermelho: tela inteira `(0.8, 0, 0, alfa)` com `alfa = 0.35 × (1 − tempoDesdeDano / 0.25)` enquanto `tempoDesdeDano < 0.25`
    - ordem em `cores`: fundo da barra, preenchimento da barra, …, flash vermelho por último
  - `struct render::DadosFim { int andar, nivel, inimigos, engrenagens, pontuacao; uint32_t semente; std::string_view causa; int posicaoPlacar; };`
  - `render::ElementosHud render::construirTelaFim(const DadosFim& d, int telaL, int telaA);` — painel escuro (alfa 0,8) e as linhas: `"VOCÊ MORREU"`, causa, `"Andar 3   Nível 1   Inimigos 12"`, `"Engrenagens 0   Pontuação 560"`, `"Semente 4242"`, `"3º lugar no placar!"` (só se `posicaoPlacar > 0`), `"Enter: nova partida"`

- [ ] **Passo 1: Escrever os testes**

```cpp
// teste_texto.cpp
TEST_CASE("um quadrado por caractere, inclusive acentuado") {
    CHECK(render::construirTexto("Olá", {0, 0}, 1, glm::vec4(1)).size() == 3 * 6);
    CHECK(render::larguraTexto("Olá", 2.0f) == doctest::Approx(48.0f));
}
TEST_CASE("glifos acentuados e desconhecidos") {
    CHECK(render::indiceGlifo(U'A') == 'A' - 32);
    CHECK(render::indiceGlifo(U'ç') > 94);
    CHECK(render::indiceGlifo(U'€') == render::indiceGlifo(U'?'));
}
TEST_CASE("UTF-8 inválido vira interrogação") {
    auto v = render::construirTexto(std::string_view("\xC3", 1), {0, 0}, 1, glm::vec4(1));
    REQUIRE(v.size() == 6);
    CHECK(v[0].camada == float(render::indiceGlifo(U'?')));
}
// teste_hud.cpp
TEST_CASE("barra de vida pela metade") {
    render::EstadoHud e{15, 30, jogo::ArmaAtiva::ChaveInglesa, 6, 12, false, 0, 9.0f, "", 1, 0};
    auto h = render::construirHud(e, 480, 270);
    // segundo retângulo = preenchimento
    float largura = h.cores[6 + 1].posicao.x - h.cores[6].posicao.x;
    CHECK(largura == doctest::Approx(50.0f));
}
TEST_CASE("flash vermelho só logo depois do dano") {
    render::EstadoHud e{30, 30, jogo::ArmaAtiva::ChaveInglesa, 6, 12, false, 0, 0.05f, "", 1, 0};
    auto com = render::construirHud(e, 480, 270).cores.size();
    e.tempoDesdeDano = 1.0f;
    auto sem = render::construirHud(e, 480, 270).cores.size();
    CHECK(com == sem + 6);
}
TEST_CASE("arma na mão mostra o quadro do estado") {
    render::EstadoHud e{30, 30, jogo::ArmaAtiva::Revolver, 6, 12, false, 0.1f, 9.0f, "", 1, 0};
    CHECK(render::construirHud(e, 480, 270).arma[0].camada == 3.0f);   // revólver tiro
    e.recarregando = true; e.animacao = 0;
    CHECK(render::construirHud(e, 480, 270).arma[0].camada == 4.0f);
}
TEST_CASE("tela de fim só fala do placar se entrou nele") {
    render::DadosFim d{3, 1, 12, 0, 560, 4242u, "Destruído por um Rato mecânico", 0};
    auto sem = render::construirTelaFim(d, 480, 270).texto.size();
    d.posicaoPlacar = 3;
    auto com = render::construirTelaFim(d, 480, 270).texto.size();
    CHECK(com == sem + render::construirTexto("3º lugar no placar!", {0, 0}, 1, glm::vec4(1)).size());
}
// teste_animacao.cpp
TEST_CASE("quadros do inimigo pelo estado e pelo tempo") {
    using T = jogo::TipoInimigo; using E = jogo::EstadoInimigo;
    const int base = render::PRIMEIRO_INIMIGO + 1 * render::QUADROS_POR_INIMIGO;  // autômato
    CHECK(render::camadaDoInimigo(T::Automato, E::Parado, 5.0f) == base + 0);
    CHECK(render::camadaDoInimigo(T::Automato, E::Perseguindo, 0.1f) == base + 1);
    CHECK(render::camadaDoInimigo(T::Automato, E::Perseguindo, 0.3f) == base + 2);
    CHECK(render::camadaDoInimigo(T::Automato, E::Preparando, 0.1f) == base + 3);
    CHECK(render::camadaDoInimigo(T::Automato, E::Recuperando, 0.1f) == base + 4);
    CHECK(render::camadaDoInimigo(T::Automato, E::Destruido, 0.1f) == base + 5);
    CHECK(render::camadaDoInimigo(T::Automato, E::Destruido, 2.0f) == base + 6);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Texto`, `Hud`, `AnimacaoInimigos` e o alfa em `VerticeCor`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 9: Arte — fonte, inimigos, projétil e armas na mão

**Arquivos:**
- Criar: `ferramentas/gerar_fonte.ps1`, `assets/fonte/fonte.png`, `assets/sprites/rato_*.png`, `automato_*.png`, `atirador_*.png`, `colosso_*.png` (7 quadros cada, 64×64), `assets/sprites/projetil.png`, `assets/armas/chave_parada.png`, `chave_golpe.png`, `revolver_parado.png`, `revolver_tiro.png`, `revolver_recarga.png` (128×128)
- Modificar: `ferramentas/gerar_sprites.ps1`, `assets/LICENCAS.txt`

- [ ] **Passo 1: Fonte.** Procurar uma fonte pixel 8×8 com licença livre (CC0 ou OFL) que tenha os acentos de `CARACTERES_FONTE` — candidatas: *Press Start 2P* (OFL) e as fontes da Kenney (CC0). Mostrar as opções ao Nidragnnar (nome, licença, link, tamanho, prévia dos acentos) e **pedir ok antes de baixar**. Rasterizar com `ferramentas/gerar_fonte.ps1` (System.Drawing, sem suavização) numa grade de 16 colunas de 8×8 na ordem de `CARACTERES_FONTE`; o carregador separa cada célula numa camada.
- [ ] **Passo 2: Inimigos.** Procurar no OpenGameArt sprites CC0/CC-BY de rato mecânico, robô operário, soldado com máscara de gás e robô grande, em vista frontal, com quadros de andar/atacar/morrer. Mostrar o que existir e **pedir ok antes de baixar**. Se o Nidragnnar preferir ou não houver nada bom, gerar sprites provisórios no `gerar_sprites.ps1` (silhuetas desenhadas com formas: corpo, cabeça, olhos, pernas que alternam nos quadros de andar, braço levantado ao atacar, peças caídas nos destroços), marcados como provisórios no `LICENCAS.txt`.
- [ ] **Passo 3: Projétil e armas na mão:** provisórios gerados (bala com brilho; chave inglesa e revólver vistos de trás, com o quadro de golpe/tiro deslocado e o de recarga com o tambor aberto).
- [ ] **Passo 4: Registrar tudo em `assets/LICENCAS.txt`.**

---

### Tarefa 10: Desenho 2D com textura, entrada e laço principal

**Arquivos:**
- Modificar: `src/render/Desenho2D.h/.cpp` (alfa + `Desenho2DTextura`), `src/render/Texturas.h/.cpp` (lado como parâmetro; carregar grade de glifos), `assets/shaders/cor2d.*`, `assets/shaders/masmorra.frag` (+ `uClarao`), `src/plataforma/Entrada.*`, `src/main.cpp`
- Criar: `assets/shaders/tex2d.vert`, `assets/shaders/tex2d.frag`

**Interfaces:**
- Consome: Tarefas 6–9.
- Produz:
  - `GLuint render::carregarArrayTexturas(const std::vector<std::filesystem::path>& arquivos, int lado = 64);`
  - `GLuint render::carregarFonte(const std::filesystem::path& png);` (corta a grade 16 × N de 8×8 em camadas; ausente → xadrez + log)
  - `class render::Desenho2DTextura { Desenho2DTextura(); void desenhar(const std::vector<VerticeTex>& v, GLuint arrayTextura, int telaL, int telaA); }` (mistura alfa ligada, descarta alfa < 0,5 da textura)
  - `Entrada`: botão esquerdo segurado (com mouse capturado, e não no clique que captura) → `atacar`; roda do mouse ou Q → `trocarArma`; R → `recarregar`; Enter → `bool pediuNovaPartida()` (consome)
  - `masmorra.frag`: `uniform float uClarao;` soma `uClarao × (1.0, 0.85, 0.6)` à luz da lanterna
  - `main`: opções `--dados <pasta>` e `--invencivel`; sprites dos inimigos (camada por `camadaDoInimigo`, tamanho por `tamanhoDoInimigo`) e projéteis desenhados com os outros sprites; HUD e mensagem na tela (o título volta a ser só `"Roguelike"` + aviso de clique); `uClarao = partida.clarao() > 0 ? 1 : 0`; na morte: congela, grava o placar **uma vez** em `pastaDeDados()/placar.txt` (falha → log), mostra `construirTelaFim`, solta o mouse; Enter cria nova `Partida` com semente aleatória e reinicia o estado (malha, interpolação, placar gravado).

- [ ] **Passo 1: Implementar.**
- [ ] **Passo 2: Compilar e capturar** (com `--dados` no scratchpad): andar 3 olhando para inimigos (`--posicao`), arma na mão e HUD; uma captura com `--invencivel` depois de alguns segundos (inimigos perto). Esperado: inimigos com o sprite certo, HUD legível com acentos, arma na mão.
- [ ] **Passo 3: Tela de fim:** rodar um cenário de morte rápida (andar 5, posição ao lado de Colossos, captura do quadro 600) com `--dados` no scratchpad. Esperado: tela de fim e `placar.txt` gravado lá, nada no `%APPDATA%`.

---

### Tarefa 11: Verificação, cenários, revisão e entrega

- [ ] **Passo 1: Release + testes.** Esperado: tudo passa, sem avisos novos.
- [ ] **Passo 2: Backup** de `%APPDATA%\Roguelike` (se existir) antes de qualquer execução que não use `--dados`; restaurar depois.
- [ ] **Passo 3: Cenários** `.bat` em `build\Release\`: partida aleatória; andar 1 (ratos e autômatos); andar 3 (todos os tipos); diante de um Atirador; diante de um Colosso; com `--invencivel` para observar a IA; revólver sem munição (`--posicao` + atirar até esvaziar).
- [ ] **Passo 4: Revisão independente** (Foco de revisão + Decisões de projeto); corrigir os importantes com teste.
- [ ] **Passo 5: Deixar o jogo aberto** com a tabela do que testar: sensação do combate, dificuldade por andar, velocidades, preparo dos golpes (dá para desviar?), tiros do Atirador (dá para desviar?), recarga, morte e placar.
- [ ] **Passo 6: Perguntar "Depois de testar, faço o commit e envio?"** e commitar com o "sim".

---

## Próximas etapas

Etapa 4 (itens e progressão) usa `Armas`, `Partida::nivel/engrenagens`, o HUD e a fonte desta etapa.
