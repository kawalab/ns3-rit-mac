/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

/*
 * NOTE [EXPERIMENTAL / SIMPLIFIED NETWORK HEADER]:
 *
 * This header defines a minimal network-layer header used by the
 * simplified rank-based routing mechanism for RIT-WPAN evaluation.
 *
 * The header carries only the essential information required for
 * rank-based forwarding:
 *  - Node rank
 *  - Source short address
 *  - Destination short address
 *
 * It is intentionally compact and does not aim to be compatible
 * with any standardized NWK-layer format.
 */


#include "rit-wpan-nwk-header.h"

#include "ns3/address-utils.h"

#include <cstdint>

namespace ns3
{
namespace lrwpan
{

RitNwkHeader::RitNwkHeader()
{
    // Default rank initialization
    SetRank(0);
}

RitNwkHeader::~RitNwkHeader()
{
}

void
RitNwkHeader::SetRank(uint16_t rank)
{
    m_rank = rank;
}

uint16_t
RitNwkHeader::GetRank() const
{
    return m_rank;
}

void
RitNwkHeader::SetSrcAddr(Mac16Address addr)
{
    m_srcAddr = addr;
}

Mac16Address
RitNwkHeader::GetSrcAddr() const
{
    return m_srcAddr;
}

void
RitNwkHeader::SetDstAddr(Mac16Address addr)
{
    m_dstAddr = addr;
}

Mac16Address
RitNwkHeader::GetDstAddr() const
{
    return m_dstAddr;
}

void
RitNwkHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    // Serialize fields in fixed order:
    // 1) Rank
    // 2) Source short address
    // 3) Destination short address
    i.WriteU16(m_rank);
    WriteTo(i, m_srcAddr);
    WriteTo(i, m_dstAddr);
}

uint32_t
RitNwkHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    // Deserialize fields in the same order as serialization
    m_rank = i.ReadU16();
    ReadFrom(i, m_srcAddr);
    ReadFrom(i, m_dstAddr);

    return i.GetDistanceFrom(start);
}

uint32_t
RitNwkHeader::GetSerializedSize() const
{
    // Rank: 2 bytes
    // Source address: 2 bytes
    // Destination address: 2 bytes
    return 6;
}

TypeId
RitNwkHeader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::lrwpan::RitNwkHeader")
            .SetParent<Header>()
            .SetGroupName("LrWpan")
            .AddConstructor<RitNwkHeader>();

    return tid;
}

TypeId
RitNwkHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RitNwkHeader::Print(std::ostream& os) const
{
    os << "RitNwkHeader"
       << " [Rank=" << m_rank
       << ", Src=" << m_srcAddr
       << ", Dst=" << m_dstAddr
       << "]";
}

} // namespace lrwpan
} // namespace ns3
