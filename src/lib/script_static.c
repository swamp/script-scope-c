/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <swamp-dump/dump_ascii.h>
#include <swamp-runtime/debug.h>
#include <swamp-runtime/fixup.h>
#include <swamp-script-scope/script_static.h>

void swampScriptStaticSetCode(SwampScriptStatic* self, const uint8_t* buffer, size_t octetCount, const char* debugName)
{
    const int verboseFlag = 0;
    swampUnpackInit(&self->unpack, verboseFlag);

    SwampOctetStream stream;
    stream.octets = buffer;
    stream.octetCount = octetCount;
    stream.position = 0;

    CLOG_INFO("unpack script file %s", debugName)
    int unpackErr = swampUnpackSwampOctetStream(&self->unpack, &stream, self->bindFn, verboseFlag, self->allocator);
    if (unpackErr < 0) {
        CLOG_ERROR("could not load '%s'", debugName)
    }
    self->updateFn = swampUnpackEntryPoint(&self->unpack);
    if (!self->updateFn) {
        CLOG_ERROR("couldn't find entry point main() in '%s'", debugName)
    }

    swampStaticMemoryInit(&self->constantStaticMemory, self->unpack.constantStaticMemoryOctets,
                          self->unpack.constantStaticMemoryMaxSize);

    const SwampDebugInfoFiles* debugInfoLines = swampLedgerGetDebugInfoFiles(&self->unpack.ledger);
    if (!debugInfoLines) {
        CLOG_ERROR("must have debug info")
    }
    self->debugInfoFiles = debugInfoLines;

    const SwtiType* _functionType = swtiChunkTypeFromIndex(&self->unpack.typeInfoChunk, self->updateFn->typeIndex);
    if (_functionType->type != SwtiTypeFunction) {
        CLOG_ERROR("must have function type")
    }
    self->functionType = (const SwtiFunctionType*) _functionType;
    self->returnType = self->functionType->parameterTypes[self->functionType->parameterCount - 1];
}

const SwtiChunk* swampScriptStaticTypeInfo(const SwampScriptStatic* self)
{
    return &self->unpack.typeInfoChunk;
}

bool swampScriptStaticIsReady(const SwampScriptStatic* self)
{
    return self->updateFn != 0;
}

const SwampFunc* swampScriptStaticFindFunction(const SwampScriptStatic* self, const char* functionName)
{
    const SwampLedger* ledger = &self->unpack.ledger;

    return swampLedgerFindFunction(ledger, functionName);
}

void swampScriptStaticInit(SwampScriptStatic* self, SwampResolveExternalFunction bindFn,
                           struct ImprintAllocator* allocator)
{
    self->updateFn = 0;
    self->bindFn = bindFn;
    CLOG_DEBUG("swampScriptStatic: Initiating script static (%p)", (const void*) self);
    self->allocator = allocator;
}

void swampScriptStaticDestroy(SwampScriptStatic* self)
{
    swampUnpackFree(&self->unpack);
}

const char** swampScriptStaticResourceNames(const SwampScriptStatic* self, int* maxCount)
{
    *maxCount = 0;

    const SwampResourceNameChunkEntry* entry = swampLedgerFindResourceNames(&self->unpack.ledger);

    *maxCount = entry->resourceCount;

    return entry->resourceNames;
}
