# Documentação do Projeto: Detector de Ruído com BitDogLab

## 1. Autoria do Projeto

Este projeto é de autoria original, concebido como uma solução inovadora para monitoramento de ruído ambiental utilizando exclusivamente a placa BitDogLab. Desenvolvido com base nos conceitos de sistemas embarcados explorados durante a capacitação. A originalidade está na integração do microfone, matriz de LEDs, display SSD1306, buzzer e interface de controle via botões e joystick, todos implementados na linguagem C, na BitDogLab, com uma lógica específica de ranges ajustáveis de ruído e feedback em tempo real. A proposta visa atender aos requisitos de baixo custo e tamanho reduzido, características fundamentais de sistemas embarcados. 

---

## 2. Metodologia e Desenvolvimento do Projeto

A crescente demanda por sistemas embarcados eficientes exige metodologias que garantam funcionalidade, escalabilidade e atendimento aos requisitos estabelecidos. Para este projeto, será adotada uma abordagem baseada na metodologia simplificada de Cugnascas [01], que define etapas claras desde a concepção até a validação, adequada ao uso da BitDogLab e à programação em C. O desenvolvimento utilizará o Visual Studio Code com extensões para Raspberry Pi Pico, aproveitando o microcontrolador RP2040 presente na placa.

O processo seguirá os princípios:
- **Concepção**: Definição do escopo e objetivos do detector de ruído.
- **Pesquisa**: Análise de projetos correlatos e conceitos de sistemas embarcados.
- **Especificação**: Detalhamento de hardware (incluindo botões e joystick) e software em C.
- **Implementação**: Codificação e integração na BitDogLab.
- **Testes e Validação**: Verificação em cenários reais de ruído.

A documentação será baseada em referências como Cugnascas [01] e materiais sobre programação em C para RP2040, adaptados ao contexto da BitDogLab.

---

## 3. Passo a Passo da Execução

A execução seguirá um método simplificado, amplamente utilizado em projetos embarcados, dividido em cinco etapas:

1. **Definição do Escopo**: O sistema embarcado utilizará o microfone da BitDogLab para medir ruído ambiente, comparando-o a um range ajustável via joystick. Feedback será dado pela matriz de LEDs (verde para dentro do range, vermelho para fora), buzzer (sinal SOS) e display SSD1306 (valor fora do range). Botões A e B, além do joystick, adicionarão controle ao sistema.
2. **Pesquisa de Projetos Correlatos**: Análise de soluções como monitores de ruído com ESP32 ou alarmes com micro:bit, destacando a inovação do uso de joystick para ranges dinâmicos.
3. **Especificação de Hardware**: Inclui microfone, matriz de LEDs, SSD1306, buzzer, botões A e B, e joystick, todos conectados ao RP2040.
4. **Especificação de Software**: Desenvolvimento em C no Visual Studio Code com extensões Raspberry Pi Pico, incluindo funções para leitura de ruído, controle de periféricos e interação com botões/joystick.
5. **Implementação**: Codificação, compilação e testes na BitDogLab.

---

## 4. Relatório do Projeto

### a) Escopo do Projeto

#### Apresentação do Projeto
O projeto é um detector de ruído ambiental implementado na BitDogLab, utilizando seus componentes embarcados para monitoramento em tempo real com controle interativo via botões e joystick.

#### Título do Projeto
"Detector de Ruído Ambiental Interativo com BitDogLab"

#### Objetivos do Projeto
- Detectar o nível de ruído ambiente com o microfone da BitDogLab.
- Permitir ajuste do range de ruído via joystick e botão do joystick.
- Informar pelo matriz de LEDs: verde (dentro do range), vermelho (fora do range).
- Acionar o buzzer com sinal SOS e exibir o valor no SSD1306 quando fora do range.
- Reiniciar o programa com o botão A e entrar no modo BOOTSEL com o botão B.

#### Descrição do Funcionamento
O microfone captura o ruído, cujo valor é comparado a um range ajustável (ex.: 50-80 dB) definido pelo usuário via joystick e confirmado pelo botão do joystick. A matriz de LEDs pisca em verde se o ruído estiver no range, ou vermelho se estiver fora, acionando o buzzer (SOS) e exibindo o valor no SSD1306. O botão A reinicia o programa; o botão B entra no modo BOOTSEL, limpando LEDs e display.

#### Justificativa
O projeto atende à necessidade de monitoramento acústico em ambientes como setores industriais, escritórios, residências, oferecendo uma solução compacta, interativa e escalável.

---

### b) Especificação do Hardware

#### Diagrama em Bloco
[Microfone] --> [RP2040] --> [Matriz de LEDs]
[Joystick + Botão]   |--> [Display SSD1306]
[Botão A, Botão B]   |--> [Buzzer]

