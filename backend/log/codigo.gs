function doGet(e) {
  Logger.log(JSON.stringify(e));

  if (!e || !e.parameter) {
    return ContentService.createTextOutput('Nenhum parametro');
  }

  var rid = e.parameter.rid;
  var lock = null;
  var cache = null;
  var chave = null;

  try {
    // Só protege e deduplica quando o ESP32 manda o rid
    if (rid) {
      lock = LockService.getScriptLock();
      try {
        lock.waitLock(15000);
      } catch (err) {
        return ContentService.createTextOutput('Ocupado');
      }

      cache = CacheService.getScriptCache();
      chave = 'rid_' + rid;

      // Chave já existe: é repetição de uma requisição que já foi gravada
      if (cache.get(chave) !== null) {
        return ContentService.createTextOutput('Ok');
      }
    }

    gravaLog(e.parameter);
    SpreadsheetApp.flush();

    // Só anota o rid depois que a gravação terminou com sucesso
    if (cache) {
      cache.put(chave, '1', 600);
    }

    return ContentService.createTextOutput('Ok');
  } finally {
    if (lock && lock.hasLock()) {
      lock.releaseLock();
    }
  }
}

function gravaLog(p) {
  var planilhaID = '1xM6hMVZNCEZ8KsejYHULakkqIxFFeZxgFyqpWZld4ZU'; // Spreadsheet ID
  var planilha = SpreadsheetApp.openById(planilhaID).getActiveSheet();
  var dados = planilha.getDataRange().getValues();
  var num = p.uid;

  var agora = new Date();
  var horaAtual = Utilities.formatDate(agora, 'America/Sao_Paulo', 'HH:mm:ss');
  var dataAtual = Utilities.formatDate(agora, 'America/Sao_Paulo', 'dd/MM/yyyy');

  if (p.nome == 'Calculadora-emprestada') {
    for (var i = dados.length - 1; i >= 0; i--) {
      if (dados[i][4] == num) {
        if (dados[i][2] == 'Calculadora-emprestada') {
          planilha.appendRow([dataAtual, horaAtual, 'Calculadora-devolvida', 'GERADO AUTOMATICAMENTE - VERIFICAR DEVOLUÇÃO', num]);
        } else {
          break;
        }
      }
    }
  }

  planilha.appendRow([dataAtual, horaAtual, p.nome, p.valor, p.uid]);
}

function stripQuotes(value) {
  return value.replace(/^["']|['"]$/g, "");
}

function testeDedupe() {
  var rid = 'teste' + Date.now();
  var base = {nome: 'Calculadora-devolvida', valor: '-', uid: 'TESTE'};

  var r1 = doGet({parameter: base});                                            // sem rid
  var r2 = doGet({parameter: Object.assign({rid: rid}, base)});                 // rid novo
  var r3 = doGet({parameter: Object.assign({rid: rid}, base)});                 // mesmo rid
  Logger.log([r1, r2, r3].map(function (r) { return r.getContent(); }));
}
