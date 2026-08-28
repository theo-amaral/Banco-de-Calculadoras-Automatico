#include <HTTPClient.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

// Defines dos pinos
#define SS_PIN 15
#define RST_PIN 5  // RST do RFID
#define LED_PIN 2
#define BUZZER_PIN 4

// Primeira opção de rede wifi a se conectar. Mude o SSID e a senha para atualizar.
#define SSID_A "CEE"
#define PASS_A "C33w!fi#"

// Segunda opção de rede wifi a se conectar. Mude o SSID e a senha para atualizar.
#define SSID_B "WifiCalculadora"
#define PASS_B "CEEcalc123"

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);

// Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {{'1', '2', '3', 'A'}, {'4', '5', '6', 'B'}, {'7', '8', '9', 'C'}, {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {13, 12, 14, 27};  // Ajuste os pinos conforme necessário
byte colPins[COLS] = {26, 25, 33, 32};  // Ajuste os pinos conforme necessário
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String ssidEscolhido;
String passwordEscolhido;

// Variaveis de estado
const String scriptID = "AKfycbzRRR7mqQwMyhBXubDizPepQ6kfTJrFyTQ2SFkhE_jDAOLmhMjW61hr27F6-OV8Qpk";
const String urlNusps =
    "https://script.google.com/macros/s/AKfycbyVa5s6y2LapLeHA-z22L7VFIzWjMWvH34suPs0FrUt7mcPd8i8QGFx_kdKQG1p6F4CuA/";
const String urlCalcs =
    "https://script.google.com/macros/s/AKfycbz2cieuLrqqRdKGE7-ZOuS49DZ8zUu-Nhx8LFCX3YYXwGIkLeU54CuagQ2mTSTgJ87K/";
const String urlLog =
    "https://script.google.com/macros/s/AKfycbzRRR7mqQwMyhBXubDizPepQ6kfTJrFyTQ2SFkhE_jDAOLmhMjW61hr27F6-OV8Qpk/";
String lastUid = "";
String pessoaID = "";
bool esperandoCartao = false;
bool processoIniciado = false;
bool modoDevolucao = false;
bool aguardandoConfirmacao = false;
bool modoCadastroRFID = false;
bool aguardandoConfirmacaoEmprestimoDuplo = false;
bool luzDeFundoLigada = true;
bool modoDevolucaoForcada = false;
String calculadoraID_manual = "";
bool numeroDigitado = false;

void mostrarMensagem(String linha1, String linha2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(linha1.substring(0, 16));  // Linha 1
    lcd.setCursor(0, 1);
    lcd.print(linha2.substring(0, 16));  // Linha 2
}