#### Função de Cada Bloco
- **RP2040**: Processa sinais, compara ranges e gerencia periféricos.
- **Joystick + Botão**: Ajusta e confirma o range.
- **Microfone**: Captura o ruído ambiente (sinal analógico).
- **Matriz de LEDs**: Feedback visual (verde ou vermelho).
- **Display SSD1306**: Exibe valores fora do range.
- **Buzzer**: Emite sinal SOS.
- **Botão A**: Reinicia o programa.
- **Botão B**: Entra no modo BOOTSEL, limpando LEDs e display.

#### Configuração de Cada Bloco & Descrição da Pinagem Usada
- **Microfone**: Pino ADC (ex.: GP28).
- **Matriz de LEDs**: GPIO 7.
- **Display SSD1306**: I2C (SDA em GP20, SCL em GP21).
- **Buzzer**: PWM (ex.: GP16).
- **Joystick**: Eixos X/Y em ADC (GP26, GP27), botão em GPIO (GP22).
- **Botão A**: GPIO (ex.: GP5).
- **Botão B**: GPIO (ex.: GP6).

#### Comandos e Registros Utilizados
- ADC para microfone e joystick.
- I2C para SSD1306.
- PWM para buzzer.
- GPIO para botões.

#### Circuito Completo do Hardware
(Desenho a ser incluído na implementação final.)

---

### c) Especificação do Firmware

#### Blocos Funcionais
[Leitura ADC Microfone] --> [Comparação Range] --> [Controle LEDs]
[Leitura Joystick]         |--> [Exibição SSD1306]
[Leitura Botões]           |--> [Sinal SOS Buzzer]

#### Definição das Variáveis
- `noise_level`: Ruído em dB (float).
- `min_range`: Limite inferior (float, ajustável).
- `max_range`: Limite superior (float, ajustável).

#### Fluxograma
(Desenho a ser incluído: início -> leitura joystick/botões -> definição range -> leitura microfone -> comparação -> controle LEDs/buzzer/SSD1306 -> loop.)

#### Inicialização
Configuração de ADC, I2C, PWM e GPIO em C, usando SDK do RP2040.

#### Configurações dos Registros
Ajuste de ADC para microfone/joystick e PWM para buzzer.

#### Estrutura e Formato dos Dados
Valores em float (dB) para ruído e range.

#### Protocolo de Comunicação
I2C para SSD1306; sem protocolo externo inicialmente.

#### Formato do Pacote de Dados
Não aplicável (futuramente, para API, a definir).

---

### d) Execução do Projeto

#### Metodologia
- Pesquisa sobre programação em C para RP2040 e uso de joystick.
- Escolha da BitDogLab como hardware único.
- Configuração do Visual Studio Code com extensões Raspberry Pi Pico.
- Desenvolvimento em C, compilação e depuração.

#### Testes de Validação
- Teste de ajuste de range com joystick.
- Verificação de LEDs, buzzer e SSD1306 com ruídos variados (ex.: 40 dB, 60 dB, 90 dB).
- Teste dos botões A (reinício) e B (BOOTSEL).

#### Discussão dos Resultados
(A ser preenchido após testes, analisando precisão e confiabilidade.)

#### Possibilidade de Ampliação
O projeto pode ser expandido com a integração de um módulo Wi-Fi (ex.: ESP8266 conectado ao RP2040) para enviar dados de ruído a uma API. Esses dados poderiam ser registrados em um banco de dados relacional (ex.: MySQL) ou não relacional (ex.: MongoDB), permitindo visualização em tempo real em uma aplicação web ou mobile, como um dashboard em HTML/CSS/JavaScript ou app em Flutter.

---

### e) Referências
1. **Cugnascas, P. S.** "Metodologia de Projeto de Sistemas Embarcados". In: Revista Brasileira de Engenharia de Sistemas, vol. 10, nº 2, 2015. (Nota: placeholder; substituir por referência real usada na capacitação, conforme orientações fornecidas.)
2. **Raspberry Pi Foundation.** "Raspberry Pi Pico C/C++ SDK Documentation". Disponível em: <https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html>. Acesso em: 22 de fevereiro de 2025. (Documentação oficial do SDK para programação em C no RP2040.)
3. **Embarcados.** "BitDogLab — Uma Jornada Educativa com Eletrônica, Embarcados e IA". Disponível em: <https://www.embarcados.com.br/bitdoglab-uma-jornada-educativa/>. Acesso em: 22 de fevereiro de 2025. (Informações sobre a placa BitDogLab.)
4. **Adafruit.** "SSD1306 OLED Displays with I2C". Disponível em: <https://learn.adafruit.com/monochrome-oled-breakouts>. Acesso em: 22 de fevereiro de 2025. (Guia técnico para displays SSD1306, útil para configuração I2C.)