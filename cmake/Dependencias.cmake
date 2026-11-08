# Bibliotecas de terceiros baixadas na primeira configuração (versões fixas)
include(FetchContent)

# Algumas dependências ainda pedem versões antigas do CMake; o CMake 4 recusa sem isto
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# SDL2 como DLL, sem testes nem exemplos
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.32.10
    GIT_SHALLOW    TRUE)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)

FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(SDL2 glm doctest)

# SDL_mixer como DLL: só WAV, OGG (stb_vorbis) e MP3 (minimp3), todos embutidos, sem bibliotecas externas
set(SDL2MIXER_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
foreach(formato OPUS MOD MIDI FLAC WAVPACK GME CMD AIFF VOC)
    set(SDL2MIXER_${formato} OFF CACHE BOOL "" FORCE)
endforeach()
set(SDL2MIXER_WAVE ON CACHE BOOL "" FORCE)
set(SDL2MIXER_VORBIS "STB" CACHE STRING "" FORCE)
set(SDL2MIXER_MP3 ON CACHE BOOL "" FORCE)  # para os sons gerados com IA, que muitas vezes vêm em MP3
set(SDL2MIXER_MP3_MINIMP3 ON CACHE BOOL "" FORCE)
set(SDL2MIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL2_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG        release-2.8.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(SDL2_mixer)

# glad (OpenGL 3.3 core) e stb_image ficam no repositório, em externo/
add_library(glad STATIC ${CMAKE_SOURCE_DIR}/externo/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/externo/glad/include)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_SOURCE_DIR}/externo/stb)
