# Compilateur et flags
CC = gcc
FLAGS = -W -Wall -lrt -DDEBUG -g

# Chemin des dossiers
BIN = ./bin/
OBJ = ./obj/
LIB = ./lib/

SRCSERVER = ./src/heapServer/
SRCLIBRARY = ./src/heapLibrary/
SRCEXAMPLE1 = ./src/examples/
SRCEXAMPLE2 = ./src/examples/

LIBRARY_NAME = heapLibrary.a
SERVER_NAME = server

# Permet d'afficher une ligne en bleu
echoblue = @echo "\033[01;34m$(1)\033[00m"

all: ultraclean server library example1

clean:
	$(call echoblue, "Suppression des fichiers librairies objets et binaires")
	@rm -rf $(OBJ)* $(BIN)* $(LIB)*

ultraclean: clean
	$(call echoblue, "Suppression des fichiers temporaires")
	@rm -rf */*~ *~

server:
	$(call echoblue, "Compilation du serveur")
	@$(CC) -o $(BIN)$(SERVER_NAME) $(SRCSERVER)*.c $(FLAGS)

library:
	$(call echoblue, "Compilation de la librairie")
	@$(CC) -o $(OBJ)$@.o -c $(SRCLIBRARY)*.c $(FLAGS)
	@ar -cvq $(LIB)$(LIBRARY_NAME) $(OBJ)$@.o

example1: library
	$(call echoblue, "Compilation de l'exemple 1")
	@$(CC) -o $(BIN)$@ $(SRCEXAMPLE1)example1.c $(LIB)$(LIBRARY_NAME) $(FLAGS)

example2: library
	$(call echoblue, "Compilation de l'exemple 2")
	@$(CC) -o $(BIN)$@ $(SRCEXAMPLE2)example2.c $(LIB)$(LIBRARY_NAME) $(FLAGS)

.PHONY: server clean ultraclean library example1 example2
