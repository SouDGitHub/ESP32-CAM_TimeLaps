#pragma once
#include "FS.h"
#include "SD_MMC.h"

extern bool writeFile(const char *path, const unsigned char *data, unsigned long len);
extern bool appendFile(const char *path, const unsigned char *data, unsigned long len);
extern bool initFileSystem();
extern bool createDir(const char *path);
extern bool fileExists(const char *path);