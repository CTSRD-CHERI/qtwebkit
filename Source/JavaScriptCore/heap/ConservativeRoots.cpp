/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ConservativeRoots.h"

#include "CodeBlock.h"
#include "CodeBlockSet.h"
#include "CopiedSpace.h"
#include "CopiedSpaceInlines.h"
#include "HeapInlines.h"
#include "JSCell.h"
#include "JSObject.h"
#include "JSCInlines.h"
#include "Structure.h"
#include <wtf/OSAllocator.h>

namespace JSC {

ConservativeRoots::ConservativeRoots(MarkedBlockSet* blocks, CopiedSpace* copiedSpace)
    : m_roots(m_inlineRoots)
    , m_size(0)
    , m_capacity(inlineCapacity)
    , m_blocks(blocks)
    , m_copiedSpace(copiedSpace)
{
}

ConservativeRoots::~ConservativeRoots()
{
    if (m_roots != m_inlineRoots)
        OSAllocator::decommitAndRelease(m_roots, m_capacity * sizeof(JSCell*));
}

void ConservativeRoots::grow()
{
    size_t newCapacity = m_capacity == inlineCapacity ? nonInlineCapacity : m_capacity * 2;
    JSCell** newRoots = static_cast<JSCell**>(OSAllocator::reserveAndCommit(newCapacity * sizeof(JSCell*)));
    memcpy(newRoots, m_roots, m_size * sizeof(JSCell*));
    if (m_roots != m_inlineRoots)
        OSAllocator::decommitAndRelease(m_roots, m_capacity * sizeof(JSCell*));
    m_capacity = newCapacity;
    m_roots = newRoots;
}

template<typename MarkHook>
inline void ConservativeRoots::genericAddPointer(void* p, TinyBloomFilter filter, MarkHook& markHook)
{
#ifdef __CHERI_PURE_CAPABILITY__
    // Check if this is a valid capability
    if (!__builtin_cheri_tag_get(p))
        return;
#endif
    markHook.mark(p);

    m_copiedSpace->pinIfNecessary(p);

    if (!Heap::isPointerGCObject(filter, *m_blocks, p))
        return;

    if (m_size == m_capacity)
        grow();

    m_roots[m_size++] = static_cast<JSCell*>(p);
}

template<typename MarkHook>
SUPPRESS_ASAN
void ConservativeRoots::genericAddSpan(void* begin, void* end, MarkHook& markHook)
{
#ifdef __CHERI_PURE_CAPABILITY__
    if ((vaddr_t)begin > (vaddr_t)end) { //XXXKG: end may not be a capability hence use address_get, otherwise we will get a tag violation
#else
    if (begin > end) {
#endif
        void* swapTemp = begin;
        begin = end;
        end = swapTemp;
    }

    RELEASE_ASSERT(isPointerAligned(begin));
    RELEASE_ASSERT(isPointerAligned(end));

    TinyBloomFilter filter = m_blocks->filter(); // Make a local copy of filter to show the compiler it won't alias, and can be register-allocated.
    for (char** it = static_cast<char**>(begin);
#ifdef __CHERI_PURE_CAPABILITY__
        (vaddr_t)it != (vaddr_t)static_cast<char**>(end);
#else
        it != static_cast<char**>(end);
#endif
        ++it) {
        genericAddPointer(*it, filter, markHook);
    }
}

class DummyMarkHook {
public:
    void mark(void*) { }
};

void ConservativeRoots::add(void* begin, void* end)
{
    DummyMarkHook dummy;
    genericAddSpan(begin, end, dummy);
}

void ConservativeRoots::add(void* begin, void* end, JITStubRoutineSet& jitStubRoutines)
{
    genericAddSpan(begin, end, jitStubRoutines);
}

class CompositeMarkHook {
public:
    CompositeMarkHook(JITStubRoutineSet& stubRoutines, CodeBlockSet& codeBlocks, const LockHolder& locker)
        : m_stubRoutines(stubRoutines)
        , m_codeBlocks(codeBlocks)
        , m_codeBlocksLocker(locker)
    {
    }
    
    void mark(void* address)
    {
        m_stubRoutines.mark(address);
        m_codeBlocks.mark(m_codeBlocksLocker, address);
    }

private:
    JITStubRoutineSet& m_stubRoutines;
    CodeBlockSet& m_codeBlocks;
    const LockHolder& m_codeBlocksLocker;
};

void ConservativeRoots::add(
    void* begin, void* end, JITStubRoutineSet& jitStubRoutines, CodeBlockSet& codeBlocks)
{
    LockHolder locker(codeBlocks.getLock());
    CompositeMarkHook markHook(jitStubRoutines, codeBlocks, locker);
    genericAddSpan(begin, end, markHook);
}

} // namespace JSC
