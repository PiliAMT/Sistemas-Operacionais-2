# MySh - Mini Shell para Linux

Implementação de um interpretador de comandos (shell) minimalista em C para ambiente Linux, desenvolvido como projeto da disciplina de Sistemas Operacionais 2.

O MySh atua como intermediário entre o usuário e o kernel: exibe um prompt interativo, lê comandos digitados, localiza e executa os programas correspondentes, e garante que nenhum processo filho seja deixado em estado inválido após a execução.

---

## Recursos

- **Prompt dinâmico:** exibe `usuario@host:/caminho/atual$` com cores ANSI, atualizado a cada iteração.
- **Execução via PATH:** comandos como `ls`, `grep` e `gcc` são localizados automaticamente nas pastas do PATH do sistema.
- **Execução via caminho:** suporte a caminhos absolutos (`/bin/ls`) e relativos (`./meu_prog`).
- **Gestão de argumentos:** todos os argumentos digitados são capturados e repassados ao programa chamado.
- **Comandos internos:**
  - `exit` - encerra o shell imediatamente.
  - `cd <dir>` - muda o diretório de trabalho do processo pai (não pode ser feito via `fork`).
- **Tratamento de erros:** mensagens descritivas para comandos inexistentes, caminhos inválidos e permissão negada, usando `strerror(errno)`.
- **Prevenção de processos zumbi:** handler `SIGCHLD` via `sigaction` colhe processos filhos terminados de forma não-bloqueante, evitando acúmulo na tabela de processos.
- **Segurança de memória:** buffer de entrada zerado a cada iteração com `memset`; zero memory leaks verificado com Valgrind.
- **Independência:** não utiliza `system()` - não depende de nenhuma shell pré-existente.

---

## Requisitos

- Sistema operacional Linux (ou WSL no Windows).
- Compilador `gcc` com suporte a GNU11 (`-std=gnu11`).
- Ferramenta `make`.

Não há dependências de bibliotecas externas.

---

## Compilação e execução

**1. Clone o repositório:**
```
git clone https://github.com/PiliAMT/Sistemas-Operacionais-2.git
cd Sistemas-Operacionais-2
```

**2. Compile:**
```
make
```
O `make clean` é recomendado para garantir que não haja arquivos-objeto (`.o`) antigos:
```
make clean && make
```

**3. Execute:**
```
./mysh
```

**4.** O prompt será exibido, indicando que o shell está pronto:
```
MySh iniciado. Digite 'exit' ou Ctrl+D para sair.
user@DESKTOP:/mnt/c/projeto$ 
```

**5.** Digite quaisquer comandos do sistema:
```
user@DESKTOP:/mnt/c/projeto$ ls -l /tmp
user@DESKTOP:/mnt/c/projeto$ cd /var/log
user@DESKTOP:/tmp$ grep -r "erro" /var/log
```

**6.** Para encerrar, use `exit` ou `Ctrl+D`:
```
user@DESKTOP:/mnt/c/projeto$ exit
Encerrando MySh...
```

---

## Arquitetura e fluxo de execução

O MySh é estruturado em módulos independentes, cada um compilado separadamente e integrado via `mysh.h`.

### Fase 1 - Inicialização (`main.c`)

Antes do loop principal, `main()` chama `setup_signals()` para registrar o handler de `SIGCHLD`. Isso garante que **nenhum** processo filho criado durante a sessão se torne zumbi, desde o primeiro comando executado.

### Fase 2 - Prompt e leitura (`parser.c`)

`show_prompt()` consulta `getcwd()` e `gethostname()` para montar e exibir o prompt dinâmico. Em seguida, `main()` chama `fgets()` para ler a linha digitada pelo usuário de forma segura, limitando a leitura a `MAX_INPUT` caracteres.

### Fase 3 - Parsing (`parser.c`)

`parse_input()` recebe a string bruta e a tokeniza com `strtok`, usando espaço, tab e newline como delimitadores. O resultado é um array `args[]` de ponteiros, terminado em `NULL` — formato exato exigido pelo `execvp`. Múltiplos espaços consecutivos são ignorados automaticamente.

### Fase 4 - Comandos internos (`main.c`)

Antes de criar qualquer processo filho, `main()` verifica se o comando é um **built-in**:
- `exit` → quebra o loop e encerra o shell.
- `cd` → chama `chdir()` diretamente no processo pai. Se feito via `fork`, o filho mudaria de diretório mas o pai (o shell) permaneceria no mesmo lugar — por isso `cd` nunca passa pelo executor.

### Fase 5 - Execução (`executor.c`)

`execute_command()` recebe o `args[]` e:
1. Chama `fork()` para criar um processo filho.
2. No filho: chama `execvp(args[0], args)`, que localiza o programa no PATH e substitui a imagem do processo. Se `execvp` falhar, o filho chama `handle_exec_error()` e encerra com `EXIT_FAILURE`.
3. No pai: chama `waitpid()` para aguardar o término do filho e recuperar o controle do terminal.

### Fase 6 - Tratamento de sinais (`signals.c`)

Quando um processo filho termina, o kernel envia `SIGCHLD` ao pai. O `sigchld_handler()` é invocado assincronamente e executa um loop de `waitpid(-1, NULL, WNOHANG)` para colher todos os filhos já terminados sem bloquear o shell. O flag `SA_RESTART` garante que chamadas de sistema em andamento (como `fgets`) não sejam interrompidas pelo sinal.

---

## Visão geral dos arquivos

| Arquivo | Responsabilidade |
|---|---|
| `mysh.h` | Cabeçalho central: `#includes`, constantes (`MAX_INPUT`, `MAX_ARGS`) e assinaturas de todas as funções. Todo `.c` inclui apenas este arquivo. |
| `main.c` | Loop principal: inicializa sinais, exibe prompt, lê entrada, trata built-ins (`exit`, `cd`) e delega execução. |
| `parser.c` | `show_prompt()` — monta e exibe o prompt dinâmico com cores ANSI. `parse_input()` — tokeniza a linha de entrada em `args[]` via `strtok`. |
| `executor.c` | `execute_command()` — realiza `fork` + `execvp` + `waitpid`. Delega erros de execução para `signals.c`. |
| `signals.c` | `setup_signals()` — registra handlers via `sigaction`. `sigchld_handler()` — previne processos zumbi. `handle_exec_error()` — converte `errno` em mensagem legível via `strerror`. |
| `Makefile` | Compila todos os módulos com `gcc -Wall -Wextra -g -std=gnu11` e gera o executável `mysh`. |

---

## Divisão de responsabilidades

| Integrante | Módulos | Responsabilidades |
|---|---|---|
| Integrante 1 | `main.c`, `parser.c` | Loop principal, prompt dinâmico, parsing, built-ins (`exit`, `cd`) |
| Integrante 2 | `executor.c` | `fork`, `execvp`, `waitpid`, tratamento de falha no `fork` |
| Integrante 3 | `signals.c` | `sigaction`, prevenção de zumbis, `strerror`, limpeza de memória |
