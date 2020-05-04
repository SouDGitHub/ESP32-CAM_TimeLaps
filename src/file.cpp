#include "file.h"
#include "FS.h"
#include "SD_MMC.h"
#include "debug.h"

bool writeFile(const char *path, const unsigned char *data, unsigned long len)
{
	if(USE_SERIAL) Serial.printf("Writing file: %s\n", path);
	File file = SD_MMC.open(path, FILE_WRITE);
	if (!file)
	{
		if(USE_SERIAL) Serial.println("Failed to open file for writing");
		return false;
	}
	if (file.write(data, len))
	{
		if(USE_SERIAL) Serial.println("File written");
	}
	else
	{
		if(USE_SERIAL) Serial.println("Write failed");
		return false;
	}
	file.close();
	return true;
}

bool appendFile(const char *path, const unsigned char *data, unsigned long len)
{
	if(USE_SERIAL) Serial.printf("Appending to file: %s\n", path);

	File file = SD_MMC.open(path, FILE_APPEND);
	if (!file)
	{
		if(USE_SERIAL) Serial.println("Failed to open file for writing");
		return false;
	}
	if (file.write(data, len))
	{
		if(USE_SERIAL) Serial.println("File written");
	}
	else
	{
		if(USE_SERIAL) Serial.println("Write failed");
		return false;
	}
	file.close();
	return true;
}


bool initFileSystem()
{
	SD_MMC.end();
	if (!SD_MMC.begin("/sdcard", true))
	{
		if(USE_SERIAL) Serial.println("Card Mount Failed");
		return false;
	}
	uint8_t cardType = SD_MMC.cardType();

	if (cardType == CARD_NONE)
	{
		if(USE_SERIAL) Serial.println("No SD card attached");
		return false;
	}
	if(USE_SERIAL) Serial.print("SD Card Type: ");
	if (cardType == CARD_MMC){
		if(USE_SERIAL) Serial.println("MMC");
	}
	else if (cardType == CARD_SD){
		if(USE_SERIAL) Serial.println("SDSC");
	}
	else if (cardType == CARD_SDHC){
		if(USE_SERIAL) Serial.println("SDHC");
	}
	else
		if(USE_SERIAL) Serial.println("UNKNOWN");

	uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
	if(USE_SERIAL) Serial.printf("SD Card Size: %lluMB\n", cardSize);
	if(USE_SERIAL) Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
	if(USE_SERIAL) Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
	return true;
}

bool createDir(const char *path)
{
	if(USE_SERIAL) Serial.printf("Creating Dir: %s\n", path);
	if (SD_MMC.mkdir(path))
	{
		if(USE_SERIAL) Serial.println("Dir created");
	}
	else
	{
		if(USE_SERIAL) Serial.println("mkdir failed");
		return false;
	}
	return true;
}

bool fileExists(const char *path)
{
	return SD_MMC.exists(path);
}