// Função pra conectar à uma rede passada nos parâmetros
void conectaWifi(String SSID, String pass) {
    Serial.println("Conectando ao Wi-Fi...");
    WiFi.begin(SSID, pass);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
        delay(250);
    }

    Serial.println("\nWi-Fi conectado.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// Função para escolher e conectar à rede correta
void selectWifi() {
    mostrarMensagem("Rede 1: Tecla A", "Rede 2: Tecla B");

    char key;

    // 2. Espera indefinidamente até que 'A' ou 'B' seja pressionado
    while (true) {
        key = keypad.getKey();
        if (key == 'A') {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            ssidEscolhido = SSID_A;
            passwordEscolhido = PASS_A;
            mostrarMensagem("Conectando na", "Rede 1...");
            break;  // Sai do loop de seleção
        } else if (key == 'B') {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            ssidEscolhido = SSID_B;
            passwordEscolhido = PASS_B;
            mostrarMensagem("Conectando na", "Rede 2...");
            break;  // Sai do loop de seleção
        }
    }

    conectaWifi(ssidEscolhido, passwordEscolhido);
}

void verificaConexao() {
    if (WiFi.status() != WL_CONNECTED) {
        mostrarMensagem("Conexao perdida", "Reconectando...");
        conectaWifi(ssidEscolhido, passwordEscolhido);
        mostrarMensagem("Conexao", "recuperada!");
        delay(2000);
    }
}

String accessUrl(String url) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.begin(client, url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    String payload = "";
    if (httpCode > 0) {
        payload = http.getString();
    }
    http.end();

    return payload;
}

// Retorna: 1 para OK, 2 para BANIDO, 0 para ERRO/Não Encontrado
int verificarNUSP(String nusp, String& primeiroNome) {
    String url = urlNusps + "exec?nusp=" + nusp;

    Serial.println("Verificando NUSP: " + nusp);
    String payload = accessUrl(url);
    Serial.println("Status do NUSP: " + payload);
    int posBanido = payload.indexOf("BANIDO:");
    int posOK = payload.indexOf("OK:");

    if (posBanido != -1) {
        // Se encontrou "BANIDO:", extrai o nome e retorna 2
        String temp = payload.substring(posBanido + 7);
        int FimDoNome = -1;
        for (int i = 0; i < temp.length(); i++) {
            if (temp.charAt(i) == '\n' || temp.charAt(i) == '\r') {
                FimDoNome = i;
                break;
            }
        }
        if (FimDoNome != -1) {
            primeiroNome = temp.substring(0, FimDoNome);
        } else {
            primeiroNome = temp;
        }
        primeiroNome.trim();
        return 2;  // STATUS BANIDO

    } else if (posOK != -1) {
        // Se encontrou "OK:", extrai o nome e retorna 1
        String temp = payload.substring(posOK + 3);
        int FimDoNome = -1;
        for (int i = 0; i < temp.length(); i++) {
            if (temp.charAt(i) == '\n' || temp.charAt(i) == '\r') {
                FimDoNome = i;
                break;
            }
        }
        if (FimDoNome != -1) {
            primeiroNome = temp.substring(0, FimDoNome);
        } else {
            primeiroNome = temp;
        }
        primeiroNome.trim();
        return 1;  // STATUS OK
    }

    // Se nada foi encontrado ou houve erro de HTTP
    return 0;  // STATUS ERRO
}

String verificarEmprestimoAtivo(String nusp) {
    String url = urlCalcs + "exec?acao=consulta_emprestimo&nusp=" + nusp;

    String payload = accessUrl(url);

    Serial.println("Verificando emprestimo ativo para NUSP: " + nusp);
    Serial.println("Payload da verificação: " + payload);
    return payload;
}

bool sendData(String nome, String valor, String uid) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    // Usa o scriptID principal (de registro), que você definiu no início do código
    String url = urlLog + "exec?nome=" + nome + "&valor=" + valor + "&uid=" + uid;

    http.begin(client, url);
    // Opcional, mas bom ter: seguir redirecionamentos se houver
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    String payload = http.getString();

    Serial.println("Enviando dados: " + url);
    Serial.println("Resposta do servidor de registro: " + payload);
    Serial.println("HTTP Code do registro: " + httpCode);

    http.end();

    // Consideramos sucesso se o código HTTP for 200 OK
    if (httpCode == 200) {
        return true;
    } else {
        return false;
    }
}

String getCalcNum(String uid) {
    String url = urlCalcs +
                 "exec"
                 "?acao=consulta_uid&uid=" +
                 uid;

    Serial.println("Checando disponibilidade da calculadora com RFID: " + uid);
    String payload = accessUrl(url);
    Serial.println("Payload da disponibilidade: " + payload);

    return payload;
}

String registraCalc(String num, String uid) {
    String url = urlCalcs +
                 "exec"
                 "?acao=registro_uid&uid=" +
                 uid + "&num=" + num;

    Serial.println("Registrando calculadora com numero: " + num);
    String payload = accessUrl(url);
    Serial.println("Payload do registro: " + payload);
    return payload;
}

String uidToString(byte* buffer, byte bufferSize) {
    String result = "";
    for (byte i = 0; i < bufferSize; i++) {
        if (i > 0) result += "-";
        if (buffer[i] < 0x10) result += "0";
        result += String(buffer[i], HEX);
    }
    result.toUpperCase();
    return result;
}

int emprestaCalculadora(String numero, String nusp) {
    String url = urlCalcs +
                 "exec"
                 "?acao=registro_emprestimo&nusp=" +
                 nusp + "&numero=" + numero;

    String payload = accessUrl(url);

    if (payload == "Calculadora emprestada") return 1;
    return 0;
}

int devolveCalculadora(String numero) {
    String url = urlCalcs + "exec?acao=registro_devolucao&numero=" + numero;
    String payload = accessUrl(url);

    if (payload == "Calculadora devolvida") return 1;
    return 0;
}

