# Sons de "Abaixo da Cidade de Latão" — prompts para gerar com IA

A pasta `atual/` tem os sons que o jogo usa hoje (ouça para comparar). Para cada um, abaixo vai um prompt pronto, em
**inglês** (as IAs de som entendem melhor). Os marcados com ★ são os **provisórios sintetizados**: esses são os mais
importantes de trocar. Os outros vêm de pacotes livres (CC0) e já funcionam; troque se quiser tudo no mesmo estilo.

Serve para IAs de efeito sonoro (como ElevenLabs Sound Effects, Stable Audio) e, para as músicas, IAs de música
(como Suno ou Udio, pedindo **instrumental**).

## Regras para o som voltar ao jogo

- **Mesmo nome de arquivo**, em **WAV** ou **OGG** (MP3 também serve: eu converto).
- **Duração:** a da tabela, mais ou menos. Efeitos curtos começam **na hora** (sem silêncio antes) e acabam limpos.
- **Músicas:** de 1 a 3 minutos, **instrumentais**, que possam **repetir em loop** (o fim emenda no começo).
- Não precisa acertar o volume: eu normalizo cada som e ajusto no jogo.
- Se a IA der várias versões, mande 2 ou 3 de cada: escolhemos de ouvido.

## Estilo (cole antes de cada prompt)

> **STYLE:** steampunk underground city, brass and iron machinery, steam and pistons, gritty retro game sound effect,
> dry (little reverb), no music, no voice.

Para as músicas troque por: **"instrumental loopable video game soundtrack, steampunk industrial, no vocals"**.

---

## ★ Provisórios sintetizados (prioridade)

| Arquivo | Duração | Quando toca | Prompt |
|---|---|---|---|
| `zumbido.wav` | 0,6 s | vespa a corda percebe você | Buzzing of a mechanical clockwork wasp, fast metallic wing flutter with a tiny ticking spring, approaching |
| `motor.wav` | 0,9 s | dirigível a vapor percebe você | Small steam airship engine starting up, low chugging piston rhythm and a propeller whirr |
| `explosao.wav` | 0,8 s | bomba do dirigível explode | Small bomb explosion in a stone tunnel, sharp blast with metal debris and a short rumble tail |
| `disparo_revolver.wav` | 0,35 s | tiro do revólver (o mais ouvido do jogo) | Single gunshot of a heavy steampunk revolver, punchy crack with a brief metallic ring |
| `musica_chefe.wav` | 1–3 min, loop | luta com um chefe | Tense boss battle music, driving industrial percussion like hammers on anvils, low brass stabs, hissing steam, relentless pace, loopable |

## Das armas

| Arquivo | Duração | Quando toca | Prompt |
|---|---|---|---|
| `golpe_ar.wav` | 0,2 s | golpe de arma branca que não acerta | Quick whoosh of a heavy wrench swung through the air |
| `acerto.wav` | 0,3 s | golpe ou tiro acerta um inimigo | Heavy metal impact on an iron automaton, clank with a short ring |
| `disparo_bacamarte.wav` | 0,7 s | tiro do bacamarte | Loud blunderbuss shot, deep boom with scattered pellets and a puff of smoke |
| `disparo_tesla.wav` | 0,7 s | tiro da pistola Tesla | Tesla pistol discharge, crackling electric arc with a rising capacitor whine |
| `recarga.wav` | 1,5 s | recarregar | Reloading a revolver: cylinder swing out, bullets dropping in, cylinder snapping shut |
| `arma_vazia.wav` | 0,2 s | gatilho sem bala | Dry click of an empty revolver hammer |

## Do jogador e do mundo

| Arquivo | Duração | Quando toca | Prompt |
|---|---|---|---|
| `passo_pedra.wav` | 0,2 s | passo no esgoto e nas minas | Single boot footstep on wet stone |
| `passo_metal.wav` | 0,3 s | passo na fábrica | Single boot footstep on an iron grating |
| `dano.wav` | 0,5 s | você leva dano | Dull body hit, grunt-free impact with a leather and metal thud |
| `pegar.wav` | 0,5 s | pegar item, munição ou engrenagens | Brass gears and coins clinking as they are picked up |
| `nivel.wav` | 0,6 s | subir de nível | Short triumphant steam whistle chime with a bell |
| `comporta.wav` | 1,0 s | comporta abrindo | Heavy iron bulkhead door sliding open with a hiss of steam |
| `elevador.wav` | 1,5 s | descer de elevador | Cargo elevator starting to descend, chains rattling, gears grinding, steam release |

## Dos inimigos

| Arquivo | Duração | Quando toca | Prompt |
|---|---|---|---|
| `tiro_inimigo.wav` | 0,5 s | Atirador da Guilda e chefes atiram | Distant steam rifle shot from an enemy, sharper and thinner than the player's revolver |
| `inimigo_destruido.wav` | 0,8 s | inimigo destruído | Clockwork machine breaking apart, springs and gears scattering, short steam burst |

## Menus

| Arquivo | Duração | Quando toca | Prompt |
|---|---|---|---|
| `menu_mover.wav` | 0,1 s | mudar de opção no menu | Tiny brass tick, like a gear advancing one tooth |
| `menu_confirmar.wav` | 0,1 s | confirmar no menu | Short brass lever click with a soft steam puff |

## Música de fundo

| Arquivo | Duração | Quando toca | Prompt |
|---|---|---|---|
| `musica.ogg` | 1–3 min, loop | o jogo todo (menos a luta com chefe) | Dark ambient exploration music for an underground steampunk city, slow mechanical rhythm, distant machinery and dripping water, low cello drones, subtle, loopable |
