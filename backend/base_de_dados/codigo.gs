function doGet(e) {
  var resultado = 'Ok';
  var planilha = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var dados = planilha.getDataRange().getValues();
  if(e.parameter == undefined) {
    resultado = 'Nenhum Parametro.';
  }
  else if (e.parameter.acao == 'consulta_uid') {
    var uid = e.parameter.uid;

    for(var i = 0; i < dados.length; i++) {
      if(dados[i][1] == uid) {
        var numero = dados[i][0];
        return ContentService.createTextOutput(numero);
      }
    }
    return ContentService.createTextOutput('Calculadora nao registrada');
  }
  else if (e.parameter.acao == 'registro_uid') {
    var ultimaLinha = planilha.getLastRow();
    var uid = e.parameter.uid;
    var num = e.parameter.num;
    for(var i = 0; i < dados.length; i++) {
      if(dados[i][1] == uid) {
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
    for(var i = 0; i < dados.length; i++) {
      if(dados[i][3] == nusp) return ContentService.createTextOutput('EMPRESTADO');
    }
    return ContentService.createTextOutput('DISPONIVEL');
  }
  else if (e.parameter.acao == 'registro_emprestimo') {
    var numero = e.parameter.numero;
    var nusp = e.parameter.nusp;
    var linha = 0;

    for(var i = 0; i < dados.length; i++) {
      if(dados[i][0] == numero){
        linha = i + 1;
      }
    }
    if (linha != 0){
      planilha.getRange(linha, 3).setValue('Emprestada');
      planilha.getRange(linha, 4).setValue(nusp);
      planilha.getRange(linha, 5).setValue(Utilities.formatDate(new Date(), "GMT-3", "dd/MM/yyyy"));
      planilha.getRange(linha, 6).setValue(Utilities.formatDate(new Date(), "GMT-3", "HH:mm:ss"));
      return ContentService.createTextOutput('Calculadora emprestada');
    }
    return ContentService.createTextOutput('Calculadora nao encontrada');
  } 
  else if (e.parameter.acao == "registro_devolucao") {
    var numero = e.parameter.numero;

    for(var i = 0; i < dados.length; i++) {
      if(dados[i][0] == numero) {
        planilha.getRange(i+1, 3).setValue('Disponivel');
        planilha.getRange(i+1, 4).setValue('');
        planilha.getRange(i+1, 5).setValue('');
        planilha.getRange(i+1, 6).setValue('');
        return ContentService.createTextOutput('Calculadora devolvida');
      }
    }
    return ContentService.createTextOutput('Calculadora nao encontrada');
  }
  else if (e.parameter.acao == "registro_num") {
    var num = e.parameter.num;
    
    for(var i = 0; i < dados.length; i++) {
      if(dados[i][0] == num) {
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
