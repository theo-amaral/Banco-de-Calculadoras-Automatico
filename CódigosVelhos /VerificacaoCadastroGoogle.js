function updateCalculadoras() {
  try {
    const ss = SpreadsheetApp.getActiveSpreadsheet();
    const logSheet = ss.getSheetByName("LOG");
    const cadastrosSheet = ss.getSheetByName("Cadastros");
    
    if (!logSheet || !cadastrosSheet) {
      throw new Error("Planilhas 'LOG' ou 'Cadastros' não encontradas!");
    }

    let controleSheet = ss.getSheetByName("Controle");
    if (!controleSheet) {
      controleSheet = ss.insertSheet("Controle");
    } else {
      controleSheet.clear();
    }

    controleSheet.appendRow(["Calculadora", "Status", "NUSP", "Nome", "Tempo Empréstimo (horas)", "Última Atualização"]);

    const logs = logSheet.getDataRange().getValues().filter(r => r[0]);
    const cadastros = cadastrosSheet.getDataRange().getValues().filter(r => r[0]);

    for (let calc = 1; calc <= 30; calc++) {
      try {
        const transacoes = logs.filter(t => t[4] == calc);
        
        let status = "Disponível";
        let nusp = "";
        let nome = "";
        let tempo = "";

        if (transacoes.length > 0) {
          const ultima = transacoes[transacoes.length - 1];
          const dataTexto = ultima[0];
          const horaTexto = ultima[1];
          status = ultima[2].includes("emprestada") ? "Emprestada" : "Disponível";

          if (status === "Emprestada") {
            nusp = ultima[3];

            let dataEmprestimo = null;
            try {
              const [dia, mes, ano] = dataTexto.split('/');
              const dataStr = `${mes}/${dia}/${ano} ${horaTexto}`;
              dataEmprestimo = new Date(dataStr);
              tempo = Math.floor((new Date() - dataEmprestimo) / (1000 * 60 * 60)) + "h";
            } catch (e) {
              Logger.log(`Erro ao converter data para a calculadora ${calc}: ${e.message}`);
            }

            const aluno = cadastros.find(c => c[3] == nusp);
            nome = aluno ? aluno[2] : "Não encontrado";
          }
        }

        controleSheet.appendRow([calc, status, nusp, nome, tempo, new Date()]);
      } catch (e) {
        Logger.log(`Erro ao processar a calculadora ${calc}: ${e.message}`);
      }
    }

    Logger.log("Atualização concluída com sucesso!");
  } catch (e) {
    Logger.log("Erro crítico: " + e.message);
    SpreadsheetApp.getUi().alert("Erro: " + e.message);
  }
}



















