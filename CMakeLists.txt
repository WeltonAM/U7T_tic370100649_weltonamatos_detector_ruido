# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico CACHE STRING "Board type")

# Inclui o SDK do Pico
include(pico_sdk_import.cmake)

# Nome do projeto
project(DetectorRuido C CXX ASM)

# Inicializa o SDK
pico_sdk_init()

# Define o executável antes de adicionar dependências
add_executable(DetectorRuido DetectorRuido.c)

# Gera o cabeçalho para o programa PIO em lib/ws2812.pio
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(DetectorRuido ${CMAKE_CURRENT_LIST_DIR}/lib/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

# Define nome e versão do programa
pico_set_program_name(DetectorRuido "DetectorRuido")
pico_set_program_version(DetectorRuido "0.1")

# Habilita/desabilita stdio (USB para debug, UART desativado)
pico_enable_stdio_uart(DetectorRuido 0)
pico_enable_stdio_usb(DetectorRuido 1)

# Vincula as bibliotecas necessárias
target_link_libraries(DetectorRuido 
    pico_stdlib 
    hardware_adc 
    hardware_gpio
    hardware_pio
    hardware_clocks
)

# Inclui diretórios adicionais
target_include_directories(DetectorRuido PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/generated
)

# Gera os arquivos de saída adicionais (.uf2, etc.)
pico_add_extra_outputs(DetectorRuido)