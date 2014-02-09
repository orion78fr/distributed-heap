# Compilateur et flags
CC = gcc
CFLAGS = -W -Wall -ansi -pedantic -Iinclude 
LFLAGS = -lrt -pthread

# Variables pour créer le tar correspondant au dossier de TME POSIX
TARNAME = ../PSAR.tgz
TARFILES = README makefile bin include lib obj src

# Chemin des dossiers
BIN = ./bin/
OBJ = ./obj/
INCLUDE = ./include/
LIB = ./lib/
SRC = ./src/heapServer/

# Objets de chaque programme
SERVER_OBJ = $(OBJ)main.o

# Permet d'afficher une ligne en bleu
echoblue = @echo "\033[01;34m$(1)\033[00m"

all: clean server

clean:
	$(call echoblue,"Suppression des fichiers .o et des binaires")
	@rm -rf $(OBJ)*.o $(BIN)*

ultraclean: clean
	$(call echoblue,"Suppression des fichiers temporaires")
	@rm -rf */*~ *~

tar: ultraclean
	$(call echoblue,"Création du tar")
	@rm -rf $(TARNAME)
	@tar -zcf $(TARNAME) $(TARFILES)

exo1: exo1serv exo1cli

server: $(SERVER_OBJ)
	$(call echoblue,"Link de $^ en $@")
	@$(CC) -o $(BIN)$@ $^ $(LFLAGS)

$(OBJ)%.o: $(SRC)%.c
	$(call echoblue,"Compilation de $< en $@")
	@$(CC) -o $@ -c $< $(CFLAGS)


