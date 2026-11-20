OBJ = FileManager.o Flash.o FlashWorker.o
TARGET = FileManager.OS3KApp

include ../../Makefile.common

$(OBJ): Flash.h Makefile

FlashWorker.o: FlashWorker.S
	$(CC) -m68000 -c $< -o $@
