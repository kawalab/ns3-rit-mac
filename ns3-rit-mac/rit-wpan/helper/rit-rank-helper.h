/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef NS3_RIT_WPAN_RANK_HELPER_H
#define NS3_RIT_WPAN_RANK_HELPER_H

#ifndef NS3_RIT_WPAN_RANK_HELPER_H
#define NS3_RIT_WPAN_RANK_HELPER_H

#include "ns3/node-container.h"

#include <cstdint>
#include <vector>

namespace ns3
{
namespace lrwpan
{

/**
 * Helper to assign RIT rank values to RitWpanNetDevice instances on nodes.
 *
 * Note: This helper assumes ranks are used as routing-layer metadata in experiments.
 */
class RitWpanRankHelper
{
  public:
    RitWpanRankHelper();
    ~RitWpanRankHelper();

    RitWpanRankHelper(const RitWpanRankHelper&) = delete;
    RitWpanRankHelper& operator=(const RitWpanRankHelper&) = delete;

    /**
     * Assign ranks assuming router nodes are placed on a grid.
     *
     * Rank is computed from the node order in the container:
     * rank = floor((nodeId-1)/gridSizeX) + 1
     */
    void Install(NodeContainer c, uint8_t gridSizeX) const;

    /**
     * Assign ranks using a user-provided list.
     * Ranks are assigned in order, cycling through the list if needed.
     * A rank value of 0 is treated as invalid and skipped.
     */
    void Install(NodeContainer c, const std::vector<uint8_t>& rankList) const;
};

} // namespace lrwpan
} // namespace ns3

#endif // NS3_RIT_WPAN_RANK_HELPER_H
