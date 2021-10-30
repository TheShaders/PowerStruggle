#ifndef __FILES_H__
#define __FILES_H__

#include <cstdint>
#include <cstddef>

#include <types.h>

void *load_file(const char *path);
Model *load_model(const char *path);

#endif
