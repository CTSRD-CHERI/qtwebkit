/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
//TODO-PBB:COPORT LICENSE 

#ifndef COMESG_PORT_H
#define COMESG_PORT_H

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include "Attachment.h"

#define COPORT_NULL NULL

namespace IPC {

class ComesgPort {
public:
    ComesgPort()
        : m_coport(NULL)
    {
    }

    ComesgPort(coport_t coport)
        : m_coport(coport)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        encoder << Attachment(m_coport);
    }

    static bool decode(ArgumentDecoder& decoder, ComesgPort& p)
    {
        Attachment attachment;
        if (!decoder.decode(attachment))
            return false;
        
        p.m_coport = attachment.coport();
        return true;
    }

    coport_t coport() const { return m_coport; }

private:
    coport_t m_coport;
};

} // namespace IPC

#endif // COMESG_PORT_H
