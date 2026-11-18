# Roteiro do vídeo — Physical Computing (até 4 minutos)

O vídeo precisa: (1) apresentar brevemente a solução, (2) explicar como o circuito ESP32 entra nela, (3) mostrar o funcionamento no Wokwi.

| Tempo | O que mostrar | O que falar |
|---|---|---|
| 0:00–0:30 | Rosto ou slide com o nome Quadra Viva | O problema: o morador só descobre o alagamento quando a rua já alagou, porque a boca de lobo entupida não avisa ninguém. A solução: sensores nas bocas de lobo, previsão de chuva e quadras públicas que viram reservatório. |
| 0:30–1:00 | Print do app mobile (Início) | O app é a porta do cidadão. O circuito é a camada física que alimenta o app com o risco de cada rua. |
| 1:00–1:45 | Circuito no Wokwi, parado | Sonar no topo da boca de lobo mede o lixo acumulado. Potenciômetro simula o sensor de lâmina d'água. Botão simula o alerta de chuva da IA. Saídas: LEDs, buzzer e servo como cancela da quadra. |
| 1:45–3:15 | Simulação rodando + monitor serial | Seguir a tabela "Roteiro de teste" do README: verde, amarelo, botão de chuva sobe para vermelho e trava a cancela, crítico com buzzer, voltar ao verde. Mostrar o JSON na Serial mudando. |
| 3:15–3:45 | Trecho do `sketch.ino` com `classificar()` | A regra de decisão e o ponto principal: com chuva prevista, o nó age antes da água chegar. |
| 3:45–4:00 | Slide final | Impacto: bocas desobstruídas antes da chuva. ODS 11, 13 e 9. Citar os integrantes. |

Dicas:
- Gravar a tela com o Wokwi em tela cheia e a Serial visível.
- Falar só o que o circuito faz de verdade; dizer claramente que os sensores são simulados.
- Subir no YouTube como **não listado** e conferir numa aba anônima.
