/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

/*
 * NOTE [EXPERIMENTAL / CURRENTLY UNUSED]:
 *
 * This sub-header was introduced for experimental extensions of the RIT MAC,
 * such as signaling continuous transmission or other control hints between nodes.
 *
 * In the current public evaluation scenarios, this sub-header is NOT actively used,
 * and its insertion/removal is disabled to keep the packet format minimal and stable.
 *
 * The implementation is intentionally preserved to:
 *  - document the design space explored during early experiments, and
 *  - serve as a foundation for future experimental extensions.
 *
 * As a result, this file may appear unused in the default simulation workflow,
 * but it is kept by design.
 */

#include "rit-sub-header.h"

#include <ostream>

namespace ns3
{
namespace lrwpan
{

namespace
{
constexpr uint8_t kFlagContinuous = 0x01; // bit0: CONTINUOUS
} // namespace

void
RitSubHeader::SetContinuous(bool enabled)
{
    if (enabled)
    {
        m_flags |= kFlagContinuous;
    }
    else
    {
        m_flags &= ~kFlagContinuous;
    }
}

bool
RitSubHeader::isContinuous() const
{
    return (m_flags & kFlagContinuous) != 0;
}

void
RitSubHeader::SetSubHeaderFrameControl(uint8_t flags)
{
    m_flags = flags;
}

uint8_t
RitSubHeader::GetSubHeaderFrameControl() const
{
    return m_flags;
}

TypeId
RitSubHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::lrwpan::RitSubHeader")
                            .SetParent<Header>()
                            .SetGroupName("LrWpan")
                            .AddConstructor<RitSubHeader>();
    return tid;
}

TypeId
RitSubHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RitSubHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_flags);
}

uint32_t
RitSubHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_flags = i.ReadU8();
    return i.GetDistanceFrom(start);
}

uint32_t
RitSubHeader::GetSerializedSize() const
{
    return 1;
}

void
RitSubHeader::Print(std::ostream& os) const
{
    os << "RitSubHeader: CONTINUOUS=" << (isContinuous() ? "1" : "0");
}

} // namespace lrwpan
} // namespace ns3
