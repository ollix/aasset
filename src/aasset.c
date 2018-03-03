// Copyright (c) 2017 Ollix. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// ---
// Author: olliwang@ollix.com (Olli Wang)

#include "aasset.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <android/asset_manager.h>

#define _BSD_SOURCE

// Caches the states of a asset file.
struct AAssetFile {
  AAsset* asset;
  FILE* stream;
  off_t pos;
  struct AAssetFile* next;
};
typedef struct AAssetFile AAssetFile;

// The weak reference to the `AAssetManager` object from Android.
AAssetManager* android_asset_manager = NULL;

// The weak reference pointing to the first managed `AAssetFile` object.
AAssetFile* managed_aasset_files = NULL;

// Private Functions.

// Allocates a new `AAssetFile` object and prepends it to the list of
// `managed_aasset_files`.
AAssetFile* aasset_alloc_file() {
  AAssetFile* aasset_file = (AAssetFile*)malloc(sizeof(AAssetFile));
  if (aasset_file == NULL)
    return NULL;

  aasset_file->asset = NULL;
  aasset_file->pos = 0;
  aasset_file->next = NULL;

  if (managed_aasset_files == NULL) {
    managed_aasset_files = aasset_file;
  } else {
    aasset_file->next = managed_aasset_files;
    managed_aasset_files = aasset_file;
  }
  return aasset_file;
}

// Returns the pointer to the `AAssetFile` object, which has the matched
// `stream`. Returns `NULL` on failure.
AAssetFile* aasset_get_file(FILE* stream) {
  if (managed_aasset_files == NULL)
    return NULL;

  AAssetFile* prev = NULL;
  AAssetFile* iterator = managed_aasset_files;
  AAssetFile* aasset_file = NULL;
  while (iterator) {
    if (iterator->stream == stream) {
      aasset_file = iterator;
      break;
    }
    prev = iterator;
    iterator = iterator->next;
  }

  if (aasset_file == NULL) {
    return NULL;
  }

  // Moves the aasset file to the beginning of the list.
  if (prev != NULL) {
    prev->next = aasset_file->next;
    aasset_file->next = managed_aasset_files;
    managed_aasset_files = aasset_file;
  }
  return aasset_file;
}

// Standard Functions.

int aasset_init(AAssetManager* manager) {
  if (manager == NULL) {
    return 1;
  }
  android_asset_manager = manager;
  return 0;
}

int aasset_fread(void* ptr, size_t size, size_t count, FILE* stream) {
  AAssetFile* aasset_file = aasset_get_file(stream);
  if (aasset_file == NULL) {
    return 0;
  }
  int bytes = AAsset_read(aasset_file->asset, ptr, size * count);
  aasset_file->pos += bytes;
  return bytes;
}

int aasset_fseek(FILE* stream, long int offset, int origin) {
  AAssetFile* aasset_file = aasset_get_file(stream);
  if (aasset_file == NULL) {
    return 1;
  }
  aasset_file->pos = AAsset_seek(aasset_file->asset, offset, origin);
  return 0;
}

int aasset_ftell(FILE* stream) {
  AAssetFile* aasset_file = aasset_get_file(stream);
  if (aasset_file == NULL) {
    return EOF;
  }
  return aasset_file->pos;
}

int aasset_fclose(FILE* stream) {
  if (managed_aasset_files == NULL)
    return 1;

  AAssetFile* prev = NULL;
  AAssetFile* iterator = managed_aasset_files;
  AAssetFile* aasset_file = NULL;
  while (iterator) {
    if (iterator->stream == stream) {
      aasset_file = iterator;
      break;
    }
    prev = iterator;
    iterator = iterator->next;
  }

  if (aasset_file == NULL)
    return 1;

  if (prev == NULL) {
    managed_aasset_files = aasset_file->next;
  } else {
    prev->next = aasset_file->next;
  }
  AAsset_close(aasset_file->asset);
  return 0;
}

static int android_close(void* cookie) {
  AAsset* asset = (AAsset*)cookie;
  if (managed_aasset_files != NULL) {
    AAssetFile* prev = NULL;
    AAssetFile* iterator = managed_aasset_files;
    AAssetFile* aasset_file = NULL;
    while (iterator) {
      if (iterator->asset == asset) {
        aasset_file = iterator;
        break;
      }
      prev = iterator;
      iterator = iterator->next;
    }

    if (aasset_file == NULL)
      return 1;

    if (prev == NULL) {
      managed_aasset_files = aasset_file->next;
    } else {
      prev->next = aasset_file->next;
    }
  }
  AAsset_close(asset);
  return 0;
}

static int android_read(void* cookie, char* buf, int size) {
  return AAsset_read((AAsset*)cookie, buf, size);
}

static fpos_t android_seek(void* cookie, fpos_t offset, int whence) {
  return AAsset_seek((AAsset*)cookie, offset, whence);
}

static int android_write(void* cookie, const char* buf, int size) {
  return -1;
}

FILE* aasset_fopen(const char* fname, const char* mode) {
  if (mode[0] == 'w') return NULL;

  // An Android asset path should never begin with `/`.
  if (fname[0] == '/') fname = &fname[1];

  AAsset* asset = AAssetManager_open(android_asset_manager, fname, 0);
  if (asset == NULL) {
    return NULL;
  }

  FILE* stream = (FILE*)funopen(asset, android_read, android_write,
                                android_seek, android_close);
  if (stream == NULL) {
    AAsset_close(asset);
    return NULL;
  }
  AAssetFile* aasset_file = aasset_alloc_file();
  aasset_file->stream = stream;
  aasset_file->asset = asset;
  aasset_file->pos = 0;
  return stream;
}

// Extension Functions.

int aasset_fsize(FILE* stream) {
  AAssetFile* aasset_file = aasset_get_file(stream);
  if (aasset_file == NULL) {
    return 1;
  }

  const int kPosition = aasset_file->pos;
  const int kFileSize = AAsset_seek(aasset_file->asset, 0, SEEK_END);
  AAsset_seek(aasset_file->asset, kPosition, SEEK_SET);
  return kFileSize;
}
