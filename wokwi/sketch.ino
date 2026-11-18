/*
  Quadra Viva — nó sensor da boca de lobo (ESP32 + Wokwi)
  Disciplina: Physical Computing: IoT & IoB · FIAP Global Solution 2026.2

  O que o nó faz:
    1. Mede o nível de lixo/sedimento dentro da boca de lobo (HC-SR04) -> obstrução em %.
    2. Mede a lâmina d'água na rua (potenciômetro simulando o sensor de nível) -> cm.
    3. Recebe a "previsão de chuva" (botão simulando o alerta do modelo de IA).
    4. Classifica o risco (baixo / atenção / alto / crítico), com os mesmos níveis do app mobile.
    5. Avisa: LEDs, buzzer (só no crítico) e servo (cancela que trava a quadra-reservatório).
    6. Publica no Serial um JSON no mesmo formato da "Rua" do app (data/types.ts).

  Os valores de limite abaixo são hipóteses de projeto, não normas técnicas.
*/

#include <ESP32Servo.h>

// ---------- Pinos ----------
const int PIN_TRIG = 5;
const int PIN_ECHO = 18;
const int PIN_LAMINA = 34;  // entrada analógica (ADC1)
const int PIN_LED_VERDE = 25;
const int PIN_LED_AMARELO = 26;
const int PIN_LED_VERMELHO = 27;
const int PIN_BUZZER = 23;
const int PIN_SERVO = 13;
const int PIN_BOTAO = 4;  // INPUT_PULLUP: pressionado = LOW

// ---------- Identificação do nó ----------
const char* RUA_ID = "r1";  // Rua das Acácias, como no mock do app
const char* BOCA_ID = "bl-01";

// ---------- Geometria e limites (hipóteses de projeto) ----------
const float PROFUNDIDADE_CM = 60.0;  // sensor no topo -> fundo limpo da boca de lobo
const float LAMINA_MAX_CM = 30.0;    // fundo de escala do sensor de lâmina d'água

const float OBSTRUCAO_ATENCAO = 40.0;  // %
const float OBSTRUCAO_ALTO = 70.0;     // %
const float LAMINA_ATENCAO = 3.0;      // cm
const float LAMINA_ALTO = 8.0;         // cm
const float LAMINA_CRITICO = 15.0;     // cm

const int ANGULO_ABERTA = 0;
const int ANGULO_TRAVADA = 90;

const unsigned long INTERVALO_LEITURA_MS = 500;
const unsigned long INTERVALO_RELATORIO_MS = 2000;
const unsigned long DEBOUNCE_MS = 50;

enum Risco { BAIXO, ATENCAO, ALTO, CRITICO };
const char* NOME_RISCO[] = {"baixo", "atencao", "alto", "critico"};

Servo cancela;

float obstrucaoPct = 0;
float laminaCm = 0;
bool sensorOk = true;
bool chuvaPrevista = false;
bool quadraTravada = false;
Risco risco = BAIXO;

unsigned long ultimaLeitura = 0;
unsigned long ultimoRelatorio = 0;
unsigned long ultimoBotao = 0;
bool botaoAnterior = HIGH;

// Distância em cm; retorna -1 se não houver eco (sensor desconectado ou fora de alcance).
float medirDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long dur = pulseIn(PIN_ECHO, HIGH, 30000UL);  // timeout 30 ms (~5 m)
  if (dur == 0) return -1;
  return dur * 0.0343f / 2.0f;
}

// Mediana de 5 leituras para ignorar picos (respingos, ruído).
float distanciaFiltradaCm() {
  float v[5];
  int n = 0;
  for (int i = 0; i < 5; i++) {
    float d = medirDistanciaCm();
    if (d > 0) v[n++] = d;
    delay(15);
  }
  if (n == 0) return -1;
  for (int i = 1; i < n; i++) {  // ordenação por inserção
    float k = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > k) {
      v[j + 1] = v[j];
      j--;
    }
    v[j + 1] = k;
  }
  return v[n / 2];
}