void setup() {
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    lcd.init();
    lcd.backlight();

    selectWifi();
    mostrarMensagem("Sistema pronto", "Pressione A ou B");
}

void loop() {
    char key = keypad.getKey();

    if (key == '*') {
        Serial.println("--- OPERAÇÃO CANCELADA PELO USUÁRIO (RESET GLOBAL) ---");
        mostrarMensagem("Operacao", "Cancelada");
        digitalWrite(BUZZER_PIN, HIGH);
        delay(300);
        digitalWrite(BUZZER_PIN, LOW);  // Um bipe longo de cancelamento
        delay(1500);

        // Reseta TODAS as variáveis de estado para voltar ao início
        processoIniciado = false;
        modoDevolucao = false;
        esperandoCartao = false;
        aguardandoConfirmacao = false;
        aguardandoConfirmacaoEmprestimoDuplo = false;
        modoCadastroRFID = false;
        pessoaID = "";

        mostrarMensagem("Sistema pronto", "Pressione A ou B");
        rfid.PCD_Init();  // Também reinicializa o leitor RFID por segurança
        return;           // Sai do loop para começar um novo ciclo limpo
    }

    // --- PARTE 1: Menu inicial ---
    // Condição para garantir que só roda se nenhum processo estiver ativo
    if (!processoIniciado && !modoDevolucao && !modoCadastroRFID) {
        if (key == 'A' || key == 'B' || key == 'D') {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            if (key == 'A') {
                processoIniciado = true;
                pessoaID = "";
                mostrarMensagem("Digite o NUSP", "e pressione #");
            } else if (key == 'B') {
                modoDevolucao = true;
                mostrarMensagem("Devolucao", "Aproxime calc.");
            } else if (key == 'D') {
                modoCadastroRFID = true;
                mostrarMensagem("Passe a", "calculadora");
            }
        }
    }

    // --- PARTE 2: Processo de empréstimo ---
    if (processoIniciado) {
        // ESTADO 1: Aguardando confirmação do empréstimo duplo (MAIOR PRIORIDADE)
        if (aguardandoConfirmacaoEmprestimoDuplo) {
            if (key) {  // Se qualquer tecla for pressionada
                digitalWrite(BUZZER_PIN, HIGH);
                delay(100);
                digitalWrite(BUZZER_PIN, LOW);
                if (key == 'A') {
                    // Usuário escolheu continuar mesmo assim
                    aguardandoConfirmacaoEmprestimoDuplo = false;
                    esperandoCartao = true;  // Pula direto para a espera do cartão
                    mostrarMensagem("Confirmado!", "Aproxime calc.");
                } else if (key == 'B') {
                    // Usuário escolheu cancelar
                    mostrarMensagem("Operacao", "Cancelada");
                    delay(2000);
                    // Reseta tudo
                    processoIniciado = false;
                    aguardandoConfirmacaoEmprestimoDuplo = false;
                    pessoaID = "";
                    mostrarMensagem("Sistema pronto", "Pressione A ou B");
                }
            }
        }

        // ESTADO 2: Aguardando confirmação do empréstimo normal
        else if (aguardandoConfirmacao) {
            if (key) {
                digitalWrite(BUZZER_PIN, HIGH);
                delay(100);
                digitalWrite(BUZZER_PIN, LOW);
                if (key == 'A') {
                    aguardandoConfirmacao = false;
                    esperandoCartao = true;
                    mostrarMensagem("Confirmado!", "Aproxime calc.");
                } else if (key == 'B') {
                    mostrarMensagem("Operacao", "Cancelada");
                    delay(2000);
                    // Reseta tudo
                    processoIniciado = false;
                    aguardandoConfirmacao = false;
                    pessoaID = "";
                    mostrarMensagem("Sistema pronto", "Pressione A ou B");
                }
            }
        }

        // ESTADO 3: Aguardando digitação do NUSP (estado inicial do processo)
        else if (!esperandoCartao) {
            if (key) {  // Apenas processa se uma tecla foi pressionada
                if (isDigit(key) && pessoaID.length() < 10) {
                    pessoaID += key;
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(100);
                    digitalWrite(BUZZER_PIN, LOW);
                    mostrarMensagem("NUSP:", pessoaID);
                } else if (key == 'C' && pessoaID.length() > 0) {
                    pessoaID.remove(pessoaID.length() - 1);
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(50);
                    digitalWrite(BUZZER_PIN, LOW);
                    mostrarMensagem("NUSP:", pessoaID);
                } else if (key == '#' && pessoaID.length() > 0) {
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(60);
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);

                    mostrarMensagem("Verificando NUSP", "Aguarde...");
                    String nomeVerificado;
                    int statusNUSP = verificarNUSP(pessoaID, nomeVerificado);  // Nova chamada que retorna um número

                    switch (statusNUSP) {
                        case 1: {  // CASO OK: Usuário existe e não está banido
                            mostrarMensagem("Verificando se", "ha emprestimo...");
                            String statusEmprestimo = verificarEmprestimoAtivo(pessoaID);

                            if (statusEmprestimo.indexOf("DISPONIVEL") != -1) {
                                aguardandoConfirmacao = true;  // Vai para o ESTADO 2
                                mostrarMensagem("Emprestar para:", nomeVerificado);
                                delay(2500);
                                mostrarMensagem("A: Confirma", "B: Cancela");
                            } else if (statusEmprestimo.indexOf("EMPRESTADO") != -1) {
                                mostrarMensagem("NUSP COM", "EMPRESTIMO");
                                digitalWrite(BUZZER_PIN, HIGH);
                                delay(600);
                                digitalWrite(BUZZER_PIN, LOW);
                                delay(2000);
                                aguardandoConfirmacaoEmprestimoDuplo = true;  // Vai para o ESTADO 1
                                mostrarMensagem("A: Continua", "B: Cancela");
                            } else {
                                mostrarMensagem("Erro ao checar", "emprestimos.");
                                delay(2000);
                                processoIniciado = false;  // Reseta
                                pessoaID = "";
                                mostrarMensagem("Sistema pronto", "Pressione A ou B");
                            }
                            break;
                        }
                        case 2:  // CASO BANIDO: Usuário existe mas está banido
                            mostrarMensagem("USUARIO BANIDO", nomeVerificado);
                            digitalWrite(BUZZER_PIN, HIGH);
                            delay(1000);
                            digitalWrite(BUZZER_PIN, LOW);
                            delay(2500);
                            // Reseta o processo
                            processoIniciado = false;
                            pessoaID = "";
                            mostrarMensagem("Sistema pronto", "Pressione A ou B");
                            break;

                        case 0:  // CASO ERRO ou NÃO ENCONTRADO
                        default:
                            mostrarMensagem("NUSP nao", "cadastrado");
                            digitalWrite(BUZZER_PIN, HIGH);
                            delay(500);
                            digitalWrite(BUZZER_PIN, LOW);
                            delay(2000);
                            processoIniciado = false;  // Reseta
                            pessoaID = "";
                            mostrarMensagem("Sistema pronto", "Pressione A ou B");
                            break;
                    }
                }
            }
        }
    }

    // --- PARTE 3: Leitura do RFID ---
    if ((modoDevolucao || esperandoCartao || modoCadastroRFID) && rfid.PICC_IsNewCardPresent() &&
        rfid.PICC_ReadCardSerial()) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(150);
        digitalWrite(BUZZER_PIN, LOW);

        String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();

        if (modoCadastroRFID) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            Serial.println("--- MODO CADASTRO RFID ---");
            Serial.println("UID Lido: " + uid);
            Serial.println("--------------------------");
            mostrarMensagem("Digite o numero", "");
            String numeroCalc = "";

            while (!numeroDigitado && key != '*') {
                key = keypad.getKey();
                if (key) {  // Apenas processa se uma tecla foi pressionada
                    if (isDigit(key) && numeroCalc.length() < 3) {
                        numeroCalc += key;
                        digitalWrite(BUZZER_PIN, HIGH);
                        delay(100);
                        digitalWrite(BUZZER_PIN, LOW);
                        mostrarMensagem("Digite o numero", numeroCalc);
                    } else if (key == 'C' && numeroCalc.length() > 0) {
                        numeroCalc.remove(numeroCalc.length() - 1);
                        digitalWrite(BUZZER_PIN, HIGH);
                        delay(100);
                        digitalWrite(BUZZER_PIN, LOW);
                        mostrarMensagem("Digite o numero", numeroCalc);
                    } else if (key == '#' && numeroCalc.length() > 0) {
                        digitalWrite(BUZZER_PIN, HIGH);
                        delay(80);
                        digitalWrite(BUZZER_PIN, LOW);
                        delay(60);
                        digitalWrite(BUZZER_PIN, HIGH);
                        delay(80);
                        digitalWrite(BUZZER_PIN, LOW);
                        numeroDigitado = true;

                        mostrarMensagem("Registrando a", "calculadora...");
                        String codigo = registraCalc(numeroCalc, uid);
                        if (codigo == "1") {
                            mostrarMensagem("Calculadora", "registrada!");
                        } else if (codigo == "2") {
                            mostrarMensagem("Calculadora ja", "cadastrada!");
                        } else {
                            mostrarMensagem("Erro ao", "cadastrar.");
                        }
                        digitalWrite(BUZZER_PIN, HIGH);
                        delay(80);
                        digitalWrite(BUZZER_PIN, LOW);
                        delay(60);
                        digitalWrite(BUZZER_PIN, HIGH);
                        delay(80);
                        digitalWrite(BUZZER_PIN, LOW);
                        delay(2000);
                    }
                }
                delay(10);
            }
            modoCadastroRFID = false;  // Reseta o estado
            numeroDigitado = false;
            mostrarMensagem("Sistema pronto", "Pressione A ou B");
        }

        else if (modoDevolucao) {
            mostrarMensagem("Verificando registro", "da calculadora...");
            String numero = getCalcNum(uid);
            if (numero != "Calculadora nao registrada") {
                mostrarMensagem("Devolvendo Calc:", numero);
                delay(1500);
                mostrarMensagem("Registrando...", "Aguarde");
                if (devolveCalculadora(numero) && sendData("Calculadora-devolvida", "-", numero)) {
                    mostrarMensagem("Devolucao", "Concluida!");
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(60);
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(2000);
                } else {
                    mostrarMensagem("Erro ao", "devolver.");
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(60);
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(2000);
                }
            } else {
                mostrarMensagem("Calculadora nao", "registrada!");
                digitalWrite(BUZZER_PIN, HIGH);
                delay(80);
                digitalWrite(BUZZER_PIN, LOW);
                delay(60);
                digitalWrite(BUZZER_PIN, HIGH);
                delay(80);
                digitalWrite(BUZZER_PIN, LOW);
                delay(2000);
            }

            // Reset total dos estados após a operação
            modoDevolucao = false;
            processoIniciado = false;
            esperandoCartao = false;
            aguardandoConfirmacao = false;
            aguardandoConfirmacaoEmprestimoDuplo = false;
            pessoaID = "";
            mostrarMensagem("Sistema pronto", "Pressione A ou B");
        } else if (processoIniciado && esperandoCartao) {
            mostrarMensagem("Checando base", "de dados");
            String numero = getCalcNum(uid);
            if (numero != "Calculadora nao registrada") {
                mostrarMensagem("Emprestando Calc:", numero);
                delay(1500);
                mostrarMensagem("Registrando...", "Aguarde");
                if (emprestaCalculadora(numero, pessoaID) && sendData("Calculadora-emprestada", pessoaID, numero)) {
                    mostrarMensagem("Emprestimo", "Concluido!");
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(60);
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(2000);
                } else {
                    mostrarMensagem("Erro ao", "emprestar.");
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(60);
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(2000);
                }
            } else {
                mostrarMensagem("Calculadora nao", "registrada!");
                digitalWrite(BUZZER_PIN, HIGH);
                delay(80);
                digitalWrite(BUZZER_PIN, LOW);
                delay(60);
                digitalWrite(BUZZER_PIN, HIGH);
                delay(80);
                digitalWrite(BUZZER_PIN, LOW);
                delay(2000);
            }

            // Reset total dos estados após a operação
            modoDevolucao = false;
            processoIniciado = false;
            esperandoCartao = false;
            aguardandoConfirmacao = false;
            aguardandoConfirmacaoEmprestimoDuplo = false;
            pessoaID = "";
            mostrarMensagem("Sistema pronto", "Pressione A ou B");
        }

        // Comandos comuns de reset do RFID após qualquer leitura bem sucedida
        rfid.PCD_Init();
        lastUid = uid;
    }
}
