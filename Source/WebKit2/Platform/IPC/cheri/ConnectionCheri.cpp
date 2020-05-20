/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2020 Peter S. Blandford-Baker
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
#include "Connection.h"

#include "DataReference.h"
#include "SharedMemory.h"
#include "coport.h"
#include "comsg.h"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#include <wtf/Assertions.h>
#include <wtf/RandomNumber.h>
#include <wtf/StdLibExtras.h>
#include <wtf/UniStdExtras.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/win/WCharStringExtras.h>

#if PLATFORM(GTK)
#include <gio/gio.h>
#elif PLATFORM(QT)
#include <QPointer>
#include <QCoportNotifier>
#endif

namespace IPC {

//static const size_t messageMaxSize = 4096;
static const size_t attachmentMaxAmount = 255;

class MessageInfo {
public:
    MessageInfo() { }

    MessageInfo(size_t bodySize, size_t initialAttachmentCount)
        : m_bodySize(bodySize)
        , m_attachmentCount(initialAttachmentCount)
    {
    }

    size_t bodySize() const { return m_bodySize; }

    size_t attachmentCount() const { return m_attachmentCount; }

private:
    size_t m_bodySize;
    size_t m_attachmentCount;

};

void Connection::platformInitialize(Identifier identifier)
{
    if(m_isServer)
    {
        if(identifier.coport!=NULL)
            m_remoteCoport.port = identifier.coport;
        if(identifier.coportName!=NULL)
            strncpy(m_remoteCoport.name,identifier.coportName,COPORT_NAME_LEN);
        memset(m_localCoport,0,sizeof(named_port_t));
    } 
    else {
        if(identifier.coport!=NULL)
            m_localCoport.port = identifier.coport;
        if(identifier.coportName!=NULL)
            strncpy(m_localCoport.name,identifier.coportName,COPORT_NAME_LEN);
        memset(m_remoteCoport,0,sizeof(named_port_t));
    }

#if PLATFORM(QT)
    m_coportNotifier=0;
#endif
}

void Connection::platformInvalidate()
{
    if (m_remoteCoport != NULL )
        coclose(m_remoteCoport.port);
    if (m_localCoport != NULL)    
        coclose(m_localCoport.port);
    m_isConnected = false;
}

#if PLATFORM(QT)
class CoportNotifierResourceGuard {
public:
    CoportNotifierResourceGuard(QCoportNotifier* coportNotifier)
        : m_coportNotifier(coportNotifier)
    {
        m_coportNotifier.data()->setEnabled(false);
    }

    ~CoportNotifierResourceGuard()
    {
        if (m_coportNotifier)
            m_coportNotifier.data()->setEnabled(true);
    }

private:
    QPointer<QCoportNotifier> const m_coportNotifier;
};
#endif

bool Connection::processMessage(void * __capability msg)
{
    if (cheri_getlen(msg) < sizeof(MessageInfo))
        return false;

    void * __capability messageData = msg;
    MessageInfo messageInfo;
    memcpy(&messageInfo, messageData, sizeof(messageInfo));
    messageData = cheri_setoffset(msg,sizeof(messageInfo));

    size_t bodyLength =  messageInfo.bodySize();
    size_t attachmentLength = messageInfo.attachmentCount() * sizeof(Attachment);
    void * __capability body = cheri_csetboundsexact(messageData,bodyLength);
    messageData = cheri_setoffset(msg,sizeof(bodyLength));
    Attachment * __capability attachments = cheri_csetboundsexact(messageData,bodyLength); 

    auto decoder = std::make_unique<MessageDecoder>(DataReference(body, messageInfo.bodySize()), WTFMove(attachments));

    processIncomingMessage(WTFMove(decoder));
    return true;
}

void Connection::readyReadHandler()
{
#if PLATFORM(QT)
    SocketNotifierResourceGuard socketNotifierEnabler(m_socketNotifier);
#endif
    void * __capability msg = NULL;
    while (true) {
        int corecv_result = corecv(m_localCoport, &msg, sizeof(msg));

        if (corecv_result < 0) {
            // EINTR was already handled by readBytesFromSocket.
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            if (m_isConnected) {
                WTFLogAlways("Error receiving IPC message on coport in process %d: %s", m_socketDescriptor, getpid(), strerror(errno));
                connectionDidClose();
            }
            return;
        }

        if (!corecv_result) {
            connectionDidClose();
            return;
        }

        // Process messages from data received.
        while (true) {
            if (!processMessage(msg))
                break;
        }
    }
}

bool Connection::open()
{
#if PLATFORM(QT)
    ASSERT(!m_coportNotifier);
#endif

    RefPtr<Connection> protectedThis(this);
    m_isConnected = true;
#if PLATFORM(QT)
    m_coportNotifier = m_connectionQueue->registerCoportEventHandler(m_localCoport, QCoportNotifier::Read,
        [protectedThis] {
            protectedThis->readyReadHandler();
        });
#endif

    // Schedule a call to readyReadHandler. Data may have arrived before installation of the signal handler.
    m_connectionQueue->dispatch([protectedThis] {
        protectedThis->readyReadHandler();
    });
    }

