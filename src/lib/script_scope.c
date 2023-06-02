/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <imprint/allocator.h>
#include <monotonic-time/monotonic_time.h>
#include <swamp-dump/dump_ascii.h>
#include <swamp-runtime/clone.h>
#include <swamp-runtime/compact.h>
#include <swamp-runtime/debug.h>
#include <swamp-runtime/fixup.h>
#include <swamp-runtime/swamp.h>
#include <swamp-script-scope/script_scope.h>

void swampScriptStateDebugOutput(const SwampState* state, const char* description)
{
    if (state->debugType == 0) {
        CLOG_ERROR("must have type!!")
    }
#if 1
#define TEMP_STR_SIZE (16 * 1024)
    static char tempStr[TEMP_STR_SIZE];
    CLOG_INFO("state '%s': %s", description,
              swampDumpToAsciiString(state->state, state->debugType, 0, tempStr, TEMP_STR_SIZE))
#endif
}

const SwampState* swampScriptScopeExecute(SwampScriptScope* self, const SwampScriptExecuteInfo* info)
{
    if (info->expectedReturnOctetSize != self->staticCode->updateFn->returnOctetSize) {
        CLOG_ERROR("swampScriptScopeExecute: wrong octet return size. expected %zu but got %zu",
                   info->expectedReturnOctetSize, self->staticCode->updateFn->returnOctetSize)
        return 0;
    }
    if (info->parameterOctetCount != self->staticCode->updateFn->parametersOctetSize) {
        CLOG_ERROR("swampScriptScopeExecute: '%s' '%s' wrong parameter octet size, encountered %zu, but requires %zu",
                   self->staticCode->debugName, self->staticCode->updateFn->debugName, info->parameterOctetCount,
                   self->staticCode->updateFn->parametersOctetSize)
        return 0;
    }

    SwampResult result;
    result.expectedOctetSize = info->expectedReturnOctetSize;

    SwampParameters parameters;
    parameters.parameterCount = info->parameterCount;
    parameters.octetSize = info->parameterOctetCount;

    SwampDynamicMemory* beforeDynamicMemory = &self->randomAccessMemory[self->activeRandomAccessMemorySlot];
    SwampUnmanagedMemory* beforeUnmanagedMemory = &self->unmanagedMemory[self->activeRandomAccessMemorySlot];
    self->machineContext.dynamicMemory = beforeDynamicMemory;
    self->machineContext.unmanagedMemory = beforeUnmanagedMemory;

    int runErr = swampRun(&result, &self->machineContext, self->staticCode->updateFn, parameters, self->verboseFlag);
    if (runErr < 0) {
        CLOG_ERROR("run error %d", runErr);
        return 0;
    }

    SwampState returnState;
    returnState.debugType = self->staticCode->returnType;
    returnState.state = self->machineContext.bp;
    returnState.stateOctetCount = self->staticCode->updateFn->returnOctetSize;
    returnState.align = self->staticCode->updateFn->returnAlign;

    // swampScriptStateDebugOutput(&returnState, "return state before reset");

    self->activeRandomAccessMemorySlot = !self->activeRandomAccessMemorySlot;
    SwampDynamicMemory* nextDynamicMemory = &self->randomAccessMemory[self->activeRandomAccessMemorySlot];
    SwampUnmanagedMemory* nextUnmanagedMemory = &self->unmanagedMemory[self->activeRandomAccessMemorySlot];

    void* compactedState;

    // CLOG_INFO("clearing slot %dww", self->activeRandomAccessMemorySlot);
    // swampScriptStateDebugOutput(&returnState, "return state after reset");
    if (self->shouldBeCompacted) {
        // CLOG_EXECUTE(MonotonicTimeNanoseconds before = monotonicTimeNanosecondsNow();)
        swampCompact(self->machineContext.bp, self->staticCode->returnType, nextDynamicMemory, nextUnmanagedMemory,
                     beforeUnmanagedMemory, &compactedState);
        // CLOG_EXECUTE(MonotonicTimeNanoseconds after = monotonicTimeNanosecondsNow();)

    } else {
        compactedState = self->machineContext.bp;
    }

    swampDynamicMemoryReset(beforeDynamicMemory);
    swampUnmanagedMemoryReset(beforeUnmanagedMemory);

    self->lastState.state = compactedState;
    self->lastState.stateOctetCount = info->expectedReturnOctetSize;
    self->lastState.debugType = self->staticCode->returnType;

    self->machineContext.dynamicMemory = nextDynamicMemory;
    self->machineContext.unmanagedMemory = nextUnmanagedMemory;

#if 0
    swampScriptStateDebugOutput(&self->lastState, "swampScriptScope");
#endif

    return &self->lastState;
}

SwampDynamicMemory* swampScriptScopeUsedRandomAccessMemory(const SwampScriptScope* self)
{
    return (SwampDynamicMemory*) &self->randomAccessMemory[self->activeRandomAccessMemorySlot];
}

