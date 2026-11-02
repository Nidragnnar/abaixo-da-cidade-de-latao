cmake_minimum_required(VERSION 3.25)
project(Roguelike LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Runtime estático: os amigos não precisam instalar o Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(cmake/Dependencias.cmake)

# Avisos e acentos só no nosso código
function(configurar_alvo alvo)
    target_compile_options(${alvo} PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(${alvo} PRIVATE NOMINMAX)
    target_include_directories(${alvo} PRIVATE ${CMAKE_SOURCE_DIR}/src)
endfunction()

# Lógica pura (sem SDL nem OpenGL), testada sem janela
add_library(nucleo STATIC
    src/jogo/PassoFixo.cpp
    src/mundo/Grade.cpp
    src/mundo/Zona.cpp
    src/mundo/Aleatorio.cpp
    src/mundo/Consultas.cpp
    src/mundo/Gerador.cpp
    src/mundo/Colisao.cpp
    src/jogo/Jogador.cpp
    src/jogo/Partida.cpp
    src/jogo/Inimigos.cpp
    src/jogo/Projeteis.cpp
    src/jogo/IA.cpp
    src/jogo/Armas.cpp
    src/jogo/Placar.cpp
    src/jogo/ArquivoTexto.cpp
    src/jogo/Config.cpp
    src/jogo/Sons.cpp
    src/jogo/Demo.cpp
    src/ui/Menus.cpp
    src/jogo/Itens.cpp
    src/jogo/Inventario.cpp
    src/render/Camera.cpp
    src/render/GeometriaMasmorra.cpp
    src/render/Xadrez.cpp
    src/render/Letterbox.cpp
    src/render/Comportas.cpp
    src/render/Sprites.cpp
    src/render/Zonas.cpp
    src/render/Automapa.cpp
    src/render/Primitivas2D.cpp
    src/render/Texto.cpp
    src/render/Hud.cpp
    src/render/AnimacaoInimigos.cpp
    src/render/TelaInventario.cpp
    src/render/TelasMenu.cpp
)
configurar_alvo(nucleo)
target_include_directories(nucleo PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nucleo PUBLIC glm::glm)

# Jogo
add_executable(Roguelike WIN32
    src/main.cpp
    src/plataforma/Log.cpp
    src/plataforma/Caminhos.cpp
    src/plataforma/Janela.cpp
    src/plataforma/Entrada.cpp
    src/plataforma/Audio.cpp
    src/render/Shader.cpp
    src/render/Texturas.cpp
    src/render/MalhaGPU.cpp
    src/render/TelaBaixa.cpp
    src/render/Desenho2D.cpp
)
configurar_alvo(Roguelike)
target_link_libraries(Roguelike PRIVATE nucleo SDL2::SDL2main SDL2::SDL2 SDL2_mixer::SDL2_mixer glad stb opengl32)
set_target_properties(Roguelike PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:Roguelike>)

# SDL2.dll e SDL2_mixer.dll ao lado do exe
add_custom_command(TARGET Roguelike POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE:SDL2_mixer::SDL2_mixer>
            $<TARGET_FILE_DIR:Roguelike>)

# Assets copiados um a um, sempre que mudam (não só quando o exe é religado)
file(GLOB_RECURSE ARQUIVOS_ASSETS CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR}/assets ${CMAKE_SOURCE_DIR}/assets/*)
set(COPIAS_ASSETS)
foreach(arquivo IN LISTS ARQUIVOS_ASSETS)
    set(destino ${CMAKE_BINARY_DIR}/$<CONFIG>/assets/${arquivo})
    add_custom_command(OUTPUT ${destino}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets/${arquivo} ${destino}
        DEPENDS ${CMAKE_SOURCE_DIR}/assets/${arquivo})
    list(APPEND COPIAS_ASSETS ${destino})
endforeach()
add_custom_target(copiar_assets ALL DEPENDS ${COPIAS_ASSETS})
add_dependencies(Roguelike copiar_assets)

# Testes automáticos
file(GLOB ARQUIVOS_TESTE CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/testes/*.cpp)
add_executable(testes_roguelike ${ARQUIVOS_TESTE})
configurar_alvo(testes_roguelike)
target_link_libraries(testes_roguelike PRIVATE nucleo doctest::doctest)
# Para conferir que todo som tem arquivo
target_compile_definitions(testes_roguelike PRIVATE PASTA_ASSETS="${CMAKE_SOURCE_DIR}/assets")

enable_testing()
add_test(NAME testes COMMAND testes_roguelike)

# Sons e música provisórios (não faz parte do jogo): cmake --build --preset release --target gerar_sons
add_executable(gerar_sons EXCLUDE_FROM_ALL ferramentas/gerar_sons.cpp)
configurar_alvo(gerar_sons)

# Corta e normaliza sons baixados de pacotes (não faz parte do jogo): ferramentas\preparar_sons.ps1
add_executable(cortar_som EXCLUDE_FROM_ALL ferramentas/cortar_som.cpp)
configurar_alvo(cortar_som)
