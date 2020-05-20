/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (c) 2010 University of Szeged
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (c) 2020 Peter S. Blandford-Baker
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
#if USE(PROCESS_COLOCATION_IPC)
#include "SharedMemory.h"

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>
#include <wtf/RandomNumber.h>
#include <wtf/UniStdExtras.h>
#include <wtf/text/CString.h>

namespace WebKit {

SharedMemory::Handle::Handle()
{
}

SharedMemory::Handle::~Handle()
{
}

void SharedMemory::Handle::clear()
{
    m_attachment = IPC::Attachment();
}

bool SharedMemory::Handle::isNull() const
{
    return m_attachment.m_cap() == -1;
}

void SharedMemory::Handle::encode(IPC::ArgumentEncoder& encoder) const
{
    encoder << releaseAttachment();
}

bool SharedMemory::Handle::decode(IPC::ArgumentDecoder& decoder, Handle& handle)
{
    ASSERT_ARG(handle, handle.isNull());

    IPC::Attachment attachment;
    if (!decoder.decode(attachment))
        return false;

    handle.adoptAttachment(WTFMove(attachment));
    return true;
}

IPC::Attachment SharedMemory::Handle::releaseAttachment() const
{
    return WTFMove(m_attachment);
}

void SharedMemory::Handle::adoptAttachment(IPC::Attachment&& attachment)
{
    ASSERT(isNull());

    m_attachment = WTFMove(attachment);
}

RefPtr<SharedMemory> SharedMemory::allocate(size_t size)
{
    void* data = commap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (data == MAP_FAILED) {
        return 0;
    }

    RefPtr<SharedMemory> instance = adoptRef(new SharedMemory());
    instance->m_data = data;
    return instance.release();
}

static inline int accessModeMMap(SharedMemory::Protection protection)
{
    switch (protection) {
    case SharedMemory::Protection::ReadOnly:
        return PROT_READ;
    case SharedMemory::Protection::ReadWrite:
        return PROT_READ | PROT_WRITE;
    }

    ASSERT_NOT_REACHED();
    return PROT_READ | PROT_WRITE;
}

RefPtr<SharedMemory> SharedMemory::map(const Handle& handle, Protection protection)
{
    ASSERT(!handle.isNull());

    if (!cheri_gettag(handle.m_attachment.cap()))
        return nullptr;
    else if (handle.m_attachment.type==CoportType)
        return nullptr;
    
    void * __capability data = handle.m_attachment.cap();
    data=SET_PROT(data,accessModeMMap(protection));

    RefPtr<SharedMemory> instance = wrapMap(handle.m_attachment.cap());
    instance->m_isWrappingMap = false;
    return instance;
}

RefPtr<SharedMemory> SharedMemory::wrapMap(void __capability * data)
{
    RefPtr<SharedMemory> instance = adoptRef(new SharedMemory());
    instance->m_data = data;
    instance->m_isWrappingMap = true;
    return instance;
}

SharedMemory::~SharedMemory()
{
    if (m_isWrappingMap)
        return;
m_data
    comunmap(m_data, cheri_getlen(m_data));
}

bool SharedMemory::createHandle(Handle& handle, Protection)
{
    ASSERT_ARG(handle, handle.isNull());
    ASSERT(m_data);
    ASSERT(cheri_gettag(m_data));

    handle.m_attachment = IPC::Attachment(m_data, cheri_getlen(m_data));

    return true;
}

unsigned SharedMemory::systemPageSize()
{
    static unsigned pageSize = 0;

    if (!pageSize)
        pageSize = getpagesize();

    return pageSize;
}

} // namespace WebKit

#endif

