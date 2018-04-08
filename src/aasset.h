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

#ifndef AASSET_H_
#define AASSET_H_

#include <stdio.h>

#include <android/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initializes aasset manager. Returns 0 if successful.
int aasset_init(AAssetManager* manager);
// Returns the pointer to the AAssetManager object.
AAssetManager* aasset_manager();

// Mimics standard C functions for manipulating files located in Android's
// assets folder.
FILE* aasset_fopen(const char* fname, const char* mode);
int aasset_fread(void* ptr, size_t size, size_t count, FILE* stream);
int aasset_fseek(FILE* stream, long int offset, int origin);
int aasset_ftell(FILE* stream);
int aasset_fclose(FILE* stream);

// Extensions.
int aasset_fsize(FILE* stream);

// Hijacks standard C functions.
#ifdef AASSET_HIJACK_STANDARD_FUNCTIONS
#define fopen(name, mode) aasset_fopen(name, mode)
#define fread(ptr, size, count, stream) aasset_fread(ptr, size, count, stream)
#define fseek(stream, offset, origin) aasset_fseek(stream, offset, origin)
#define ftell(stream) aasset_ftell(stream)
#define fclose(stream) aasset_fclose(stream)
#endif  // AASSET_HIJACK_STANDARD_FUNCTIONS

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AASSET_H_
