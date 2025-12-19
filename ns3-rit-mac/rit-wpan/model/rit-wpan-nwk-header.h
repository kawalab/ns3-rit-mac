/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef RIT_NWK_HEADER_H
#define RIT_NWK_HEADER_H

#include "ns3/header.h"
#include "ns3/mac16-address.h"

namespace ns3
{
namespace lrwpan
{

/**
 * \brief Minimal network-layer header for rank-based routing.
 *
 * This header is used by the simplified routing logic implemented
 * for receiver-initiated (RIT) MAC protocol evaluation.
 */
class RitNwkHeader : public Header
{
  public:
    RitNwkHeader();
    ~RitNwkHeader() override;

    /** Set the node rank carried by this header */
    void SetRank(uint16_t rank);

    /** Get the node rank */
    uint16_t GetRank() const;

    /** Set the source MAC short address */
    void SetSrcAddr(Mac16Address addr);

    /** Get the source MAC short address */
    Mac16Address GetSrcAddr() const;

    /** Set the destination MAC short address */
    void SetDstAddr(Mac16Address addr);

    /** Get the destination MAC short address */
    Mac16Address GetDstAddr() const;

    // ns-3 Header API
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

  private:
    uint16_t m_rank;        //!< Node rank used for rank-based forwarding
    Mac16Address m_srcAddr; //!< Source address (currently optional in RIT mode)
    Mac16Address m_dstAddr; //!< Destination address
};

} // namespace lrwpan
} // namespace ns3

#endif // RIT_NWK_HEADER_H