    return true;
}

bool Connection::platformCanSendOutgoingMessages() const
{
    return m_isConnected;
}

bool Connection::sendOutgoingMessage(std::unique_ptr<MessageEncoder> encoder)
{
#if PLATFORM(QT)
    ASSERT(m_coportNotifier);
#endif

    Vector<Attachment> attachments = encoder->releaseAttachments();
    if (attachments.size() > (attachmentMaxAmount - 1)) {
        ASSERT_NOT_REACHED();
        return false;
    }

    MessageInfo messageInfo(encoder->bufferSize(), attachments.size());
    size_t bodySize = encoder->bufferSize();
    size_t messageSize = sizeof(messageInfo) + (attachments.size() * sizeof(Attachment)) + encoder->bufferSize();
    
    unsigned char * messageBuffer = fastMalloc(messageSize);
    memcpy(messageBuffer,&messageInfo,sizeof(MessageInfo));
    memcpy(messageBuffer+sizeof(MessageInfo),encoder->buffer(),encoder->bufferSize());
    memcpy(messageBuffer+sizeof(MessageInfo)+encoder->bufferSize(),attachments.data(),attachments.size()*sizeof(Attachment));

    while (cosend(m_remoteCoport,messageBuffer.messageSize) == -1) {
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            pollcoport_T pollcp = make_pollcoport(m_remoteCoport,COPOLL_OUT);
            copoll(&pollfd, 1, -1);
            continue;
        }
        if (m_isConnected)
            WTFLogAlways("Error sending IPC message: %s", strerror(errno));
        return false;
    }
    return true;
}

Connection::CoportConnectionPair Connection::createPlatformConnection(unsigned options)
{
    //Webkit's notion
    UNUSED_PARAM(options);
    CoportConnectionPair coport_conn;
    coport_t port;
    Vector<UChar> coport_name;

    unsigned randomID = randomNumber() * std::numeric_limits<unsigned>::max();
    coport_name=String::format("com.apple.WebKit.%x", randomID).charactersWithNullTermination();
    coopen(coport_name,COCARRIER,&coport_conn.client.localCoport);

    randomID = randomNumber() * std::numeric_limits<unsigned>::max();
    coport_name=String::format("com.apple.WebKit.%x", randomID).charactersWithNullTermination();
    coopen(coport_name,COCARRIER,&coport_conn.server.localCoport);
    
    coport_conn.client.remoteCoport=coport_clearperm(coport_conn.client.localCoport,COPORT_PERM_RECV);
    coport_conn.server.remoteCoport=coport_clearperm(coport_conn.server.localCoport,COPORT_PERM_RECV);
    //coport_pair.client.port=coport_clearperm(port,COPORT_PERM_SEND);
    //coport_pair.server.port=coport_clearperm(port,COPORT_PERM_RECV);
    return coport_pair;
}
    
void Connection::willSendSyncMessage(unsigned flags)
{
    UNUSED_PARAM(flags);
}
    
void Connection::didReceiveSyncReply(unsigned flags)
{
    UNUSED_PARAM(flags);    
}

#if PLATFORM(QT)
void Connection::setShouldCloseConnectionOnProcessTermination(WebKit::PlatformProcessIdentifier process)
{
    RefPtr<Connection> protectedThis(this);
    m_connectionQueue->dispatchOnTermination(process,
        [protectedThis] {
            protectedThis->connectionDidClose();
        });
}
#endif

} // namespace IPC
