#include <HTTPClient.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pinos da placa
#define SS_PIN 15
#define RST_PIN 5  // RST do RFID
#define LED_PIN 2
#define BUZZER_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

//-----------------------------------------------------------------------------------------------------------------
//-------------------------------------------------Set Wifi--------------------------------------------------------
const char* ssid_1 = "CEE";           // COLOQUE AQUI O NOME DA REDE WIFI QUE O ESP VAI SE CONECTAR
const char* password_1 = "C33w!fi#";  // COLOQUE AQUI A SENHA DA REDE WIFI QUE O ESP VAI SE CONECTR-
//-----------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------
//-------------------------------------------------Set Wifi--------------------------------------------------------
const char* ssid_2 = "WifiCalculadora";  // COLOQUE AQUI O NOME DA REDE WIFI QUE O ESP VAI SE CONECTAR
const char* password_2 = "CEEcalc123";   // COLOQUE AQUI A SENHA DA REDE WIFI QUE O ESP VAI SE CONECTR-
//-----------------------------------------------------------------------------------------------------------------

// Google Apps Script
// Atualizar aqui o Script ID do google, sempre que uma mudança no script for feita, esse código muda

// Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {{'1', '2', '3', 'A'}, {'4', '5', '6', 'B'}, {'7', '8', '9', 'C'}, {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {13, 12, 14, 27};  // Ajuste os pinos conforme necessário
byte colPins[COLS] = {26, 25, 33, 32};  // Ajuste os pinos conforme necessário
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variaveis de estado
const String scriptID = "AKfycby8qK4YqNirCeueu0CnYG41PLmn3DVVXBtGx08rtSmF-u8vMlRVrJTSBwbK3Pd3Eg";
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

// Substitua sua função setup() por esta nova versão:
void mostrarMensagem(String linha1, String linha2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(linha1.substring(0, 16));  // Linha 1
    lcd.setCursor(0, 1);
    lcd.print(linha2.substring(0, 16));  // Linha 2
}

void setup() {
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    lcd.init();
    lcd.backlight();

    // --- INÍCIO DA NOVA LÓGICA DE SELEÇÃO DE REDE ---

    // 1. Mostra o menu de seleção no display
    mostrarMensagem("Rede 1: Tecla A", "Rede 2: Tecla B");

    char key = NO_KEY;
    const char* ssidEscolhido = "";
    const char* passwordEscolhido = "";

    // 2. Espera indefinidamente até que 'A' ou 'B' seja pressionado
    while (true) {
        key = keypad.getKey();
        if (key == 'A') {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            ssidEscolhido = ssid_1;
            passwordEscolhido = password_1;
            mostrarMensagem("Conectando na", "Rede 1...");
            break;  // Sai do loop de seleção
        } else if (key == 'B') {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            ssidEscolhido = ssid_2;
            passwordEscolhido = password_2;
            mostrarMensagem("Conectando na", "Rede 2...");
            break;  // Sai do loop de seleção
        }
    }

    // --- FIM DA NOVA LÓGICA ---

    // 3. Inicia a conexão WiFi com as credenciais que foram escolhidas
    Serial.println("Conectando ao Wi-Fi...");
    WiFi.begin(ssidEscolhido, passwordEscolhido);

    // O resto do código (LED piscando, etc.) continua como antes
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

    digitalWrite(LED_PIN, HIGH);  // Acende o LED azul permanentemente

    // Mostra a mensagem final de sistema pronto
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sistema pronto");
    lcd.setCursor(0, 1);
    lcd.print("Pressione A ou B");
}

// Retorna: 1 para OK, 2 para BANIDO, 0 para ERRO/Não Encontrado
int verificarNUSP(String nusp, String& primeiroNome) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi não conectado");
        return 0;  // ERRO
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    // URL do seu script de verificação de cadastro/banido
    String url =
        "https://script.google.com/macros/s/AKfycbz9WPSQ2U5OE6Vnzwv7RvpDB0svIaownyCjN4iLb0s5l1-3CfxMv3O0dVyScEsRHWYanw/"
        "exec?nusp=" +
        nusp;

    http.begin(client, url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();

    String payload = "";
    if (httpCode == 200) {
        WiFiClient* stream = http.getStreamPtr();
        while (stream->available()) {
            payload += (char)stream->read();
        }
    }
    http.end();

    if (httpCode == 200) {
        // Procura por "BANIDO:" ou "OK:" na resposta
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
    }

    // Se nada foi encontrado ou houve erro de HTTP
    return 0;  // STATUS ERRO
}

