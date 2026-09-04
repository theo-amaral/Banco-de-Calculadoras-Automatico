function doGet(e) { 
  var nusp = e.parameter.nusp;
  if (!nusp) {
    return ContentService.createTextOutput("ERRO:Falta o parametro nusp");
  }

  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var dados = sheet.getDataRange().getValues();

  for (var i = 1; i < dados.length; i++) {  // Começa de 1 pra ignorar cabeçalho
    if (String(dados[i][3]).trim() === nusp) {  // Coluna D (índice 3) é onde está o NUSP
      return ContentService.createTextOutput("EMPRESTADO");
    }
  }

  return ContentService.createTextOutput("DISPONIVEL");
}
