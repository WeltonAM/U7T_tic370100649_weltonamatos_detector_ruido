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

Passo 1: Inicializando o Sistema
Conectar o Raspberry Pi Pico ao computador usando um cabo micro USB.
Carregar o programa no Raspberry Pi Pico. Para isso, compile o código fornecido e faça o upload do arquivo .uf2 no dispositivo montado.
O sistema inicializará automaticamente. Você verá um texto de boas-vindas no display SSD1306 e os LEDs WS2812 acesos em azul, indicando que o sistema está pronto para ser configurado.

Passo 2: Configuração dos Limites de Ruído
O sistema pede para definir os limites mínimo e máximo de ruído, o que será feito através da interface de display e joystick.

Tela Inicial: O display exibirá a mensagem "Detector De Ruído Iniciado". Para continuar, pressione o Botão A.
Configuração do Valor Mínimo:
O display exibirá a configuração do valor mínimo (ex: Min: 000).
Use o Joystick X (esquerda/direita) para aumentar ou diminuir o valor do dígito selecionado.
Use o Joystick Y (cima/baixo) para mover o cursor entre os dígitos.
Após definir o valor mínimo, pressione o Botão A para passar para a configuração do valor máximo.
Configuração do Valor Máximo:
O display exibirá a configuração do valor máximo (ex: Max: 0000).
Novamente, use o Joystick X e Joystick Y para ajustar os dígitos.
Após definir o valor máximo, pressione o Botão A para finalizar a configuração.

Passo 3: Modo de Operação
Com os limites definidos, o sistema começa a monitorar o nível de ruído.

Monitoramento de Ruído:

O sistema irá constantemente monitorar o nível do microfone.
Se o nível de ruído estiver dentro do intervalo predefinido, os LEDs WS2812 estarão verdes.
Se o nível de ruído sair do intervalo (acima ou abaixo dos limites definidos), o sistema exibirá uma mensagem de alerta "FORA DO RANGE" no display e acionará os LEDs vermelhos.
O buzzer emitirá um som de SOS para alertar sobre o desvio do intervalo.
Solução de Problemas:

Se os LEDs ficarem vermelhos e o sistema exibir a mensagem "FORA DO RANGE", significa que o valor do microfone está fora do intervalo definido.
O buzzer continuará emitindo o sinal de SOS até que o sistema seja reiniciado pressionando o Botão A.
Após pressionar o Botão A, o sistema voltará à tela de configuração e permitirá que os limites sejam ajustados novamente.

Passo 4: Reinicializando o Sistema
Se você desejar reiniciar o sistema ou realizar um reset para ajustar os parâmetros, faça o seguinte:

Pressione o Botão A para reiniciar o sistema, reiniciando o Detector de Ruído, entrando novamente no modo de configuração.

Ou pressione o Botão B para reiniciar o Raspberry Pi Pico e entrar no modo BOOTSEL.
Isso apagará o código atual do dispositivo e permitirá reprogramá-lo se necessário.


Passo 5: Desligando o Sistema
Botão B: Entrar no modo BOOTSEL para reprogramar o dispositivo.

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
O microfone captura o ruído, cujo valor é comparado a um range ajustável (ex.: 100-4000 dB) definido pelo usuário via joystick e confirmado pelo botão do A. A matriz de LEDs acende em verde se o ruído estiver no range, ou vermelho se estiver fora, acionando o buzzer com sinal SOS e exibindo o valor no SSD1306. O botão A reinicia o programa; o botão B entra no modo BOOTSEL, limpando LEDs e display.

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
- **Display SSD1306**: I2C (SDA em GP14, SCL em GP15). 
- **Buzzer**: PWM (ex.: GP21).
- **Joystick**: Eixos X/Y em ADC (GP26, GP27).
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
- Pesquisa sobre conceito de embarcado e hardware único.
- Escolha da BitDogLab como hardware único.
- Configuração do Visual Studio Code com extensões Raspberry Pi Pico.
- Desenvolvimento em C, compilação e depuração.

#### Testes de Validação
- Teste de ajuste de range com joystick.
- Verificação de LEDs, buzzer e SSD1306.
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