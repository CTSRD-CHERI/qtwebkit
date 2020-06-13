/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

//#include "config.h"
#include "Attachment.h"
#include <coport.h>

namespace IPC {

Attachment::Attachment(coport_t port)
    : m_type(CoportType)
    , m_cap(port)
{
}

Attachment::Attachment(void * __capability cap)
    : m_type(CoMappedMemoryType)
    , m_cap(cap)
{
}

Attachment::Attachment(Attachment&& attachment)
    : m_type(attachment.m_type)
    , m_port(attachment.m_cap)
{
    attachment.m_type = Uninitialized;
    attachment.m_cap = NULL;
}

Attachment& Attachment::operator=(Attachment&& attachment)
{
    m_type = attachment.m_type;
    attachment.m_type = Uninitialized;
    m_cap = attachment.m_cap;
    attachment.m_cap = NULL;

    return *this;
}

Attachment::~Attachment()
{
    if (m_type == CoportType)
        coclose(m_cap);
    else if (m_type == CoMappedMemoryType)
        comunmap(m_cap);
}

#if 0
void Attachment::encode(ArgumentEncoder& encoder) const
{

    encoder << m_cap;
    encoder << m_type;
}

bool Attachment::decode(ArgumentDecoder& decoder, Attachment& attachment)
{
    ASSERT_ARG(attachment, !attachment.m_handle);

    void * __capability sourceCap;
    if (!decoder.decode(sourceHandle))
        return false;

    Type sourceType;
    if (!decoder.decode(sourceType))
        return false;

    attachment.m_cap = sourceCap;
    attachment.m_type = sourceType

    return true;
}
#endif

} // namespace IPC
