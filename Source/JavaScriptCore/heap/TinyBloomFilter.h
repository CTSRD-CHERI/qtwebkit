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

#ifndef TinyBloomFilter_h
#define TinyBloomFilter_h

namespace JSC {

class TinyBloomFilter {
public:
    TinyBloomFilter();

    void add(size_t);
    void add(TinyBloomFilter&);
    bool ruleOut(size_t) const; // True for 0.
    void reset();

private:
    size_t m_bits;
};

inline TinyBloomFilter::TinyBloomFilter()
    : m_bits(0)
{
}

inline void TinyBloomFilter::add(size_t bits)
{
    m_bits |= bits;
}

inline void TinyBloomFilter::add(TinyBloomFilter& other)
{
    m_bits |= other.m_bits;
}

inline bool TinyBloomFilter::ruleOut(size_t bits) const
{
    if (!bits)
        return true;

    if ((bits & m_bits) != bits)
        return true;

    return false;
}

inline void TinyBloomFilter::reset()
{
    m_bits = 0;
}

} // namespace JSC

#endif // TinyBloomFilter_h
