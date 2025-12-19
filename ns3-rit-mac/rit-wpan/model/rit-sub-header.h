/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef NS3_LRWPAN_RIT_SUB_HEADER_H
#define NS3_LRWPAN_RIT_SUB_HEADER_H

#include "ns3/header.h"

#include <cstdint>
#include <ostream>

namespace ns3
{
namespace lrwpan
{

/**
 * @ingroup lrwpan
 *
 * @brief Experimental sub-header for RIT frame extensions.
 *
 * This header provides a compact flag field for experimental control signaling
 * (e.g., continuous transmission indication). It serializes into 1 byte.
 *
 * Layout (m_flags):
 *  - bit0: CONTINUOUS
 *  - bit1-7: reserved for future use
 */
class RitSubHeader : public Header
{
  public:
    RitSubHeader() = default;
    ~RitSubHeader() override = default;

    /**
     * @brief Set/clear the CONTINUOUS flag (bit0).
     */
    void SetContinuous(bool enabled);

    /**
     * @brief Return true if the CONTINUOUS flag (bit0) is set.
     */
    bool isContinuous() const;

    /**
     * @brief Set raw flags (all bits preserved as-is).
     */
    void SetSubHeaderFrameControl(uint8_t flags);

    /**
     * @brief Get raw flags (all bits preserved as-is).
     */
    uint8_t GetSubHeaderFrameControl() const;

    // ns-3 Header API
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

  private:
    uint8_t m_flags{0}; //!< bit0: CONTINUOUS, bit1-7: reserved
};

} // namespace lrwpan
} // namespace ns3

#endif // NS3_LRWPAN_RIT_SUB_HEADER_H
