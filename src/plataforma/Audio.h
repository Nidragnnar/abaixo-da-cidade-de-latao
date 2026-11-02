#pragma once
#include <array>

#include "jogo/Sons.h"

struct Mix_Chunk;
struct Mix_Music;

namespace plataforma {

// Efeitos e música com o SDL_mixer (único lugar que toca som). Sem placa de som ou arquivo faltando:
// uma linha no log e silêncio; o jogo nunca fecha por causa do áudio.
class Audio {
public:
    Audio();
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool ativo() const { return ativo_; }
    // assets/sons/<nome>.ogg, senão .wav
    void carregar();
    // volume 0–1 (já com a distância); pan −1 esquerda … +1 direita
    void tocar(jogo::Som s, float volume, float pan);
    // assets/sons/musica.ogg, senão .wav, em loop
    void tocarMusica();
    // 0–10 cada
    void definirVolumes(int efeitos, int musica);

private:
    bool ativo_ = false;
    std::array<Mix_Chunk*, static_cast<size_t>(jogo::Som::TOTAL)> efeitos_{};
    Mix_Music* musica_ = nullptr;
    float volumeEfeitos_ = 0.8f;
};

}
