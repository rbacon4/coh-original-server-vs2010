#pragma once

#include <cstdio>


FILE *fopen_nofail(const char *pFileName, const char *pModes);

bool FileExists(const char *pFileName);