function doGet(e) {
  var resultado = 'Ok';
  var planilha = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var numeroLinhas = planilha.getRange('A:A').getNumRows();
  
  if(e.parameter == undefined) {
    resultado = 'Nenhum Parametro.';
  }
  else if (e.parameter.acao == 'consulta_uid') {
    var uid = e.parameter.uid;

    var colunaUids = planilha.getRange('B:B').getValues();
    for (var i = 0; i < numeroLinhas; i++){
      if (colunaUids[i][0] == uid) {
        return ContentService.createTextOutput(planilha.getRange(i + 1, 1).getValue());
      }
    }
    return ContentService.createTextOutput('Calculadora nao registrada');
  }
  else if (e.parameter.acao == 'registro_uid') {
    var ultimaLinha = planilha.getLastRow();
    var uid = e.parameter.uid;
    var num = e.parameter.num;

    var colunaUids = planilha.getRange('B:B').getValues();
    for(var i = 0; i < numeroLinhas; i++) {
      if(colunaUids[i][0] == uid) {
        return ContentService.createTextOutput('2');
      }
    }
    planilha.appendRow([num, uid, 'Disponivel']);
    var ultimaLinha = planilha.getLastRow();
    var ultimaColuna = planilha.getLastColumn();

    planilha.getRange(2, 1, ultimaLinha - 1, ultimaColuna).sort(1);
    return ContentService.createTextOutput('1');
  }
  else if (e.parameter.acao == 'consulta_emprestimo') {
    var nusp = e.parameter.nusp;

    var colunaNusps = planilha.getRange('D:D').getValues();
    for(var i = 0; i < numeroLinhas; i++) {
      if(colunaNusps[i][0] == nusp) return ContentService.createTextOutput('EMPRESTADO');
    }
    return ContentService.createTextOutput('DISPONIVEL');
  }
  else if (e.parameter.acao == 'registro_emprestimo') {
    var numero = e.parameter.numero;
    var nusp = e.parameter.nusp;
    var linha = 0;

    var colunaNumeros = planilha.getRange('A:A').getValues();
    for(var i = 0; i < numeroLinhas; i++) {
      if(colunaNumeros[i][0] == numero){
        linha = i + 1;
      }
    }
    if (linha != 0){
      planilha.getRange(linha, 3, 1, 4).setValues([['Emprestada', nusp, Utilities.formatDate(new Date(), "GMT-3", "dd/MM/yyyy"), Utilities.formatDate(new Date(), "GMT-3", "HH:mm:ss")]])
      return ContentService.createTextOutput('Calculadora emprestada');
    }
    return ContentService.createTextOutput('Calculadora nao encontrada');
  } 
  else if (e.parameter.acao == "registro_devolucao") {
    var numero = e.parameter.numero;

    var colunaNumeros = planilha.getRange('A:A').getValues();
    for(var i = 0; i < numeroLinhas; i++) {
      if(colunaNumeros[i][0] == numero) {
        planilha.getRange(i + 1, 3, 1, 4).setValues([['Disponivel', '', '', '']]);
        return ContentService.createTextOutput('Calculadora devolvida');
      }
    }
    return ContentService.createTextOutput('Calculadora nao encontrada');
  }
  else if (e.parameter.acao == "registro_num") {
    var num = e.parameter.num;

    var colunaNumeros = planilha.getRange('A:A').getValues();
    for(var i = 0; i < numeroLinhas; i++) {
      if(colunaNumeros[i][0] == num) {
        return ContentService.createTextOutput('2');
      }
    }
    planilha.appendRow([num, '-', 'Disponivel']);
    var ultimaLinha = planilha.getLastRow();
    var ultimaColuna = planilha.getLastColumn();

    planilha.getRange(2, 1, ultimaLinha - 1, ultimaColuna).sort(1);
    return ContentService.createTextOutput('1');
  }
  else {
    return ContentService.createTextOutput('0');
  }
}
