# Arte de "Abaixo da Cidade de Latão" — prompts para gerar com IA

A pasta `atual/` tem as imagens que o jogo usa hoje (a maioria provisória, feita por roteiro). Para cada uma, abaixo
vai um prompt pronto. Os prompts estão em **inglês** porque as IAs de imagem entendem melhor; a explicação em
português vem antes de cada grupo.

## Regras para a imagem voltar ao jogo

- **Mesmo nome de arquivo** e formato **PNG**.
- **Tamanho:** 64×64 pixels (sprites e texturas); **128×128** as armas na mão (`armas/`).
- **Sprites e armas:** fundo **transparente**, bordas **duras** (sem meia-transparência: o jogo apaga todo pixel
  com menos de 50% de opacidade, então sombra suave e antisserrilhado viram buraco).
- **Texturas:** **sem** transparência e **repetíveis** (a borda da direita continua na da esquerda, a de cima na de
  baixo).
- **Inimigos e chefes:** 7 quadros, **mesmo desenho e mesmo tamanho em todos**, de frente para quem olha (o jogo
  mostra o sprite sempre virado para o jogador). Ordem: `_0` parado, `_1` andando A, `_2` andando B, `_3` preparando
  o ataque, `_4` atacando/recuperando, `_5` explodindo, `_6` destroços no chão.
- Se a IA gerar maior (512, 1024), tudo bem: eu reduzo para 64 sem suavizar. Mande também a grande.
- Depois que você mandar, eu ajusto o tamanho de cada coisa no jogo à proporção do desenho novo.

## Estilo (cole antes de cada prompt)

> **STYLE:** 16-bit pixel art, steampunk retro first-person dungeon crawler like early-90s PC games,
> limited palette of brass, copper, iron grey, soot black and warm lamplight orange, crisp hard pixel edges,
> no anti-aliasing, no soft shadows, no text, no watermark.

Para sprites e armas acrescente: **"single object centered, transparent background, full body visible, facing the
viewer"**. Para texturas: **"seamless tileable texture, flat lighting, fills the whole square"**.

---

## Inimigos (sprites/*_0..6.png)

Peça uma **folha com os 7 quadros lado a lado** (7 × 64 = 448 × 64) para manter o mesmo personagem; eu corto.

**Rato mecânico** (`rato_`, pequeno, vem em bando)
> A small clockwork rat made of riveted iron plates, brass gears visible on its side, glowing red eyes, a wind-up key
> on its back, segmented metal tail. Sprite sheet of 7 frames: idle; walk A; walk B; crouched ready to bite; biting
> lunge; bursting apart in sparks; scattered broken pieces on the ground.

**Autômato operário** (`automato_`, robô de fábrica, corpo a corpo)
> A hulking factory worker automaton, boiler-shaped torso with a pressure gauge, piston arms ending in heavy wrench
> hands, single round lamp eye, soot-stained iron and brass. 7 frames: idle; walk A; walk B; arm raised winding up a
> punch; punching; exploding in steam and sparks; heap of scrap on the floor.

**Atirador da Guilda** (`atirador_`, mercenário que atira de longe)
> A Guild mercenary in a long leather coat and gas mask with brass goggles, holding a steam rifle. 7 frames: idle;
> walk A; walk B; aiming the rifle; firing with a muzzle flash; collapsing in a burst of steam; body and broken rifle
> lying on the floor.

**Colosso a vapor** (`colosso_`, grande e blindado)
> A towering armored steam colossus, thick iron plating, chimney stacks on the shoulders puffing smoke, glowing
> furnace in the chest, massive hammer fists. 7 frames: idle; walk A; walk B; both fists raised; smashing down;
> furnace exploding; large wreck of plates on the floor.

**Vespa a corda** (`vespa_`, voa e mergulha)
> A clockwork wasp of brass and black iron stripes, wind-up key on its back, translucent-looking metal wings drawn as
> solid pale grey, red glass eyes, needle stinger. 7 frames: hovering wings up; flying wings up; flying wings down;
> diving with stinger forward; stinging; bursting into sparks; broken wasp on the floor.

**Dirigível a vapor** (`dirigivel_`, paira perto do teto e solta bombas)
> A small steam airship drone: patched canvas balloon with brass bands, iron gondola with a tiny boiler and a rear
> propeller, bomb hatch underneath. 7 frames: hovering; propeller position A; propeller position B; hatch opening
> with a bomb visible; bomb dropping; balloon torn and on fire; crumpled canvas and gondola on the floor.

## Chefes (sprites/*_0..6.png, desenhados maiores na tela)

**Rainha dos Ratos** (`rainha_`, andar 2, esgoto)
> A giant mechanical rat queen, iron fur plates, long whip-like chain tail, red glowing eyes, a dented brass crown.
> 7 frames: idle; run A; run B; low crouch aiming a charge; dizzy after hitting a wall with spinning stars; exploding;
> wreck with the crown lying beside it.

