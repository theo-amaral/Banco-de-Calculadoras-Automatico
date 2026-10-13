function doGet(e) { 
  var nusp = e.parameter.nusp;
  if (!nusp) {
    return ContentService.createTextOutput("ERRO:Falta o parametro nusp");
  }

  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var dados = sheet.getDataRange().getValues();

  for (var i = 1; i < dados.length; i++) {  // Começa de 1 pra ignorar cabeçalho
    if (dados[i][3] == nusp) {  // Coluna D (índice 3) é onde está o NUSP
      var nomeCompleto = dados[i][2]; // Coluna C (índice 2) é o nome
      var primeiroNome = nomeCompleto.trim().split(" ")[0];
      return ContentService.createTextOutput("OK:" + primeiroNome);
    }
  }

  return ContentService.createTextOutput("ERRO:Usuario nao encontrado");
}
