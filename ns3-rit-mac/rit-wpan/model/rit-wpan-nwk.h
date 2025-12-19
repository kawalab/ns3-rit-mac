/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef RIT_WPAN_NWK_H
#define RIT_WPAN_NWK_H

#include "rit-wpan-mac.h"

#include "ns3/object.h"
#include "ns3/random-variable-stream.h"

#include <cstdint>
#include <map>

namespace ns3
{
namespace lrwpan
{

/**
 * \brief Simplified rank-based routing layer for RIT-WPAN evaluation
 *
 * RitSimpleRouting provides a minimal network-layer functionality
 * required to enable multi-hop communication in RIT-WPAN simulations.
 *
 * Packet forwarding decisions are made solely based on node rank,
 * assuming a static tree topology rooted at a designated parent.
 *
 * NOTE:
 *  - No route discovery or maintenance is implemented.
 *  - This class is tightly coupled with the evaluation scenarios.
 */
class RitSimpleRouting : public Object
{
  public:
    /**
     * \brief Get the TypeId of this object
     * \return the TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Default constructor
     */
    RitSimpleRouting();

    ~RitSimpleRouting() override;

    /**
     * \brief Bootstrap procedure (not implemented)
     *
     * NOTE [NOT IMPLEMENTED / OUT OF SCOPE]:
     * Dynamic bootstrap logic is intentionally omitted in this
     * simplified evaluation-oriented implementation.
     */
    void Bootstrap();

    /**
     * \brief Handle incoming RIT request indication from MAC
     *
     * \param params RIT request indication parameters
     */
    void MlmeRitRequestIndication(MlmeRitRequestIndicationParams params);

    /**
     * \brief Set the underlying MAC instance
     * \param mac Pointer to RitWpanMac
     */
    void SetMac(Ptr<RitWpanMac> mac);

    /**
     * \brief Get the underlying MAC instance
     * \return Pointer to RitWpanMac
     */
    Ptr<RitWpanMac> GetMac() const;

    /**
     * \brief Set the rank of this node
     * \param rank Rank value
     */
    void SetRank(uint16_t rank);

    /**
     * \brief Get the rank of this node
     * \return Rank value
     */
    uint16_t GetRank() const;

    /**
     * \brief Set the short MAC address of this node
     * \param addr Short address
     */
    void SetShortAddress(Mac16Address addr);

    /**
     * \brief Send a packet via the network layer
     *
     * A new network handle is allocated internally.
     *
     * \param packet Packet to send
     * \param dst Destination MAC address
     */
    void SendRequest(Ptr<Packet> packet, Mac16Address dst);

    /**
     * \brief Send or re-send a packet with a specified network handle
     *
     * \param packet Packet to send
     * \param dst Destination MAC address
     * \param nwkHandle Network-layer handle
     */
    void SendRequest(Ptr<Packet> packet, Mac16Address dst, uint8_t nwkHandle);

    /**
     * \brief Indication of received data from MAC layer
     *
     * \param params MCPS indication parameters
     * \param pkt Received packet
     */
    void McpsDataIndication(McpsDataIndicationParams params, Ptr<Packet> pkt);

    /**
     * \brief Confirmation of transmitted data from MAC layer
     *
     * \param params MCPS confirm parameters
     */
    void McpsDataConfirm(McpsDataConfirmParams params);

    /**
     * \brief Callback type for network-layer packet reception
     */
    typedef Callback<void, Ptr<Packet>, const Mac16Address&> NwkRxCallback;

    /**
     * \brief Set the callback to notify upper layers of packet reception
     * \param cb Callback function
     */
    void SetNwkRxCallback(NwkRxCallback cb);

  private:
    // Node attributes
    uint16_t m_rank;            //!< Rank of this node
    Mac16Address m_shortAddr;   //!< Short MAC address
    Ptr<Packet> m_txPkt;        //!< Temporary pointer for transmission

    // Trace sources
    TracedCallback<Ptr<const Packet>> m_nwkTxTrace;
    TracedCallback<Ptr<const Packet>> m_nwkTxOkTrace;
    TracedCallback<Ptr<const Packet>> m_nwkTxDropTrace;
    TracedCallback<Ptr<const Packet>> m_nwkRxTrace;
    TracedCallback<Ptr<const Packet>> m_nwkRxDropTrace;
    TracedCallback<Ptr<const Packet>> m_nwkReTxTrace;

    // Upper-layer callback
    NwkRxCallback m_nwkRxCallback;

    // Underlying MAC
    Ptr<RitWpanMac> m_mac;

    // Handle and retry management
    std::map<uint8_t, std::pair<Ptr<Packet>, Mac16Address>> m_handleToPktMap;
    std::map<uint8_t, uint8_t> m_retryCountMap;
    std::map<uint8_t, uint8_t> m_msduToNwkHandleMap;

    SequenceNumber8 m_nwkHandle;
    SequenceNumber8 m_macHandle;

    static constexpr uint8_t MAX_RETRIES = 0;

    Ptr<UniformRandomVariable> m_reTxDelay;
};

} // namespace lrwpan
} // namespace ns3

#endif // RIT_WPAN_NWK_H
