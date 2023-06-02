/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_SCRIPT_STATIC_H
#define SWAMP_SCRIPT_STATIC_H

#include <stdbool.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_unpack.h>
#include <swamp-runtime/types.h>
#include <swamp-typeinfo/typeinfo.h>

struct SwtiChunk;
struct SwtiType;
struct ImprintAllocator;

typedef struct SwampScriptStatic {
    const struct SwampFunc* updateFn;
    SwampStaticMemory constantStaticMemory;
    SwampUnpack unpack;
    SwampResolveExternalFunction bindFn;
    const char* debugName;
    int verboseFlag;
    const SwtiFunctionType* functionType;
    const SwtiType* returnType;
    const struct SwampDebugInfoFiles* debugInfoFiles;
    struct ImprintAllocator* allocator;
} SwampScriptStatic;

bool swampScriptStaticIsReady(const SwampScriptStatic* self);
void swampScriptStaticInit(SwampScriptStatic* self, SwampResolveExternalFunction bindFn,
                           struct ImprintAllocator* allocator);
void swampScriptStaticDestroy(SwampScriptStatic* self);
void swampScriptStaticReload(SwampScriptStatic* self);
void swampScriptStaticSetCode(SwampScriptStatic* self, const uint8_t* buffer, size_t octetCount, const char* debugName);
const struct SwtiChunk* swampScriptStaticTypeInfo(const SwampScriptStatic* self);
const char** swampScriptStaticResourceNames(const SwampScriptStatic* self, int* maxCount);
const SwampFunc* swampScriptStaticFindFunction(const SwampScriptStatic* self, const char* functionName);

#endif