SwampUnmanagedMemory* swampScriptScopeUsedUnmanagedMemory(const SwampScriptScope* self)
{
    return (SwampUnmanagedMemory*) &self->unmanagedMemory[self->activeRandomAccessMemorySlot];
}

SwampUnmanagedMemory* swampScriptScopeNextUnmanagedMemory(const SwampScriptScope* self)
{
    return (SwampUnmanagedMemory*) &self->unmanagedMemory[!self->activeRandomAccessMemorySlot];
}



void swampScriptScopeInit(SwampScriptScope* self, const SwampScriptStatic* scriptStatic, void* userData,
                          size_t ramMemorySize, ImprintAllocator* allocator)
{
    if (!swampScriptStaticIsReady(scriptStatic)) {
        CLOG_ERROR("Must be loaded and ready");
    }
    self->staticCode = scriptStatic;
    self->shouldBeCompacted = 1;
    self->lastState.debugType = 0;
    self->lastState.state = 0;
    self->lastState.align = 0;
    self->machineContext.userData = userData;
    self->ramMemorySize = ramMemorySize;

    swampDynamicMemoryInitOwnAlloc(&self->randomAccessMemory[0], allocator, ramMemorySize);
    swampDynamicMemoryInitOwnAlloc(&self->randomAccessMemory[1], allocator, ramMemorySize);

    swampUnmanagedMemoryInit(&self->unmanagedMemory[0]);
    swampUnmanagedMemoryInit(&self->unmanagedMemory[1]);
    self->activeRandomAccessMemorySlot = 0;
    swampDynamicMemoryReset(&self->randomAccessMemory[self->activeRandomAccessMemorySlot]);

    swampContextInit(&self->machineContext, &self->randomAccessMemory[self->activeRandomAccessMemorySlot],
                     &self->staticCode->constantStaticMemory, &self->staticCode->unpack.typeInfoChunk,
                     &self->unmanagedMemory[self->activeRandomAccessMemorySlot], self->staticCode->debugInfoFiles,
                     self->staticCode->debugName);
}

void swampScriptScopeDestroy(SwampScriptScope* self)
{
    swampUnmanagedMemoryDestroy(&self->unmanagedMemory[0]);
    swampUnmanagedMemoryDestroy(&self->unmanagedMemory[1]);
    swampDynamicMemoryDestroy(&self->randomAccessMemory[0]);
    swampDynamicMemoryDestroy(&self->randomAccessMemory[1]);
    swampContextDestroy(&self->machineContext);
}

void swampScriptScopeClear(SwampScriptScope* self)
{
    swampUnmanagedMemoryReset(&self->unmanagedMemory[0]);
    swampUnmanagedMemoryReset(&self->unmanagedMemory[1]);
    swampDynamicMemoryReset(&self->randomAccessMemory[0]);
    swampDynamicMemoryReset(&self->randomAccessMemory[1]);
    self->activeRandomAccessMemorySlot = 0;
    self->machineContext.dynamicMemory = &self->randomAccessMemory[self->activeRandomAccessMemorySlot];
    self->machineContext.unmanagedMemory = &self->unmanagedMemory[self->activeRandomAccessMemorySlot];
    self->lastState.state = 0;
    self->lastState.debugType = 0;
    self->lastState.stateOctetCount = 0;
}

int swampScriptScopeClone(SwampScriptScope* target, const SwampScriptScope* source, struct ImprintAllocator* allocator)
{
    swampScriptScopeInit(target, source->staticCode, source->machineContext.userData, source->ramMemorySize, allocator);
    target->shouldBeCompacted = source->shouldBeCompacted;
    target->verboseFlag = source->verboseFlag;
    target->machineContext.debugInfoFiles = source->machineContext.debugInfoFiles;
    target->machineContext.hackIsPredicting = source->machineContext.hackIsPredicting;

    swampScriptStateDebugOutput(&source->lastState, "cloning from");

    SwampDynamicMemory* targetRam = swampScriptScopeUsedRandomAccessMemory(target);
    SwampUnmanagedMemory* targetUnmanagedMemory = swampScriptScopeUsedUnmanagedMemory(target);
    SwampUnmanagedMemory* sourceUnmanagedMemory = swampScriptScopeNextUnmanagedMemory(target);
    void* clonedState;

    int cloneErr = swampClone(source->lastState.state, source->staticCode->returnType, targetRam, targetUnmanagedMemory,
                              sourceUnmanagedMemory, &clonedState);
    if (cloneErr < 0) {
        CLOG_ERROR("couldn't clone")
        return cloneErr;
    }

    target->lastState.state = clonedState;
    target->lastState.debugType = source->staticCode->returnType;
    target->lastState.align = source->lastState.align;
    target->lastState.stateOctetCount = swtiGetMemorySize(source->staticCode->returnType);

    swampScriptStateDebugOutput(&target->lastState, "cloned result");

    return 0;
}