Risco classificar(float obstrucao, float lamina, bool previsao) {
  Risco r = BAIXO;
  if (obstrucao >= OBSTRUCAO_ATENCAO || lamina >= LAMINA_ATENCAO) r = ATENCAO;
  if (obstrucao >= OBSTRUCAO_ALTO || lamina >= LAMINA_ALTO) r = ALTO;
  if (lamina >= LAMINA_CRITICO || (obstrucao >= OBSTRUCAO_ALTO && lamina >= LAMINA_ALTO)) r = CRITICO;
  // Boca de lobo já parcialmente entupida + chuva prevista: agir antes da chuva.
  if (previsao && obstrucao >= OBSTRUCAO_ATENCAO && r < ALTO) r = ALTO;
  return r;
}

void atualizarSaidas() {
  digitalWrite(PIN_LED_VERDE, risco == BAIXO);
  digitalWrite(PIN_LED_AMARELO, risco == ATENCAO);
  digitalWrite(PIN_LED_VERMELHO, risco >= ALTO);

  // A quadra trava quando o risco é crítico, ou alto com chuva prevista.
  quadraTravada = (risco == CRITICO) || (risco == ALTO && chuvaPrevista);
  cancela.write(quadraTravada ? ANGULO_TRAVADA : ANGULO_ABERTA);
}

// Buzzer intermitente (sem delay) só no nível crítico.
void atualizarBuzzer() {
  if (risco == CRITICO && (millis() / 400) % 2 == 0) {
    tone(PIN_BUZZER, 1000);
  } else {
    noTone(PIN_BUZZER);
  }
}

void lerBotao() {
  bool atual = digitalRead(PIN_BOTAO);
  if (atual == LOW && botaoAnterior == HIGH && millis() - ultimoBotao > DEBOUNCE_MS) {
    chuvaPrevista = !chuvaPrevista;
    ultimoBotao = millis();
    Serial.print(F("# Previsao de chuva (IA): "));
    Serial.println(chuvaPrevista ? F("ATIVADA") : F("desativada"));
    risco = classificar(obstrucaoPct, laminaCm, chuvaPrevista);
    atualizarSaidas();
  }
  botaoAnterior = atual;
}

void lerSensores() {
  float d = distanciaFiltradaCm();
  sensorOk = d > 0;
  if (sensorOk) {
    obstrucaoPct = constrain((PROFUNDIDADE_CM - d) / PROFUNDIDADE_CM * 100.0f, 0.0f, 100.0f);
  }
  int bruto = analogRead(PIN_LAMINA);  // 0..4095
  laminaCm = bruto * LAMINA_MAX_CM / 4095.0f;

  Risco novo = classificar(obstrucaoPct, laminaCm, chuvaPrevista);
  if (novo != risco) {
    risco = novo;
    atualizarSaidas();
  }
}

// Mesmo formato dos campos da "Rua" do app (risco, laminaCm, bocasObstruidas).
void publicar() {
  Serial.print(F("{\"ruaId\":\""));
  Serial.print(RUA_ID);
  Serial.print(F("\",\"bocaId\":\""));
  Serial.print(BOCA_ID);
  Serial.print(F("\",\"risco\":\""));
  Serial.print(NOME_RISCO[risco]);
  Serial.print(F("\",\"laminaCm\":"));
  Serial.print(laminaCm, 1);
  Serial.print(F(",\"obstrucaoPct\":"));
  Serial.print(obstrucaoPct, 0);
  Serial.print(F(",\"bocaObstruida\":"));
  Serial.print(obstrucaoPct >= OBSTRUCAO_ALTO ? F("true") : F("false"));
  Serial.print(F(",\"chuvaPrevista\":"));
  Serial.print(chuvaPrevista ? F("true") : F("false"));
  Serial.print(F(",\"quadraTravada\":"));
  Serial.print(quadraTravada ? F("true") : F("false"));
  Serial.print(F(",\"sensorOk\":"));
  Serial.print(sensorOk ? F("true") : F("false"));
  Serial.println(F("}"));
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AMARELO, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  cancela.attach(PIN_SERVO);

  Serial.println(F("# Quadra Viva - no sensor da boca de lobo"));
  Serial.println(F("# Botao = previsao de chuva (IA) liga/desliga"));
  lerSensores();
  atualizarSaidas();
  publicar();
}

void loop() {
  unsigned long agora = millis();
  lerBotao();
  atualizarBuzzer();

  if (agora - ultimaLeitura >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = agora;
    lerSensores();
  }
  if (agora - ultimoRelatorio >= INTERVALO_RELATORIO_MS) {
    ultimoRelatorio = agora;
    publicar();
  }
}
