/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "rit-rank-helper.h"

#include "ns3/log.h"
#include "ns3/rit-wpan-net-device.h"

#include <vector>

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("RitWpanRankHelper");

namespace
{

Ptr<RitWpanNetDevice>
FindRitWpanDevice(Ptr<Node> node)
{
    for (uint32_t i = 0; i < node->GetNDevices(); ++i)
    {
        if (auto dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(i)))
        {
            return dev;
        }
    }
    return nullptr;
}

bool
HasNonZeroRank(const std::vector<uint8_t>& ranks)
{
    for (auto r : ranks)
    {
        if (r != 0)
        {
            return true;
        }
    }
    return false;
}

} // namespace

RitWpanRankHelper::RitWpanRankHelper() = default;
RitWpanRankHelper::~RitWpanRankHelper() = default;

void
RitWpanRankHelper::Install(NodeContainer c, uint8_t gridSizeX) const
{
    if (gridSizeX == 0)
    {
        NS_LOG_WARN("gridSizeX is 0. No ranks will be set.");
        return;
    }

    uint16_t nodeId = 1;
    for (auto i = c.Begin(); i != c.End(); ++i, ++nodeId)
    {
        Ptr<Node> node = *i;
        auto dev = FindRitWpanDevice(node);
        if (!dev)
        {
            NS_LOG_WARN("Node " << nodeId << " has no RitWpanNetDevice. Skipping.");
            continue;
        }

        // Rank assignment for grid-like router placement:
        // rank = floor((nodeId-1)/gridSizeX) + 1
        const uint8_t rank = static_cast<uint8_t>((nodeId - 1) / gridSizeX + 1);
        dev->SetRitRank(rank);
        dev->SetAddress(nodeId);
    }
}

void
RitWpanRankHelper::Install(NodeContainer c, const std::vector<uint8_t>& rankList) const
{
    if (rankList.empty())
    {
        NS_LOG_WARN("Empty rank list. No ranks will be set.");
        return;
    }
    if (!HasNonZeroRank(rankList))
    {
        NS_LOG_WARN("Rank list contains only zeros. No ranks will be set.");
        return;
    }

    uint32_t assigned = 0;
    uint32_t rankIndex = 0;
    uint16_t nodeId = 1;

    for (auto i = c.Begin(); i != c.End(); ++i, ++nodeId)
    {
        Ptr<Node> node = *i;
        auto dev = FindRitWpanDevice(node);
        if (!dev)
        {
            NS_LOG_WARN("Node " << nodeId << " has no RitWpanNetDevice. Skipping.");
            continue;
        }

        // Select the next non-zero rank (cycle through the list if needed)
        uint8_t rank = 0;
        for (uint32_t tries = 0; tries < rankList.size(); ++tries)
        {
            rank = rankList[rankIndex % rankList.size()];
            ++rankIndex;
            if (rank != 0)
            {
                break;
            }
        }

        if (rank == 0)
        {
            NS_LOG_WARN("Failed to select a non-zero rank for node " << nodeId << ". Skipping.");
            continue;
        }

        dev->SetRitRank(rank);
        dev->SetAddress(nodeId);
        ++assigned;
    }

    NS_LOG_INFO("Assigned ranks to " << assigned << " nodes (rankList size=" << rankList.size()
                                     << ").");
}

} // namespace lrwpan
} // namespace ns3
