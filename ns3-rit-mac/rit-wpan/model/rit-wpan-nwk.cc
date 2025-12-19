/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

/*
 * NOTE [EXPERIMENTAL / SIMPLIFIED NETWORK LAYER]:
 *
 * This file implements a minimal network-layer component based on
 * rank-based routing for RIT-WPAN evaluation.
 *
 * The routing behavior is intentionally simplified and relies on
 * statically assigned rank values to forward packets toward a
 * designated root (parent) node.
 *
 * Design scope:
 *  - Rank-based routing without route discovery or maintenance
 *  - Uplink-oriented, tree-like forwarding
 *  - Best-effort retransmission on MAC-layer failures
 *
 * This implementation is required to enable multi-hop evaluation,
 * while keeping the network-layer behavior simple and deterministic
 * to avoid masking MAC-layer effects.
 */

#include "rit-wpan-nwk.h"
#include "rit-wpan-nwk-header.h"

#include "ns3/log.h"
#include "ns3/mac16-address.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <cstdint>

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("RitSimpleRouting");
NS_OBJECT_ENSURE_REGISTERED(RitSimpleRouting);

TypeId
RitSimpleRouting::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RitSimpleRouting")
            .SetParent<Object>()
            .SetGroupName("LrWpan")
            .AddConstructor<RitSimpleRouting>()
            .AddTraceSource("NwkTx",
                            "NWK layer transmit trace",
                            MakeTraceSourceAccessor(&RitSimpleRouting::m_nwkTxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("NwkTxOk",
                            "NWK layer successful transmit trace",
                            MakeTraceSourceAccessor(&RitSimpleRouting::m_nwkTxOkTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("NwkTxDrop",
                            "NWK layer transmit drop trace",
                            MakeTraceSourceAccessor(&RitSimpleRouting::m_nwkTxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("NwkRx",
                            "NWK layer receive trace",
                            MakeTraceSourceAccessor(&RitSimpleRouting::m_nwkRxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("NwkRxDrop",
                            "NWK layer receive drop trace",
                            MakeTraceSourceAccessor(&RitSimpleRouting::m_nwkRxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("NwkReTx",
                            "NWK layer re-transmit packet trace",
                            MakeTraceSourceAccessor(&RitSimpleRouting::m_nwkReTxTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

RitSimpleRouting::RitSimpleRouting()
{
    m_txPkt = nullptr;
    m_reTxDelay = CreateObject<UniformRandomVariable>();
}

RitSimpleRouting::~RitSimpleRouting() = default;

void
RitSimpleRouting::Bootstrap()
{
    // NOTE [NOT IMPLEMENTED]:
    // Bootstrap procedures (e.g., parent discovery / rank negotiation)
    // are intentionally omitted in this simplified implementation.
}

void
RitSimpleRouting::McpsDataIndication(McpsDataIndicationParams params, Ptr<Packet> p)
{
    NS_LOG_FUNCTION_NOARGS();

    RitNwkHeader nwkHdr;
    p->RemoveHeader(nwkHdr);

    NS_LOG_DEBUG("McpsDataIndication: SrcAddr=" << nwkHdr.GetSrcAddr()
                                                << ", DstAddr=" << nwkHdr.GetDstAddr()
                                                << ", Rank=" << nwkHdr.GetRank());

    // Case 1: The packet is destined to this node.
    if (nwkHdr.GetDstAddr() == m_shortAddr)
    {
        if (m_nwkRxCallback.IsNull())
        {
            NS_LOG_DEBUG("Packet is for me but no RX callback is set; dropping.");
            return;
        }

        NS_LOG_DEBUG("Packet is for me; delivering to upper layer.");
        m_nwkRxTrace(p);
        m_nwkRxCallback(p, nwkHdr.GetSrcAddr());
        return;
    }

    /*
     * Case 2: Forwarding (simplified).
     * NOTE [EXPERIMENTAL]:
     * Current behavior: forward only if the packet rank is higher than my rank.
     * This is used as a simplified tree-based uplink forwarding rule.
     */
    if (nwkHdr.GetRank() > m_rank)
    {
        NS_LOG_DEBUG("Forwarding packet (rank-based): DstAddr=" << nwkHdr.GetDstAddr()
                                                                << ", MyRank=" << m_rank
                                                                << ", PktRank=" << nwkHdr.GetRank());
        m_nwkRxTrace(p);

        // IMPORTANT: Keep forwarding behavior unchanged.
        // (Even if a "parent" address is conceptually expected, the current logic forwards to
        //  nwkHdr.GetDstAddr() via SendRequest, as implemented originally.)
        SendRequest(p, nwkHdr.GetDstAddr());
        return;
    }

    NS_LOG_DEBUG("Dropping packet (not forwarded by rank rule).");
    m_nwkRxDropTrace(p);
}

void
RitSimpleRouting::McpsDataConfirm(McpsDataConfirmParams params)
{
    NS_LOG_FUNCTION(this << (uint32_t)params.m_msduHandle << ":" << params.m_status);

    const uint8_t msduHandle = params.m_msduHandle;

    // Resolve NWK handle from MAC handle.
    auto it = m_msduToNwkHandleMap.find(msduHandle);
    if (it == m_msduToNwkHandleMap.end())
    {
        NS_LOG_WARN("Unknown msduHandle: " << (uint32_t)msduHandle);
        return;
    }

    const uint8_t nwkHandle = it->second;

    auto pktIt = m_handleToPktMap.find(nwkHandle);
    if (pktIt == m_handleToPktMap.end())
    {
        NS_LOG_WARN("Packet not found for nwkHandle=" << (uint32_t)nwkHandle);
        m_msduToNwkHandleMap.erase(msduHandle);
        return;
    }

    Ptr<Packet> packet = pktIt->second.first;
    const Mac16Address dst = pktIt->second.second;
    const uint8_t retries = m_retryCountMap[nwkHandle];

    switch (params.m_status)
    {
    case MacStatus::SUCCESS:
        NS_LOG_DEBUG("Tx SUCCESS: nwkHandle=" << (uint32_t)nwkHandle);
        m_nwkTxOkTrace(packet);
        break;

    case MacStatus::NO_ACK:
        NS_LOG_DEBUG("Tx RETRY (" << (uint32_t)retries << "/" << (uint32_t)MAX_RETRIES
                                  << ") nwkHandle=" << (uint32_t)nwkHandle);

        if (retries < MAX_RETRIES)
        {
            m_retryCountMap[nwkHandle] = retries + 1;

            // Keep behavior: remove header before re-adding it in SendRequest().
            RitNwkHeader nwkHdr;
            packet->RemoveHeader(nwkHdr);

            m_nwkReTxTrace(packet);

            /*
             * NOTE [EXPERIMENTAL]:
             * Retransmission delay is randomized with a fixed range (0..5 seconds),
             * kept intentionally simple for evaluation.
             */
            const Time delay = Seconds(m_reTxDelay->GetValue(0, 5));

            Simulator::Schedule(
                delay,
                static_cast<void (RitSimpleRouting::*)(Ptr<Packet>, Mac16Address, uint8_t)>(
                    &RitSimpleRouting::SendRequest),
                this,
                packet,
                dst,
                nwkHandle);

            // Keep original cleanup behavior.
            m_msduToNwkHandleMap.erase(msduHandle);
            return;
        }

        NS_LOG_DEBUG("Max retries reached; dropping packet.");
        m_nwkTxDropTrace(packet);
        break;

    case MacStatus::CHANNEL_ACCESS_FAILURE:
        NS_LOG_DEBUG("Tx CSMA failure: nwkHandle=" << (uint32_t)nwkHandle);
        // fallthrough

    default:
        NS_LOG_DEBUG("Tx FAILED with status=" << params.m_status);
        m_nwkTxDropTrace(packet);
        break;
    }

    // Cleanup (keep behavior unchanged).
    m_handleToPktMap.erase(nwkHandle);
    m_retryCountMap.erase(nwkHandle);
    m_msduToNwkHandleMap.erase(msduHandle);
}

void
RitSimpleRouting::MlmeRitRequestIndication(MlmeRitRequestIndicationParams params)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> ritPayload =
        Create<Packet>(params.m_ritRequestPayload.data(), params.m_ritRequestPayload.size());

    RitNwkHeader nwkHdr;
    ritPayload->PeekHeader(nwkHdr);

    /*
     * NOTE [EXPERIMENTAL]:
     * This is a simplified policy to trigger MAC transmission upon receiving a
     * RIT request from a lower-rank node.
     */
    if (nwkHdr.GetRank() + 1 == m_rank)
    {
        NS_LOG_DEBUG("Processing RIT request from lower rank: " << nwkHdr.GetRank());
        Simulator::ScheduleNow(&RitWpanMac::SendRitData, m_mac);
    }
    else
    {
        NS_LOG_DEBUG("RIT request ignored (rank mismatch). MyRank=" << m_rank);
    }
}

// New transmission request (allocate NWK handle internally).
void
RitSimpleRouting::SendRequest(Ptr<Packet> packet, Mac16Address dst)
{
    const uint8_t nwkHandle = m_nwkHandle.GetValue();
    m_retryCountMap[nwkHandle] = 0;
    SendRequest(packet, dst, nwkHandle);
}

// Retransmission / handle-specified request.
void
RitSimpleRouting::SendRequest(Ptr<Packet> packet, Mac16Address dst, uint8_t nwkHandle)
{
    NS_LOG_FUNCTION(this << packet << dst << (uint32_t)nwkHandle);

    const uint8_t msduHandle = m_macHandle.GetValue();
    m_macHandle++;

    McpsDataRequestParams params;
    params.m_srcAddrMode = AddressMode::SHORT_ADDR;
    params.m_dstAddrMode = AddressMode::SHORT_ADDR;
    params.m_dstAddr = dst;
    params.m_msduHandle = msduHandle;
    params.m_txOptions |= TX_OPTION_ACK;

    // Add network header (keep fields unchanged).
    RitNwkHeader hdr;
    hdr.SetSrcAddr(m_shortAddr);
    hdr.SetDstAddr(dst);
    hdr.SetRank(m_rank);
    packet->AddHeader(hdr);

    // Trace and store a copy (keep behavior unchanged).
    Ptr<Packet> pktCopy = packet->Copy();
    m_nwkTxTrace(pktCopy);

    // Register handle mappings (keep behavior unchanged).
    m_handleToPktMap[nwkHandle] = std::make_pair(pktCopy, dst);
    m_msduToNwkHandleMap[msduHandle] = nwkHandle;

    m_mac->McpsDataRequest(params, packet);
}

void
RitSimpleRouting::SetMac(Ptr<RitWpanMac> mac)
{
    m_mac = mac;
}

Ptr<RitWpanMac>
RitSimpleRouting::GetMac() const
{
    return m_mac;
}

void
RitSimpleRouting::SetRank(uint16_t rank)
{
    NS_LOG_FUNCTION(this << rank);
    m_rank = rank;

    // Build and set RIT request payload (keep behavior unchanged).
    RitNwkHeader nwkHeader;
    nwkHeader.SetDstAddr(Mac16Address("FF:FF"));
    nwkHeader.SetRank(m_rank);

    Ptr<Packet> ritRequestPayload = Create<Packet>(0);
    ritRequestPayload->AddHeader(nwkHeader);

    std::vector<uint8_t> payload(ritRequestPayload->GetSize());
    ritRequestPayload->CopyData(payload.data(), payload.size());

    MacPibAttributeIdentifier id = macRitRequestPayload;
    Ptr<MacPibAttributes> attribute = Create<MacPibAttributes>();
    attribute->macRitRequestPayload = payload;

    m_mac->MlmeSetRequest(id, attribute);
}

uint16_t
RitSimpleRouting::GetRank() const
{
    return m_rank;
}

void
RitSimpleRouting::SetShortAddress(Mac16Address addr)
{
    m_shortAddr = addr;
}

void
RitSimpleRouting::SetNwkRxCallback(NwkRxCallback cb)
{
    m_nwkRxCallback = cb;
}

} // namespace lrwpan
} // namespace ns3
