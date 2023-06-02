/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_SCRIPT_SCOPE_H
#define SWAMP_SCRIPT_SCOPE_H

#include <stdbool.h>
#include <swamp-runtime/context.h>
#include <swamp-runtime/swamp.h>
#include <swamp-runtime/swamp_unpack.h>
#include <swamp-runtime/types.h>
#include <swamp-script-scope/script_static.h>
#include <swamp-state/state.h>
#include <swamp-typeinfo/typeinfo.h>

struct SwtiChunk;
struct SwtiType;

struct ImprintAllocator;

void swampScriptStateDebugOutput(const SwampState* state, const char* description);

typedef struct SwampScriptExecuteInfo {
    size_t parameterOctetCount;
    size_t parameterCount;
    size_t expectedReturnOctetSize;
} SwampScriptExecuteInfo;

typedef struct SwampScriptScope {
    struct SwampMachineContext machineContext;
    SwampDynamicMemory randomAccessMemory[2];
    SwampUnmanagedMemory unmanagedMemory[2];
    size_t activeRandomAccessMemorySlot;
    SwampState lastState;
    int verboseFlag;
    int shouldBeCompacted;
    const SwampScriptStatic* staticCode;
    size_t ramMemorySize;
} SwampScriptScope;

const SwampState* swampScriptScopeExecute(SwampScriptScope* self, const SwampScriptExecuteInfo* parameters);
void swampScriptScopeInit(SwampScriptScope* self, const SwampScriptStatic* scriptStatic, void* userData,
                          size_t ramMemorySize, struct ImprintAllocator* allocator);
void swampScriptScopeDestroy(SwampScriptScope* self);
void swampScriptScopeClear(SwampScriptScope* self);
int swampScriptScopeClone(SwampScriptScope* target, const SwampScriptScope* source, struct ImprintAllocator* allocator);
SwampDynamicMemory* swampScriptScopeUsedRandomAccessMemory(const SwampScriptScope* self);
SwampUnmanagedMemory* swampScriptScopeUsedUnmanagedMemory(const SwampScriptScope* self);
SwampUnmanagedMemory* swampScriptScopeNextUnmanagedMemory(const SwampScriptScope* self);
const char** swampScriptScopeResourceNames(const SwampScriptScope* self, int* maxCount);
const SwampFunc* swampScriptScopeFindFunction(const SwampScriptScope* self, const char* functionName);

#endif
