OBJ = FlashProbe.o Flash.o FlashWorker.o
TARGET = FlashProbe.OS3KApp

include ../../Makefile.common

FLASH_DIR = $(TOP)/applets/FileManager
$(OBJ): $(FLASH_DIR)/Flash.h Makefile

Flash.o: $(FLASH_DIR)/Flash.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -DFILE_MANAGER_FLASH_PROBE -c $< -o $@

FlashWorker.o: $(FLASH_DIR)/FlashWorker.S
	$(CC) -m68000 -c $< -o $@
