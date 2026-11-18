# Prompts de IA — Physical Computing

IA usada como copiloto para estruturar o circuito e o código. Cada integrante deve saber explicar todas as linhas do `sketch.ino`.

| # | Objetivo | Prompt (resumo) | O que foi aproveitado |
|---|---|---|---|
| 1 | Encaixar o IoT na ideia | "Temos a CitizenTech Quadra Viva (quadras que viram reservatório, sensores nas bocas de lobo, app mobile). Quero um circuito ESP32 no Wokwi que faça parte dessa solução." | Escolha de sensores: sonar para obstrução, potenciômetro como lâmina d'água, botão como previsão de IA. |
| 2 | Código | "Escreva o sketch do ESP32 que classifica o risco com os mesmos níveis do app (baixo, atenção, alto, crítico) e publica JSON na Serial." | Estrutura do sketch, mediana de leituras, `millis()` no lugar de `delay`. |
| 3 | Validação | "Compile o sketch para a placa esp32:esp32:esp32." | Compilação sem erros. |

Revisões feitas pela equipe: _pendente_ (a equipe deve registrar aqui o que ajustou, por exemplo os limites de obstrução e de lâmina d'água).