// NOVA FUNÇÃO para verificar se um NUSP já tem um empréstimo ativo
String verificarEmprestimoAtivo(String nusp) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi não conectado para checagem de empréstimo");
        return "ERRO_WIFI";
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    // IMPORTANTE: Use o link do seu SEGUNDO script aqui!
    String url =
        "https://script.google.com/macros/s/AKfycbyxzbaYZUv-z2bGPP_TaaesGBAbyNrtJOt7bFS4OkylubIreRHLameWUG9ZNUyrCC8hwg/"
        "exec?nusp=" +
        nusp;

    http.begin(client, url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    String payload = "";
    if (httpCode > 0) {
        payload = http.getString();
    }

    Serial.println("Verificando emprestimo ativo para NUSP: " + nusp);
    Serial.println("URL da verificação: " + url);
    Serial.println("HTTP Code da verificação: " + httpCode);
    Serial.println("Payload da verificação: " + payload);

    http.end();

    return payload;
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
        String numero = getUserName(uid);

        if (modoCadastroRFID) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            Serial.println("--- MODO CADASTRO RFID ---");
            Serial.println("UID Lido: " + uid);
            Serial.println("--------------------------");
            mostrarMensagem("RFID Lido!", "Verifique Serial");
            delay(2500);
            modoCadastroRFID = false;  // Reseta o estado
            mostrarMensagem("Sistema pronto", "Pressione A ou B");
        }

        else if (modoDevolucao) {
            if (numero != "") {
                mostrarMensagem("Devolvendo Calc:", numero);
                delay(1500);
                mostrarMensagem("Registrando...", "Aguarde");
                if (sendData("Calculadora-devolvida", "-", numero)) {
                    mostrarMensagem("Devolucao", "Concluida!");
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(60);
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(2000);
                } else { /* ... (código de erro) ... */
                }
            } else { /* ... (código de erro) ... */
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
            if (numero != "") {
                mostrarMensagem("Emprestando Calc:", numero);
                delay(1500);
                mostrarMensagem("Registrando...", "Aguarde");
                if (sendData("Calculadora-emprestada", pessoaID, numero)) {
                    mostrarMensagem("Emprestimo", "Concluido!");
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(60);
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(80);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(2000);
                } else { /* ... (código de erro) ... */
                }
            } else { /* ... (código de erro) ... */
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

//----------------------------------------------------------------------------------------------------
//---------------------------------------Cadastro de calculadoras-------------------------------------
//----------------------------------------------------------------------------------------------------
/*
  Para cadastrar uma nova calculadora, copie o código abaixo:

      else if (uid == "RFID da calculadora") return "Numero da calculadora"

  E cole logo a baixo no último espaço do else if, seguindo o padrao.
  Para ler o valor do RFID pode-se usar um celular com leitor ou utilizar a função de leitura do próprio sistema
  basta apertar o botão D no sistema, enquanto estiver conectado no PC, e checar o valor impresso no monitor serial
*/
//----------------------------------------------------------------------------------------------------
// AQUI QUE CADASTRA
String getUserName(String uid) {
    if (uid == "00") return "1";
    // else if (uid == "perdida            ") return "9";
    // else if (uid == "emprestada        ") return "10";
    // else if (uid == "perdida           ") return "11";
    else if (uid == "04-74-6D-52-DA-61-80")
        return "12";
    else if (uid == "04-35-6C-52-DA-61-80")
        return "13";
    else if (uid == "04-39-6C-52-DA-61-80")
        return "14";
    else if (uid == "04-CD-66-52-DA-61-80")
        return "15";
    else if (uid == "04-C1-66-52-DA-61-80")
        return "16";
    // else if (uid == "perdida           ") return "17";
    else if (uid == "04-C5-66-52-DA-61-80")
        return "18";
    else if (uid == "04-92-6B-52-DA-61-80")
        return "19";
    else if (uid == "04-5C-6D-52-DA-61-80")
        return "20";
    else if (uid == "04-8A-6B-52-DA-61-80")
        return "21";
    else if (uid == "04-C9-66-52-DA-61-80")
        return "22";
    // else if (uid == "emprestada        ") return "23";
    else if (uid == "04-BD-66-52-DA-61-80")
        return "24";
    // else if (uid == "perdida           ") return "25";
    // else if (uid == "emprestada        ") return "26";
    else if (uid == "04-B9-66-52-DA-61-80")
        return "27";
    else if (uid == "04-B6-67-52-DA-61-80")
        return "28";
    else if (uid == "04-49-6C-52-DA-61-80")
        return "29";
    else if (uid == "04-4D-6C-52-DA-61-80")
        return "30";
    else if (uid == "04-95-6A-52-DA-61-80")
        return "31";
    else if (uid == "04-41-6C-52-DA-61-80")
        return "32";
    else if (uid == "04-30-6C-52-DA-61-80")
        return "33";
    else if (uid == "06-7D-6C-FA")
        return "1";
    else
        return "";
}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

bool sendData(String nome, String valor, String udi) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Erro: WiFi não conectado para envio de dados.");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    // Usa o scriptID principal (de registro), que você definiu no início do código
    String url =
        "https://script.google.com/macros/s/" + scriptID + "/exec?nome=" + nome + "&valor=" + valor + "&uid=" + udi;

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
