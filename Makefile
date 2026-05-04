# ---------------------------------------------------------------------
# Makefile - MySh (Mini Shell)
#
# Uso:
#   make          -> compila tudo e gera o executável ./mysh
#   make clean    -> remove objetos e o executável
#   make rebuild  -> clean + compilação completa
#
# Durante o desenvolvimento paralelo, cada integrante pode compilar
# apenas seu módulo para checar erros de sintaxe:
#   make signals.o    (Integrante 3)
#   make executor.o   (Integrante 2)
#   make parser.o     (Integrante 1)
# ---------------------------------------------------------------------

# Nome do executável final
TARGET = mysh

# Compilador e flags
CC = gcc

# -Wall      -> ativa a maioria dos warnings úteis
# -Wextra    -> warnings adicionais (parâmetros não usados, etc.)
# -g         -> inclui informações de debug (útil com gdb/valgrind)
# -std=gnu11 -> padrão C11 com extensões GNU/POSIX habilitadas.
#               Necessário para sigaction, sigemptyset, SA_RESTART e
#               outras chamadas de sistema POSIX usadas no projeto.
#               (for loop declarations e demais features do C11 continuam
#               disponíveis normalmente)
CFLAGS = -Wall -Wextra -g -std=gnu11

# Arquivos-fonte (.c) que compõem o projeto
# Cada integrante adiciona o seu módulo aqui conforme avança.
SRCS = main.c \
       signals.c

# Geração automática dos nomes de objetos (.o) a partir dos .c
OBJS = $(SRCS:.c=.o)
 
# --- Regra padrão: link final ----------------------------------------
# $(CC) recebe todos os .o e gera o executável $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "Compilação concluída --> ./$(TARGET)"

# --- Regra genérica: compila qualquer .c em .o -----------------------
# $< = o arquivo .c (pré-requisito)
# $@ = o arquivo .o (alvo)
# mysh.h é listado como dependência: qualquer mudança no header
# força recompilação de todos os módulos.
%.o: %.c mysh.h
	$(CC) $(CFLAGS) -c $< -o $@

# --- Limpeza ---------------------------------------------------------
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Arquivos temporários removidos."

# --- Rebuild completo ------------------------------------------------
rebuild: clean $(TARGET)

# Declara alvos que não correspondem a arquivos reais
.PHONY: clean rebuild