**Capataz a Vapor** (`capataz_`, andar 4, fábrica)
> A massive steam foreman automaton wearing an iron top hat, boiler belly with a visible fire, pressure gauges on the
> chest, thick arms with steam vents. 7 frames: idle; heavy step A; heavy step B; arms raised venting steam
> (charging attack); slamming the ground with steam bursting out; boiler exploding; collapsed heap with the top hat.

**Grande Máquina** (`maquina_`, andar 6, fixa no meio da arena)
> An enormous stationary machine core: a round brass housing with a giant red glowing eye, two cannon barrels on the
> sides, turning gears and pipes on its base. 7 frames: idle; gears turning A; gears turning B; cannons glowing about
> to fire; cannons firing with muzzle flashes; core exploding; broken housing and gears on the floor.

## Objetos (sprites/)

| Arquivo | Prompt |
|---|---|
| `valvula_chave.png` | A large brass valve wheel used as a key, ornate spokes, small and readable from afar |
| `elevador.png` | A steampunk cargo elevator cage seen from the front, iron lattice gate, brass frame, a lamp on top |
| `projetil.png` | A small glowing orange bullet / hot slug, round, tiny |
| `bomba.png` | A round black iron bomb with a lit fuse sparking on top |
| `explosao.png` | A bright pixel-art explosion burst, yellow core, orange flames, no smoke (drawn fully lit) |
| `lampiao.png` | A brass wall lantern with a glass case and a warm flame, mounted on a short arm |
| `lustre.png` | A steam chandelier hanging from a chain, brass ring with three flames in glass |

## Itens no chão e ícones (sprites/item_*.png)

| Arquivo | Prompt |
|---|---|
| `item_chave_inglesa.png` | A big steel wrench, side view |
| `item_sabre.png` | A cavalry saber with a brass hand guard, side view |
| `item_martelo.png` | A steam hammer with a piston on the head and brass pipes, side view |
| `item_revolver.png` | A steampunk revolver with brass details, side view |
| `item_bacamarte.png` | A blunderbuss with a flared brass muzzle and wooden stock, side view |
| `item_tesla.png` | A Tesla pistol with copper coils and a glass bulb, side view |
| `item_casaco.png` | A thick leather duster coat, folded |
| `item_colete.png` | A riveted leather and iron vest |
| `item_armadura.png` | A brass breastplate armor with rivets |
| `item_tonico.png` | A small glass bottle of glowing green tonic with a cork |
| `item_balas.png` | A small box of brass revolver bullets |
| `item_cartuchos.png` | A few red shotgun shells with brass bases |
| `item_celulas.png` | Glass energy cells with blue glowing liquid and copper caps |
| `item_engrenagens.png` | A small pile of brass gears (money) |

## Armas na mão (armas/*.png, 128×128)

Primeira pessoa: a arma aparece **na parte de baixo e no meio da tela, segurada pela mão do jogador** (luva de couro),
apontando para a frente/para cima. Mesma posição e tamanho em todos os quadros da mesma arma.

| Arquivos | Prompt |
|---|---|
| `chave_0` / `chave_1` | First-person view, a gloved hand holding a big steel wrench at the bottom center; frame 0 resting, frame 1 swinging forward |
| `revolver_0` / `_1` / `_2` | First-person view, gloved hand holding a brass steampunk revolver at the bottom center; 0 aiming, 1 firing with muzzle flash, 2 cylinder open reloading |
| `sabre_0` / `_1` | First-person view, gloved hand holding a saber; 0 raised, 1 slashing across |
| `martelo_0` / `_1` | First-person view, both hands on a steam hammer; 0 resting on the shoulder, 1 smashing forward with steam |
| `bacamarte_0` / `_1` / `_2` | First-person view, hands holding a blunderbuss; 0 aiming, 1 firing with a big flash, 2 reloading with the barrel tipped down |
| `tesla_0` / `_1` | First-person view, gloved hand holding a Tesla pistol with coils; 0 idle with a faint glow, 1 discharging a blue electric arc |

## Texturas (texturas/*.png, repetíveis, sem transparência)

| Arquivo | Prompt |
|---|---|
| `esgoto_parede.png` | Damp old sewer brick wall, dark green moss in the cracks, water stains |
| `esgoto_chao.png` | Wet cobblestone sewer floor with puddles |
| `esgoto_teto.png` | Dark brick vaulted ceiling with drips and grime |
| `fabrica_parede.png` | Riveted iron wall plates with brass pipes running across, rust streaks |
| `fabrica_chao.png` | Iron diamond-plate factory floor, worn and oily |
| `fabrica_teto.png` | Iron ceiling grating with pipes and soot |
| `minas_parede.png` | Rough dark rock mine wall with wooden support beams and veins of ore |
| `minas_chao.png` | Packed dirt and gravel mine floor |
| `minas_teto.png` | Rough rock ceiling with a timber beam |
| `comporta.png` | A steam-powered iron bulkhead door, brass rivets, horizontal reinforcement bars |
| `comporta_trancada.png` | The same bulkhead door with a big brass padlock and a red warning valve |
| `caixa.png` | Side of a wooden shipping crate with iron corner brackets |
| `degrau.png` | The front face of a brick stair step (a band of bricks) |
