# Sistemas-Operacionais-2
Projeto 1 Sistemas Operacionais 2


📑 Documentação Completa do Projeto:
Mini Processador de Comandos (MySh)
1. Definição do Projeto
O objetivo é a implementação de um Mini Processador de Comandos (Shell) em C para
ambiente Linux. O software atuará como um intermediário entre o usuário e o Kernel,
gerenciando o ciclo de vida de processos e interpretando comandos de sistema.
● Data de Referência: 24/04/2026.
● Prazo de Entrega: 08/05/2026.

2. Requisitos Técnicos e Funcionais
De acordo com as especificações do exercício:
● Prompt Interativo: Exibir um prompt e aguardar a entrada do usuário
continuamente.
● Comando Interno exit: Deve encerrar o processador de comandos imediatamente.
● Execução via PATH: Quando um nome de programa é digitado, a shell deve
localizá-lo automaticamente no PATH do sistema.
● Execução via Caminho: Suportar caminhos absolutos (ex: /bin/ls) ou relativos (ex:
./meu_prog).
● Gestão de Argumentos: Capturar e passar todos os argumentos digitados na linha
de comando para o programa chamado.
● Tratamento de Erros: Se o programa não existir ou o caminho for inválido, exibir
mensagem de erro apropriada.
● Independência: Não usar a chamada system, pois a shell não deve depender de
outra shell pré-existente.
● Robustez de Processos: Garantir que a shell não deixe processos filhos
bloqueados ou em estado "zumbi".

3. Arquitetura do Sistema e Divisão de Tarefas
Para o trabalho em grupo de 3 pessoas via Git, a estrutura recomendada é a seguinte:
👤 Integrante 1: Interface, Loop e Parsing (Entrada)
● Responsabilidades:
○ Implementar o main loop que mantém a shell viva.
○ Desenvolver o prompt dinâmico usando getcwd e gethostname.
○ Capturar a entrada bruta com fgets.
○ Parsing: Utilizar strtok para quebrar a string em um array de ponteiros (char
*args[]), essencial para o execvp.
○ Validar se a entrada é apenas um Enter ou o comando exit.

👤 Integrante 2: Gerenciamento de Processos (Execução)
● Responsabilidades:
○ Implementar a lógica de fork() para criar o processo filho.
○ No processo filho, utilizar execvp(args[0], args) para substituir a imagem do
processo pelo programa desejado.
○ No processo pai, gerenciar a espera pelo término do filho usando wait ou
waitpid.
○ Garantir que a shell recupere o controle após a execução do comando.

👤 Integrante 3: Sinais, Erros e Estabilidade (Sistema)
● Responsabilidades:
○ Tratador de Sinais: Configurar o sigaction para tratar o sinal SIGCHLD,
limpando processos que terminam em background e prevenindo zumbis.
○ Gestão de Erros: Utilizar strerror(errno) para imprimir a causa real de falha
caso o execvp retorne erro (ex: "No such file or directory").
○ Memória: Garantir que os buffers de string sejam limpos e que não ocorram
vazamentos de memória (Memory Leaks).

4. Lista de Funções Mandatórias (Toolkit)
As seguintes funções da biblioteca padrão C e chamadas de sistema devem ser utilizadas:
Função Finalidade

fork Criar um novo processo (filho).

execvp Executar um programa (procura no PATH automaticamente).

wait Sincronizar o pai com o término do filho.

sigaction Tratar sinais (essencial para evitar zumbis).

getcwd Obter o diretório atual para o prompt.

gethostname Obter o nome da máquina para o prompt.

strtok Dividir a string de comando em argumentos individuais.

fgets Ler a linha digitada pelo usuário com segurança.

strerror Converter códigos de erro em mensagens legíveis.

5. Plano de Integração (Git)
1. Criação do Repositório: Criar um arquivo mysh.h com as assinaturas das funções.
2. Desenvolvimento Paralelo: Cada integrante trabalha em uma branch (parser,
executor, signals).
3. Merge Final: Unir os módulos no arquivo mysh.c principal.
4. Teste de Estresse: Executar comandos inexistentes, comandos com muitos
espaços e comandos com múltiplos argumentos (ex: ls -l /tmp)

📂 Estrutura de Arquivos Sugerida
● mysh.h: Contém as assinaturas de todas as funções e as bibliotecas necessárias
(stdio.h, unistd.h, sys/wait.h, etc.).
● main.c: O loop principal da shell (Integrante 1).
● parser.c: A lógica de quebrar a string de comando (Integrante 1).
● executor.c: A lógica do fork e execvp (Integrante 2).
● signals.c: O tratamento de sinais SIGCHLD e erros (Integrante 3).

📅 Cronograma Sugerido
Fase Prazo Objetivo

Fase 1: Setup Até 27/04 Criar repositório e definir as assinaturas das

funções no .h.

Fase 2:
Desenvolvimento

Até 02/05 Cada integrante termina sua lógica principal em

sua branch.

Fase 3: Integração 03/05 a
05/05

Juntar as branches no main e resolver conflitos.

Fase 4: Testes &
Ajustes

06/05 a
08/05

Testar comandos com espaços, argumentos
longos e verificar se há processos zumbis.
