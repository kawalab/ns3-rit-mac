/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "rit-wpan-mac.h"

#include "rit-sub-header.h"
#include "rit-wpan-precs.h"
#include "rit-wpan-precsb.h"

#include "ns3/lr-wpan-constants.h"
#include "ns3/lr-wpan-csmaca.h"
#include "ns3/lr-wpan-mac-header.h"
#include "ns3/lr-wpan-mac-pl-headers.h"
#include "ns3/lr-wpan-mac-trailer.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

#include <cstdint>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                                     \
    std::clog << "[" << m_shortAddress << "|"                                                     \
            << m_macRitPeriodTime.Get().GetSeconds() << "s|"                                      \
            << m_macRitTxWaitDurationTime.Get().GetMilliSeconds() << "ms|"                        \
            << m_ritMacMode << "|Qs:" << static_cast<uint32_t>(m_txQueue.size()) << "|"           \
            << m_macState << "|" << m_periodicRitDataRequestEvent.IsPending() << "] ";

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("RitWpanMac");
NS_OBJECT_ENSURE_REGISTERED(RitWpanMac);

std::ostream&
operator<<(std::ostream& os, const RitMacMode& ritMode)
{
    switch (ritMode)
    {
    case RIT_MODE_DISABLED:
        os << "RIT_MODE_DISABLED";
        break;
    case SENDER_MODE:
        os << "SENDER";
        break;
    case RECEIVER_MODE:
        os << "RECEIVER";
        break;
    case SLEEP_MODE:
        os << "SLEEP";
        break;
    case BOOTSTRAP_MODE:
        os << "BOOTSTRAP";
        break;
    }
    return os;
}

TypeId
RitWpanMac::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RitWpanMac")
            .SetParent<LrWpanMac>()
            .SetGroupName("LrWpan")
            .AddConstructor<RitWpanMac>()
            .AddAttribute("macRitPeriod",
                          "RIT interval (0x000000 ~ 0xFFFFFF)",
                          UintegerValue(0),
                          MakeUintegerAccessor(&RitWpanMac::m_macRitPeriod),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("macRitDataWaitDuration",
                          "Reception waiting time after RIT (0x00 ~ 0xFF)",
                          UintegerValue(1),
                          MakeUintegerAccessor(&RitWpanMac::m_macRitDataWaitDuration),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("macRitTxWaitDuration",
                          "Beacon waiting time (macRitPeriod or more, up to 0xFFFFFF)",
                          UintegerValue(65),
                          MakeUintegerAccessor(&RitWpanMac::m_macRitTxWaitDuration),
                          MakeUintegerChecker<uint32_t>())
            .AddTraceSource("MacMode",
                            "Current RIT MAC mode",
                            MakeTraceSourceAccessor(&RitWpanMac::m_ritMacMode),
                            "ns3::lrwpan::RitMacMode")
            .AddTraceSource("MacRitPeriod",
                            "Trace of macRitPeriod changes",
                            MakeTraceSourceAccessor(&RitWpanMac::m_macRitPeriod),
                            "ns3::TracedValueCallback::Uint32")
            .AddTraceSource("MacRitDataWaitDuration",
                            "Trace of macRitDataWaitDuration changes",
                            MakeTraceSourceAccessor(&RitWpanMac::m_macRitDataWaitDuration),
                            "ns3::TracedValueCallback::Uint8")
            .AddTraceSource("MacRitTxWaitDuration",
                            "Trace of macRitTxWaitDuration changes",
                            MakeTraceSourceAccessor(&RitWpanMac::m_macRitTxWaitDuration),
                            "ns3::TracedValueCallback::Uint32")
            .AddTraceSource("BeaconWaitEvent",
                            "Beacon wait events (start, end, timeout) with timestamps",
                            MakeTraceSourceAccessor(&RitWpanMac::m_beaconWaitTrace),
                            "ns3::TracedCallback::StringTime")
            .AddTraceSource("DataWaitEvent",
                            "Data wait events (start, end, timeout) with timestamps",
                            MakeTraceSourceAccessor(&RitWpanMac::m_dataWaitTrace),
                            "ns3::TracedCallback::StringTime");
    return tid;
}

RitWpanMac::RitWpanMac()
{
    NS_LOG_FUNCTION(this);
    // Initialize RIT-specific parameters
    ChangeRitMacMode(RIT_MODE_DISABLED);
    m_timeDriftApplier = CreateObject<TimeDriftApplier>();
    m_timeDriftApplier->SetDriftRatio(10); // Set a default drift ratio of 10%
    m_clockDriftApplier = CreateObject<ClockDriftApplier>();
    m_rxAlwaysOn = false; // Default to false, can be set later

    m_macRitPeriodTime = Seconds(5);
    m_macRitDataWaitDurationTime = MilliSeconds(10);
    m_macRitTxWaitDurationTime = MilliSeconds(5000);

    SetMacMaxFrameRetries(0);
}

RitWpanMac::~RitWpanMac()
{
}

void
RitWpanMac::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    ChangeRitMacMode(SLEEP_MODE); // Initial mode at initialization (before starting RIT cycle)
    m_clockDriftApplier->Initialize(m_shortAddress.ConvertToInt(), 1);
    LrWpanMac::DoInitialize();
}

void
RitWpanMac::DoDispose()
{
    NS_LOG_FUNCTION(this);
    // Cancel any scheduled events
    m_ritDataWaitTimeout.Cancel();
    m_ritTxWaitTimeout.Cancel();
    m_periodicRitDataRequestEvent.Cancel();

    // Chain up to the parent class
    LrWpanMac::DoDispose();
}

void
RitWpanMac::McpsDataRequest(McpsDataRequestParams params, Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    if (!IsRitModeEnabled())
    {
        LrWpanMac::McpsDataRequest(params, p);
        return;
    }

    McpsDataConfirmParams confirmParams;
    confirmParams.m_msduHandle = params.m_msduHandle;

    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_DATA, m_macDsn.GetValue());
    m_macDsn++;

    if (p->GetSize() > lrwpan::aMaxPhyPacketSize - lrwpan::aMinMPDUOverhead)
    {
        NS_LOG_ERROR(this << " packet too big: " << p->GetSize());
        confirmParams.m_status = MacStatus::FRAME_TOO_LONG;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }

    if ((params.m_srcAddrMode == NO_PANID_ADDR) && (params.m_dstAddrMode == NO_PANID_ADDR))
    {
        NS_LOG_ERROR(this << " Can not send packet with no Address field");
        confirmParams.m_status = MacStatus::INVALID_ADDRESS;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }
    switch (params.m_srcAddrMode)
    {
    case NO_PANID_ADDR:
        macHdr.SetSrcAddrMode(params.m_srcAddrMode);
        macHdr.SetNoPanIdComp();
        break;
    case ADDR_MODE_RESERVED:
        NS_ABORT_MSG("Can not set source address type to ADDR_MODE_RESERVED. Aborting.");
        break;
    case SHORT_ADDR:
        macHdr.SetSrcAddrMode(params.m_srcAddrMode);
        macHdr.SetSrcAddrFields(GetPanId(), GetShortAddress());
        break;
    case EXT_ADDR:
        macHdr.SetSrcAddrMode(params.m_srcAddrMode);
        macHdr.SetSrcAddrFields(GetPanId(), GetExtendedAddress());
        break;
    default:
        NS_LOG_ERROR(this << " Can not send packet with incorrect Source Address mode = "
                          << params.m_srcAddrMode);
        confirmParams.m_status = MacStatus::INVALID_ADDRESS;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }
    switch (params.m_dstAddrMode)
    {
    case NO_PANID_ADDR:
        macHdr.SetDstAddrMode(params.m_dstAddrMode);
        macHdr.SetNoPanIdComp();
        break;
    case ADDR_MODE_RESERVED:
        NS_ABORT_MSG("Can not set destination address type to ADDR_MODE_RESERVED. Aborting.");
        break;
    case SHORT_ADDR:
        macHdr.SetDstAddrMode(params.m_dstAddrMode);
        macHdr.SetDstAddrFields(params.m_dstPanId, params.m_dstAddr);
        break;
    case EXT_ADDR:
        macHdr.SetDstAddrMode(params.m_dstAddrMode);
        macHdr.SetDstAddrFields(params.m_dstPanId, params.m_dstExtAddr);
        break;
    default:
        NS_LOG_ERROR(this << " Can not send packet with incorrect Destination Address mode = "
                          << params.m_dstAddrMode);
        confirmParams.m_status = MacStatus::INVALID_ADDRESS;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }

    // IEEE 802.15.4-2006 (7.5.6.1)
    // Src & Dst PANs are identical, PAN compression is ON
    // only the dst PAN is serialized making the MAC header 2 bytes smaller
    if ((params.m_dstAddrMode != NO_PANID_ADDR && params.m_srcAddrMode != NO_PANID_ADDR) &&
        (macHdr.GetDstPanId() == macHdr.GetSrcPanId()))
    {
        macHdr.SetPanIdComp();
    }

    macHdr.SetSecDisable();
    // extract the first 3 bits in TxOptions
    int b0 = params.m_txOptions & TX_OPTION_ACK;
    int b1 = params.m_txOptions & TX_OPTION_GTS;
    int b2 = params.m_txOptions & TX_OPTION_INDIRECT;

    if (b0 == TX_OPTION_ACK)
    {
        // Set AckReq bit only if the destination is not the broadcast address.
        if (macHdr.GetDstAddrMode() == SHORT_ADDR)
        {
            // short address and ACK requested.
            Mac16Address shortAddr = macHdr.GetShortDstAddr();
            if (shortAddr.IsBroadcast() || shortAddr.IsMulticast())
            {
                NS_LOG_LOGIC("LrWpanMac::McpsDataRequest: requested an ACK on broadcast or "
                             "multicast destination ("
                             << shortAddr << ") - forcefully removing it.");
                macHdr.SetNoAckReq();
                params.m_txOptions &= ~uint8_t(TX_OPTION_ACK);
            }
            else
            {
                macHdr.SetAckReq();
            }
        }
        else
        {
            // other address (not short) and ACK requested
            macHdr.SetAckReq();
        }
    }
    else
    {
        macHdr.SetNoAckReq();
    }

    // NOTE: RIT mode does not support GTS or INDIRECT transmission.
    if (b1 == TX_OPTION_GTS || b2 == TX_OPTION_INDIRECT)
    {
        NS_LOG_ERROR(this << " GTS or INDIRECT transmission not supported in RIT mode");

        confirmParams.m_status = MacStatus::INVALID_PARAMETER;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }

    /*
    * NOTE [EXPERIMENTAL / DISABLED]:
    * The RIT sub-header was experimentally used to carry control flags
    * (e.g., continuous transmission indication).
    * It is currently disabled to keep the packet format minimal and
    * preserve backward compatibility with existing evaluation scripts.
    * This block is intentionally kept for future experimental extensions.
    */
    // RitSubHeader ritSubHdr;
    // if (m_moduleConfig.continuousTxEnabled && m_txQueue.size() > 1)
    // {
    //     ritSubHdr.SetContinuous(true);
    // }
    // else
    // {
    //     ritSubHdr.SetContinuous(false);
    // }
    // p->AddHeader(ritSubHdr);

    // RIT Direct Tx
    // RIT direct transmission:
    // In RIT mode, packets are enqueued and transmitted only after receiving a valid
    // RIT Data Request (beacon) from the intended receiver.
    // RIT does not rely on beacon-enabled synchronization and is expected to use
    // unslotted CSMA/CA or module-specific channel access logic.
    p->AddHeader(macHdr);

    LrWpanMacTrailer macTrailer;
    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(p);
    }
    p->AddTrailer(macTrailer);

    // *Just enqueue the packet, DO NOT send immediately*
    Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
    txQElement->txQMsduHandle = params.m_msduHandle;
    txQElement->txQPkt = p;
    EnqueueTxQElement(txQElement);

    if (m_ritMacMode == SLEEP_MODE)
    {
        CheckTxAndStartSender();
    }
}

void
RitWpanMac::PdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi)
{
    NS_ASSERT(m_macState == MAC_IDLE || m_macState == MAC_ACK_PENDING || m_macState == MAC_CSMA);
    NS_LOG_FUNCTION(this << psduLength << p << (uint16_t)lqi);

    if (!IsRitModeEnabled())
    {
        LrWpanMac::PdDataIndication(psduLength, p, lqi);
        return;
    }

    bool acceptFrame;
    Ptr<Packet> originalPkt = p->Copy(); // because we will strip headers

    LrWpanMacTrailer receivedMacTrailer;
    p->RemoveTrailer(receivedMacTrailer);

    if (Node::ChecksumEnabled())
    {
        receivedMacTrailer.EnableFcs(true);
    }

    LrWpanMacHeader receivedMacHdr;
    p->RemoveHeader(receivedMacHdr);

    // From section 7.5.6.2 Reception and rejection, IEEE 802.15.4-2006
    // - Level 1 filtering: Test FCS field and reject if frame fails.
    // - Level 2 filtering: If promiscuous mode pass frame to higher layer
    //   otherwise perform Level 3 filtering.
    // - Level 3 filtering: Accept frame if Frame type and version is not reserved, and if
    //   there is a dstPanId then dstPanId=m_macPanId or broadcastPanId, and if there is a
    //   shortDstAddr then shortDstAddr=shortMacAddr or broadcastAddr, and if beacon frame then
    //   srcPanId = m_macPanId if only srcAddr field in Data or Command frame,accept frame if
    //   srcPanId=m_macPanId.

    // Level 1 filtering: FCS check
    if (!receivedMacTrailer.CheckFcs(p))
    {
        m_macRxDropTrace(originalPkt);
        return;
    }

    // Level 2 filtering: Promiscuous mode
    if (m_macPromiscuousMode)
    {
        // NOTE: Promiscuous trace is disabled in RIT for performance, enable if needed
        // PrintPacket(originalPkt);
        // ReceiveInPromiscuousMode(lqi, originalPkt);
        return;
    }

    // Level 3 filtering: RIT-specific reception logic
    acceptFrame = (receivedMacHdr.GetType() != LrWpanMacHeader::LRWPAN_MAC_RESERVED);

    // TODO: Support newer RIT frame versions (e.g., Frame Version 2).
    if (acceptFrame)
    {
        acceptFrame = (receivedMacHdr.GetFrameVer() <= 1);
    }

    if (acceptFrame && (receivedMacHdr.GetDstAddrMode() > 1))
    {
        // Accept frame if one of the following is true:

        // 1) Have the same macPanId
        // 2) Is Message to all PANs
        // 3) Is a command frame and the macPanId is not present
        acceptFrame = (receivedMacHdr.GetDstPanId() == m_macPanId ||
                       receivedMacHdr.GetDstPanId() == 0xffff) ||
                      (m_macPanId == 0xffff && receivedMacHdr.IsCommand());
    }

    if (acceptFrame && (receivedMacHdr.GetDstAddrMode() == SHORT_ADDR))
    {
        if (receivedMacHdr.GetShortDstAddr() == m_shortAddress)
        {
            // unicast, for me
            acceptFrame = true;
        }
        else if ((receivedMacHdr.GetShortDstAddr().IsBroadcast() ||
                  receivedMacHdr.GetShortDstAddr().IsMulticast()) &&
                 receivedMacHdr.IsCommand())
        {
            // Broadcast or multicast only command frame.
            // Discard broadcast/multicast with the ACK bit set.
            acceptFrame = !receivedMacHdr.IsAckReq();
        }
        else
        {
            acceptFrame = false;
        }
    }

    if (acceptFrame && (receivedMacHdr.GetDstAddrMode() == EXT_ADDR))
    {
        acceptFrame = (receivedMacHdr.GetExtDstAddr() == m_macExtendedAddress);
    }

    // Check for Association Request Command in beacon-enabled CAP-based operation.
    // This logic is NOT applicable to RIT mode, but ACK-required RIT commands may be added in
    // the future.
    if (acceptFrame && receivedMacHdr.IsCommand() && receivedMacHdr.IsAckReq())
    {
        // TODO: Handle ACK-required RIT commands (if supported in the future).
        NS_LOG_ERROR("RIT mode does not support Association Request Command or ACK-required "
                    "commands in beacon-enabled CAP-based operation.");
    }

    if (!acceptFrame)
    {
        NS_LOG_DEBUG("Frame not accepted: " << "Type=" << receivedMacHdr.GetType()
                                            << ", SrcAddr=" << receivedMacHdr.GetShortSrcAddr()
                                            << ", DstAddr=" << receivedMacHdr.GetShortDstAddr());
        m_macRxDropTrace(originalPkt);
        return;
    }

    m_macRxTrace(originalPkt);
    if (receivedMacHdr.IsCommand())
    {
        ReceiveCommand(lqi, originalPkt); // Process RIT Data Request command
    }
    else if (receivedMacHdr.IsData() && m_ritMacMode == RECEIVER_MODE)
    {
        // Trace data wait end event
        m_dataWaitTrace("end", Simulator::Now());

        ReceiveData(lqi, originalPkt);

        if (receivedMacHdr.IsAckReq())
        {
            // If this is a data or mac command frame, which is not a broadcast or
            // multicast, with ack req set, generate and send an ack frame. If there is a
            // CSMA medium access in progress we cancel the medium access for sending the
            // ACK frame. A new transmission attempt will be started after the ACK was send.
            if (m_macState == MAC_ACK_PENDING)
            {
                m_ackWaitTimeout.Cancel();
                PrepareRetransmission();
            }
            else if (m_macState == MAC_CSMA)
            {
                // \todo: If we receive a packet while doing CSMA/CA, should  we drop the
                // packet because of channel busy,
                //        or should we restart CSMA/CA for the packet after sending the ACK?
                // Currently we simply restart CSMA/CA after sending the ACK.
                NS_LOG_DEBUG("Received a packet with ACK required while in CSMA. Cancel "
                             "current CSMA-CA");
                m_preCsB->Cancel();
                m_preCs->Cancel();
                m_csmaCa->Cancel();
            }
            // Cancel any pending MAC state change, ACKs have higher priority.
            m_setMacState.Cancel();
            ChangeMacState(MAC_IDLE);

            // save received packet and LQI to process the appropriate indication/response
            // after sending ACK (PD-DATA.confirm)
            m_rxPkt = originalPkt->Copy();
            m_lastRxFrameLqi = lqi;

            /*
            * NOTE [EXPERIMENTAL / DISABLED]:
            * Reception-side handling of the RIT sub-header was experimentally implemented
            * to inspect control flags such as continuous transmission indication.
            *
            * This logic is currently disabled to keep the receive path aligned with
            * the minimal packet format used in the evaluation.
            * The code is preserved for future experimental extensions and debugging.
            */
            // RitSubHeader ritSubHdr;
            // p->RemoveHeader(ritSubHdr);
            // if (ritSubHdr.isContinuous())
            // {
            //     m_continuousRxEnabled = true; // Enable continuous reception
            // }

            m_setMacState =
                Simulator::ScheduleNow(&LrWpanMac::SendAck, this, receivedMacHdr.GetSeqNum());

            // extend Receiver Timeout
            m_ritDataWaitTimeout.Cancel();
            m_ritDataWaitTimeout = Simulator::Schedule(GetRitDataWaitDurationTime(),
                                                       &RitWpanMac::ReceiverCycleTimeout,
                                                       this);
            return;
        }
        // else if (m_continuousRxEnabled)
        // {
        //     return;
        // }
        else
        {
            EndReceiverCycle(); // Set the MAC state to sleep mode after data reception
        }
    }
    else if (receivedMacHdr.IsMultipurpose())
    {
        NS_ASSERT(m_moduleConfig.beaconAckEnabled);
        if (m_ritMacMode == RECEIVER_MODE)
        {
            NS_LOG_DEBUG("Received multipurpose frame, extend the data wait time.");
            m_ritDataWaitTimeout.Cancel(); // Cancel the current data wait timeout
            // TODO M: set current extend data wait time
            m_ritDataWaitTimeout = Simulator::Schedule(GetContinuousTxTimeoutTime(),
                                                       &RitWpanMac::ReceiverCycleTimeout,
                                                       this);
        }
    }
    else if (receivedMacHdr.IsAcknowledgment() && m_txPkt && m_macState == MAC_ACK_PENDING)
    {
        LrWpanMacHeader peekedMacHdr;
        m_txPkt->PeekHeader(peekedMacHdr);
        // If it is an ACK with the expected sequence number, finish the transmission
        if (receivedMacHdr.GetSeqNum() == peekedMacHdr.GetSeqNum())
        {
            NS_LOG_DEBUG("Ack received");
            m_ritSending = false;      // Clear the sending flag
            m_ackWaitTimeout.Cancel(); // Cancel the ACK wait timeout
            m_macTxOkTrace(m_txPkt);

            if (!m_mcpsDataConfirmCallback.IsNull())
            {
                // Notify the upper layer about the successful data transmission
                McpsDataConfirmParams confirmParams;
                confirmParams.m_status = MacStatus::SUCCESS;
                confirmParams.m_msduHandle = m_txQueue.front()->txQMsduHandle;
                m_mcpsDataConfirmCallback(confirmParams);
            }

            m_setMacState.Cancel();
            m_setMacState = Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
            // if (m_moduleConfig.continuousTxEnabled && m_txQueue.size() > 0)
            // {
            //     // TODO: Consider IFS handling before sending the next packet.
            //     NS_LOG_DEBUG("RIT continuous transmission enabled, waiting for next packet.");
            //     DoSendRitData();
            //     return;
            // }
            RemoveFirstTxQElement();
            EndSenderCycle();
        }
        else
        {
            // TODO: check mac sequence
            // WARN: i don't care about mac sequence is correct or not.
            // LrWpanMac::PdDataIndication(psduLength, originalPkt, lqi);
        }
    }
}

void
RitWpanMac::PdDataConfirm(PhyEnumeration status)
{
    NS_ASSERT(m_macState == MAC_SENDING);
    NS_LOG_FUNCTION(this << status);

    if (!IsRitModeEnabled())
    {
        LrWpanMac::PdDataConfirm(status);
        return;
    }

    LrWpanMacHeader macHdr;
    Time ifsWaitTime;

    // NOTE: symbolRate can be used to compute IFS durations if needed.
    // symbolRate = m_phy->GetDataOrSymbolRate(false); // symbols per second

    m_txPkt->PeekHeader(macHdr);

    if (status == IEEE_802_15_4_PHY_SUCCESS)
    {
        if (!macHdr.IsAcknowledgment())
        {
            if (macHdr.IsCommand())
            {
                Ptr<Packet> txOriginalPkt = m_txPkt->Copy();
                LrWpanMacHeader txMacHdr;
                txOriginalPkt->RemoveHeader(txMacHdr);
                CommandPayloadHeader txMacPayload;
                txOriginalPkt->RemoveHeader(txMacPayload);

                if (txMacPayload.GetCommandFrameType() == CommandPayloadHeader::RIT_DATA_REQ)
                {
                    // NS_ASSERT(m_csmaCa->IsUnSlottedCsmaCa());
                    NS_LOG_DEBUG("RIT request command transmitted successfully.");

                    // Trace data wait start event.
                    m_dataWaitTrace("start", Simulator::Now());

                    // TODO: Adjust behavior depending on whether RIT-LE is used.
                    StartRitDataWaitPeriod(); // Start the RIT data wait period.
                    m_lastDataTxStartTime = Simulator::Now();
                }
                else
                {
                    NS_LOG_DEBUG(
                        "error! RIT request command not sent, but PdDataConfirm called with "
                        "status SUCCESS."
                        << txMacPayload.GetCommandFrameType());
                }
            }
            else if (macHdr.IsData())
            {
                // Handle successful data transmission.
                if (macHdr.IsAckReq())
                {
                    NS_LOG_DEBUG("RIT data transmission completed successfully, waiting for ACK.");
                    Time waitTime = Seconds(static_cast<double>(GetMacAckWaitDuration()) /
                                            m_phy->GetDataOrSymbolRate(false));
                    NS_ASSERT(m_ackWaitTimeout.IsExpired());
                    m_ackWaitTimeout =
                        Simulator::Schedule(waitTime, &RitWpanMac::AckWaitTimeout, this);
                    m_setMacState.Cancel();
                    m_setMacState = Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState,
                                                           this,
                                                           MAC_ACK_PENDING);
                    NS_LOG_DEBUG("end ack wait timeout scheduled");
                    return;
                }
                else
                {
                    NS_LOG_DEBUG("RIT data transmission completed successfully (no ACK required).");
                    m_ritSending = false; // Clear the sending flag.
                    m_macTxOkTrace(m_txPkt);

                    if (!m_mcpsDataConfirmCallback.IsNull())
                    {
                        // Notify the upper layer about the successful data transmission.
                        McpsDataConfirmParams confirmParams;
                        confirmParams.m_status = MacStatus::SUCCESS;
                        confirmParams.m_msduHandle = m_txQueue.front()->txQMsduHandle;
                        m_mcpsDataConfirmCallback(confirmParams);
                    }

                    if (m_moduleConfig.continuousTxEnabled && m_txQueue.size() > 0)
                    {
                        NS_LOG_DEBUG(
                            "RIT continuous transmission enabled, waiting for next packet.");
                        // TODO: Consider IFS handling before sending the next packet.
                        DoSendRitData();
                        return;
                    }

                    // Handle RIT data transmission completion.
                    NS_ASSERT(m_ritTxWaitTimeout.IsExpired());
                    RemoveFirstTxQElement(); // Remove the first element from the Tx queue.
                    EndSenderCycle();
                }
            }
            else if (macHdr.IsMultipurpose())
            {
                NS_ASSERT(m_ritMacMode == SENDER_MODE && m_moduleConfig.beaconAckEnabled);
                // NOTE: Placeholder IFS wait time for multipurpose frames.
                ifsWaitTime = NanoSeconds(1);
            }
            else
            {
                NS_LOG_ERROR(
                    "Received unexpected frame type in PdDataConfirm: " << macHdr.GetType());
            }
        }
        else
        {
            // Handle ACK transmission success.
            // Clear the packet buffer for the ACK packet sent.
            m_txPkt = nullptr;

            // RIT module: continuous transmission handling.
            if (m_moduleConfig.continuousTxEnabled && m_continuousRxEnabled)
            {
                m_ritDataWaitTimeout = Simulator::Schedule(GetContinuousTxTimeoutTime(),
                                                           &RitWpanMac::ReceiverCycleTimeout,
                                                           this);
                return;
            }

            // End the receiver cycle after successfully transmitting an ACK.
            EndReceiverCycle();
        }
    }
    else
    {
        NS_LOG_DEBUG("RIT data transmission failed with status: " << status);
        m_macTxDropTrace(m_txPkt);
        m_txPkt = nullptr; // Clear the packet buffer.
    }

    if (!ifsWaitTime.IsZero())
    {
        m_ifsEvent =
            Simulator::Schedule(ifsWaitTime, &RitWpanMac::IfsWaitTimeout, this, ifsWaitTime);
    }

    m_setMacState.Cancel();
    m_setMacState = Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
}

void
RitWpanMac::PlmeSetTRXStateConfirm(PhyEnumeration status)
{
    NS_LOG_FUNCTION(this << status);

    if (!IsRitModeEnabled())
    {
        LrWpanMac::PlmeSetTRXStateConfirm(status);
        return;
    }

    // Handle RIT-specific PLME-SET-TRX-STATE.confirm.
    NS_LOG_DEBUG("RIT mode is enabled, handling PlmeSetTRXStateConfirm in RitWpanMac.");

    if (m_macState == MAC_IDLE &&
        (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
        // NOTE: No action is required here in the current RIT implementation.
        // CheckQueue();
        return;
    }
    else if (m_macState == MAC_CSMA &&
             (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
        // Start CSMA-related processing as soon as the receiver is enabled.
        LrWpanMacHeader macHdr;
        m_txPkt->PeekHeader(macHdr);

        if ((macHdr.IsCommand() && m_moduleConfig.beaconPreCsEnabled) ||
            (macHdr.IsData() && m_moduleConfig.dataPreCsEnabled))
        {
            NS_LOG_DEBUG("Start Pre-CS");
            m_preCs->Start();
            return;
        }
    }

    // Fall back to the base LrWpanMac implementation.
    LrWpanMac::PlmeSetTRXStateConfirm(status);
}

/**
 * Handle MLME-SET.request for RIT-specific PIB-like attributes.
 *
 * RIT-specific parameters are exposed as PIB-like attributes so that they can
 * be configured through the standard MLME-SET interface used by ns-3
 * LrWpanMac, preserving consistency with IEEE 802.15.4 MAC management.
 *
 * Attributes in the experimental/vendor-specific range (id >= 0xF0) are
 * interpreted as RIT parameters and handled locally. Other attributes are
 * forwarded to the base LrWpanMac implementation.
 *
 * NOTE:
 * Although a separate convenience API may be provided for fast parameter
 * configuration in experimental code, this MLME-based path is intentionally
 * kept to maintain standard-compliant control, reproducibility, and
 * compatibility with existing ns-3 MAC configuration workflows.
 */
void
RitWpanMac::MlmeSetRequest(MacPibAttributeIdentifier id, Ptr<MacPibAttributes> attribute)
{
    MlmeSetConfirmParams confirmParams;
    confirmParams.m_status = MacStatus::SUCCESS;

    // RIT-specific attributes are assigned in a vendor/experimental range (id >= 0xF0).
    // Only handle RIT PIB-like attributes.
    if (id >= 0xF0)
    {
        if (id == static_cast<MacPibAttributeIdentifier>(macRitPeriod))
        {
            // Apply the new value first, then start/stop the RIT cycle accordingly.
            m_macRitPeriod = attribute->macRitPeriod;

            if (m_macRitPeriod == 0u)
            {
                StopRitCycle();
            }
            else if (m_ritMacMode == RIT_MODE_DISABLED)
            {
                StartRitCycle();
            }
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitDataWaitDuration))
        {
            m_macRitDataWaitDuration = attribute->macRitDataWaitDuration;
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitTxWaitDuration))
        {
            m_macRitTxWaitDuration = attribute->macRitTxWaitDuration;
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitRequestPayload))
        {
            m_macRitRequestPayload = attribute->macRitRequestPayload;
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitPeriodTime))
        {
            // Apply the new value first, then start/stop the RIT cycle accordingly.
            m_macRitPeriodTime = attribute->macRitPeriodTime;

            if (m_macRitPeriodTime.Get().IsZero())
            {
                NS_LOG_DEBUG("RIT period time set to zero, stopping RIT cycle.");
                StopRitCycle();
            }
            else if (m_ritMacMode == RIT_MODE_DISABLED)
            {
                StartRitCycle();
            }
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitDataWaitDurationTime))
        {
            m_macRitDataWaitDurationTime = attribute->macRitDataWaitDurationTime;
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitTxWaitDurationTime))
        {
            m_macRitTxWaitDurationTime = attribute->macRitTxWaitDurationTime;
        }
        else
        {
            confirmParams.m_status = MacStatus::UNSUPPORTED_ATTRIBUTE;
        }

        if (!m_mlmeSetConfirmCallback.IsNull())
        {
            confirmParams.id = id;
            m_mlmeSetConfirmCallback(confirmParams);
        }
    }
    else
    {
        LrWpanMac::MlmeSetRequest(id, attribute);
    }
}

/**
 * Handle MLME-GET.request for RIT-specific PIB-like attributes.
 *
 * This method exposes internal RIT parameters through the standard MLME-GET
 * interface used by ns-3 LrWpanMac, in order to keep the configuration and
 * inspection path consistent with IEEE 802.15.4-style MAC management.
 *
 * RIT-related attributes are assigned to a vendor/experimental identifier
 * range (id >= 0xF0) and are handled locally. All other attributes are
 * delegated to the base LrWpanMac implementation.
 *
 * Note:
 * For experimental use, higher-level or convenience configuration APIs may
 * exist, but this function is intentionally kept for standard-compliant access
 * and debugging purposes.
 */
void
RitWpanMac::MlmeGetRequest(MacPibAttributeIdentifier id)
{
    NS_LOG_FUNCTION(this << id);
    MacStatus status = MacStatus::SUCCESS;
    Ptr<MacPibAttributes> attribute = Create<MacPibAttributes>();

    // Only handle RIT-specific PIB-like attributes.
    if (id >= 0xF0)
    {
        if (id == static_cast<MacPibAttributeIdentifier>(macRitPeriod))
        {
            attribute->macRitPeriod = m_macRitPeriod;
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitDataWaitDuration))
        {
            attribute->macRitDataWaitDuration = m_macRitDataWaitDuration;
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitTxWaitDuration))
        {
            attribute->macRitTxWaitDuration = m_macRitTxWaitDuration;
        }
        else if (id == static_cast<MacPibAttributeIdentifier>(macRitRequestPayload))
        {
            attribute->macRitRequestPayload = m_macRitRequestPayload;
        }
        else
        {
            status = MacStatus::UNSUPPORTED_ATTRIBUTE;
        }

        if (!m_mlmeGetConfirmCallback.IsNull())
        {
            m_mlmeGetConfirmCallback(status, id, attribute);
        }
    }
    else
    {
        // Delegate non-RIT attributes to the base LrWpanMac implementation.
        LrWpanMac::MlmeGetRequest(id);
        return;
    }
}

void
RitWpanMac::IfsWaitTimeout(Time ifsTime)
{
    // TODO: Introduce RIT-specific IFS handling if protocol extensions require it.
    NS_LOG_FUNCTION(this << ifsTime);

    if (!IsRitModeEnabled())
    {
        LrWpanMac::IfsWaitTimeout(ifsTime);
    }

    auto symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false);
    Time lifsTime = Seconds((double)m_macLIFSPeriod / symbolRate);
    Time sifsTime = Seconds((double)m_macSIFSPeriod / symbolRate);

    if (ifsTime == lifsTime)
    {
        NS_LOG_DEBUG("LIFS of " << m_macLIFSPeriod << " symbols (" << ifsTime.As(Time::S)
                                << ") completed ");
    }
    else if (ifsTime == sifsTime)
    {
        NS_LOG_DEBUG("SIFS of " << m_macSIFSPeriod << " symbols (" << ifsTime.As(Time::S)
                                << ") completed ");
    }
    else
    {
        NS_LOG_DEBUG("Unknown IFS size (" << ifsTime.As(Time::S) << ") completed ");
    }

    m_macIfsEndTrace(ifsTime);

    if (m_ritMacMode == SENDER_MODE)
    {
        NS_LOG_DEBUG("RIT continuous transmission or beacon ACK enabled; sending next packet.");
        NS_ASSERT((m_moduleConfig.continuousTxEnabled || m_moduleConfig.beaconAckEnabled) &&
                  m_txQueue.size() > 0);
        DoSendRitData(); // Immediately transmit the next RIT data frame
        return;
    }
    else if (m_ritMacMode == SLEEP_MODE)
    {
        // Opportunistically start sender mode if packets are queued.
        CheckTxAndStartSender();
        return;
    }

    // Fall back to the base LrWpanMac behavior for non-RIT cases.
    LrWpanMac::IfsWaitTimeout(ifsTime);
}

void
RitWpanMac::SetLrWpanMacState(MacState macState)
{
    NS_LOG_FUNCTION(this << macState);

    if (!IsRitModeEnabled())
    {
        // Fall back to LrWpanMac if RIT mode is not enabled.
        LrWpanMac::SetLrWpanMacState(macState);
        return;
    }

    if (m_macState == MAC_CSMA && macState == CHANNEL_ACCESS_FAILURE)
    {
        NS_ASSERT(m_txPkt);

        // A clear channel could not be found; drop the current packet and perform
        // RIT-specific recovery depending on the frame type.
        NS_LOG_DEBUG(this << " cannot find clear channel");

        m_macTxDropTrace(m_txPkt);

        Ptr<Packet> pkt = m_txPkt->Copy();
        LrWpanMacHeader macHdr;
        pkt->RemoveHeader(macHdr);

        if (macHdr.IsData())
        {
            NS_LOG_DEBUG("RIT data packet dropped due to channel access failure.");

            if (!m_mcpsDataConfirmCallback.IsNull())
            {
                McpsDataConfirmParams confirmParams;
                confirmParams.m_msduHandle = m_txQueue.front()->txQMsduHandle;
                confirmParams.m_status = MacStatus::CHANNEL_ACCESS_FAILURE;
                m_mcpsDataConfirmCallback(confirmParams);
            }

            // Remove the queued packet corresponding to the failed transmission.
            RemoveFirstTxQElement();

            // End the sender cycle after data transmission failure.
            EndSenderCycle();
        }
        else if (macHdr.IsCommand())
        {
            CommandPayloadHeader cmdPayload;
            pkt->RemoveHeader(cmdPayload);

            switch (cmdPayload.GetCommandFrameType())
            {
            case CommandPayloadHeader::RIT_DATA_REQ:
                NS_LOG_DEBUG("RIT Beacon CSMA failed, End Receiver Cycle.");
                EndReceiverCycle();
                break;

            case CommandPayloadHeader::RIT_DATA_RES:
                // TODO: Handle RIT Data Response command on channel access failure.
                NS_LOG_DEBUG("RIT Data Response command received, but not implemented yet.");
                break;

            default:
                NS_LOG_ERROR("Unknown command frame type in RitWpanMac::SetLrWpanMacState: "
                             << static_cast<int>(cmdPayload.GetCommandFrameType()));
                break;
            }
        }
    }
    else
    {
        // Fall back to LrWpanMac for other state transitions.
        LrWpanMac::SetLrWpanMacState(macState);
    }
}

// ------------------------------------------------------------
// Here begin the RIT-specific processing functions.
// ------------------------------------------------------------

void
RitWpanMac::PeriodicRitDataRequest()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());
    NS_ASSERT(!m_periodicRitDataRequestEvent.IsPending());
    NS_LOG_DEBUG("Periodic RIT data request initiated.");

    // Schedule the next beacon transmission
    Time ritPeriodTime = GetRitPeriodTime();
    ritPeriodTime = m_clockDriftApplier->Apply(ritPeriodTime); // Apply clock drift correction

    // *module* RI-MAC beacon interval randomization (x0.5 ~ x1.5)
    if (m_timeDriftApplier && m_moduleConfig.beaconRandomizeEnabled)
    {
        ritPeriodTime = m_timeDriftApplier->ApplyByRatio(ritPeriodTime, 50.0);
        NS_LOG_DEBUG("[RIT Module] Beacon interval randomized: "
                     << ritPeriodTime.As(Time::S) << " seconds.");
    }

    m_periodicRitDataRequestEvent =
        Simulator::Schedule(ritPeriodTime, &RitWpanMac::PeriodicRitDataRequest, this);

    // Skip beacon transmission while operating in sender mode
    if (m_ritMacMode == SENDER_MODE)
    {
        NS_LOG_DEBUG("Currently in SENDER MODE, skipping RIT data request.");
    }
    else
    {
        if (CheckTxAndStartSender())
        {
            // If a packet is queued, switch to sender mode instead of transmitting a beacon
            return;
        }

        // In receiver mode, transmit the RIT data request as usual
        ChangeRitMacMode(RECEIVER_MODE);
        DoSendRitDataRequest();
    }
}

void
RitWpanMac::DoSendRitDataRequest()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());
    NS_ASSERT_MSG(m_macState == MAC_IDLE,
                  "RIT Data Request can only be sent when MAC is in IDLE state. Now macState is "
                      << m_macState);

    // Build the packet for the RIT Data Request command.
    // If no payload is configured, transmit an empty command payload.
    Ptr<Packet> ritDataRequestPacket = Create<Packet>();
    if (m_macRitRequestPayload.empty())
    {
        ritDataRequestPacket = Create<Packet>();
    }
    else
    {
        ritDataRequestPacket =
            Create<Packet>(m_macRitRequestPayload.data(), m_macRitRequestPayload.size());
    }

    // Build the MAC header for the RIT Data Request command.
    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_COMMAND, m_macDsn.GetValue());
    m_macDsn++;
    macHdr.SetFrameVer(1);

    // *module* Compact RIT Data Request:
    // Use a minimized header (source only) to reduce overhead.
    if (m_moduleConfig.compactRitDataRequestEnabled)
    {
        // Compact header for RIT Data Request (source address only).
        macHdr.SetSrcAddrMode(SHORT_ADDR);
        macHdr.SetSrcAddrFields(GetPanId(), GetShortAddress());
        macHdr.SetDstAddrMode(NO_PANID_ADDR);
        macHdr.SetPanIdComp();
        macHdr.SetSecDisable();
    }
    else
    {
        // Standard header for RIT Data Request (broadcast destination).
        macHdr.SetSrcAddrMode(SHORT_ADDR);
        macHdr.SetSrcAddrFields(GetPanId(), GetShortAddress());
        macHdr.SetDstAddrMode(SHORT_ADDR);
        macHdr.SetDstAddrFields(GetPanId(), Mac16Address("FF:FF"));
        macHdr.SetNoPanIdComp();
        macHdr.SetSecDisable();
    }

    // Beacon ACK is handled as a separate multipurpose frame in this implementation.
    // Therefore, the RIT Data Request command itself does not request an ACK.
    macHdr.SetNoAckReq();

    CommandPayloadHeader ritCmdHdr(CommandPayloadHeader::RIT_DATA_REQ);
    ritDataRequestPacket->AddHeader(ritCmdHdr);
    ritDataRequestPacket->AddHeader(macHdr);

    // Append FCS if ChecksumEnabled is set globally.
    LrWpanMacTrailer macTrailer;
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(ritDataRequestPacket);
    }
    ritDataRequestPacket->AddTrailer(macTrailer);

    // Transmit the beacon either with CSMA/CA (or Pre-CS variants), or directly.
    if (m_moduleConfig.beaconCsmaEnabled || m_moduleConfig.beaconPreCsEnabled ||
        m_moduleConfig.beaconPreCsBEnabled)
    {
        NS_LOG_DEBUG("RIT beacon transmission with Unslotted CSMA/CA");

        if (m_macState == MAC_IDLE && !m_setMacState.IsPending())
        {
            NS_ASSERT(m_csmaCa->IsUnSlottedCsmaCa());
            if (!m_ifsEvent.IsPending())
            {
                m_txPkt = ritDataRequestPacket;
                m_setMacState =
                    Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState, this, MAC_CSMA);
            }
        }
    }
    else
    {
        NS_LOG_DEBUG("RIT beacon transmission NO CSMA/CA");
        m_txPkt = ritDataRequestPacket;
        ChangeMacState(MAC_SENDING);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
    }
}

void
RitWpanMac::SendRitData()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled() && m_ritMacMode == SENDER_MODE);
    NS_ASSERT(m_txQueue.size() > 0);

    if (m_macState == MAC_IDLE)
    {
        // Trace: beacon-wait period ended (a valid trigger to attempt transmission).
        m_beaconWaitTrace("end", Simulator::Now());
        m_ritSending = true;

        if (m_moduleConfig.beaconAckEnabled)
        {
            NS_LOG_DEBUG("RIT beacon ACK enabled; sending Beacon ACK (multipurpose frame) first.");
            DoSendRitBeaconAck();
            return;
        }

        // Transmit the queued data frame (destination is set based on the last received RIT request).
        DoSendRitData();
    }
    else
    {
        NS_LOG_DEBUG("RIT MAC is busy; cannot send RIT data now. macState=" << m_macState);
        // Trace: send attempt skipped due to MAC busy.
        m_beaconWaitTrace("skip", Simulator::Now());
    }
}

void
RitWpanMac::DoSendRitData()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled() && m_ritMacMode == SENDER_MODE);
    NS_ASSERT(m_txQueue.size() > 0);

    // Update the destination address of the head-of-line data frame to the sender of the
    // most recently received RIT Data Request (i.e., the current intended receiver).
    Ptr<TxQueueElement> txQElement = m_txQueue.front();
    Ptr<Packet> pkt = txQElement->txQPkt->Copy();

    LrWpanMacHeader macHdr;
    pkt->RemoveHeader(macHdr);
    macHdr.SetDstAddrMode(SHORT_ADDR);
    macHdr.SetDstAddrFields(GetPanId(), m_lastRxRitReqFrameSrcAddr);
    pkt->AddHeader(macHdr);

    txQElement->txQPkt = pkt;

    NS_LOG_DEBUG("RIT data request command from " << m_lastRxRitReqFrameSrcAddr);
    NS_LOG_DEBUG("DoSendRitData: payload size=" << txQElement->txQPkt->GetSize() << " bytes | "
                                               << "src=" << macHdr.GetShortSrcAddr() << " | "
                                               << "dst=" << macHdr.GetShortDstAddr());

    // Transmit the data either with CSMA/CA (or Pre-CS variants), or directly.
    if (m_moduleConfig.dataCsmaEnabled || m_moduleConfig.dataPreCsEnabled ||
        m_moduleConfig.dataPreCsBEnabled)
    {
        NS_ASSERT_MSG(
            !(m_moduleConfig.dataCsmaEnabled && m_moduleConfig.dataPreCsEnabled),
            "Only one of dataCsmaEnabled or dataPreCsEnabled can be true at the same time.");

        NS_LOG_DEBUG("RIT data transmission with Unslotted CSMA/CA");
        CheckQueue();
    }
    else
    {
        NS_LOG_DEBUG("RIT data transmission NO CSMA/CA");
        m_txPkt = txQElement->txQPkt;
        ChangeMacState(MAC_SENDING);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
    }
}

void
RitWpanMac::DoSendRitBeaconAck()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());
    NS_ASSERT_MSG(m_macState == MAC_IDLE,
                  "RIT Beacon ACK can only be sent when MAC is in IDLE state. Now macState is "
                      << m_macState);

    // Create an empty packet for the Beacon ACK (multipurpose frame).
    Ptr<Packet> ritBeaconAckPacket = Create<Packet>();

    // Build the MAC header for the Beacon ACK (multipurpose frame).
    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_MULTIPURPOSE, m_macDsn.GetValue());
    m_macDsn++;
    macHdr.SetFrameVer(1);

    // Use a compact header:
    // - no source PAN/address field
    // - unicast destination set to the sender of the last RIT Data Request
    macHdr.SetSrcAddrMode(NO_PANID_ADDR);
    macHdr.SetDstAddrMode(SHORT_ADDR);
    macHdr.SetDstAddrFields(GetPanId(), m_lastRxRitReqFrameSrcAddr);
    macHdr.SetPanIdComp();
    macHdr.SetSecDisable();

    // Beacon ACK itself does not request an ACK.
    macHdr.SetNoAckReq();
    ritBeaconAckPacket->AddHeader(macHdr);

    // Append FCS if ChecksumEnabled is set globally.
    LrWpanMacTrailer macTrailer;
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(ritBeaconAckPacket);
    }
    ritBeaconAckPacket->AddTrailer(macTrailer);

    NS_LOG_DEBUG("RIT beacon ACK transmission without CSMA/CA");

    m_txPkt = ritBeaconAckPacket;
    ChangeMacState(MAC_SENDING);
    m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
}

void
RitWpanMac::ReceiveCommand(uint8_t lqi, Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << lqi << p);

    NS_LOG_DEBUG("RIT command frame received; processing...");
    LrWpanMacHeader receivedMacHdr;
    p->RemoveHeader(receivedMacHdr);

    // Peek the command type first, then strip it only when we actually handle it.
    CommandPayloadHeader peekedPayload;
    p->PeekHeader(peekedPayload);

    switch (peekedPayload.GetCommandFrameType())
    {
    case CommandPayloadHeader::RIT_DATA_REQ:
    {
        if (m_ritMacMode == SENDER_MODE)
        {
            // Ignore a new request while we are already in the middle of sending.
            if (m_ritSending)
            {
                NS_LOG_DEBUG("RIT_DATA_REQ received in SENDER_MODE, but already sending; ignored.");
                break;
            }

            // A valid RIT request arrived: cancel the sender wait timeout and prepare for data TX.
            m_ritTxWaitTimeout.Cancel();

            CommandPayloadHeader receivedRitPayload;
            p->RemoveHeader(receivedRitPayload);

            MlmeRitRequestIndicationParams ritReqParams;

            ritReqParams.m_srcAddrMode = receivedMacHdr.GetSrcAddrMode();
            ritReqParams.m_srcPanId = receivedMacHdr.GetSrcPanId();
            ritReqParams.m_srcAddr = receivedMacHdr.GetShortSrcAddr();
            ritReqParams.m_srcExtAddr = receivedMacHdr.GetExtSrcAddr();

            // Used by DoSendRitData() to set the unicast destination.
            m_lastRxRitReqFrameSrcAddr = receivedMacHdr.GetShortSrcAddr();

            ritReqParams.m_dstAddrMode = receivedMacHdr.GetDstAddrMode();
            ritReqParams.m_dstPanId = receivedMacHdr.GetDstPanId();
            ritReqParams.m_dstAddr = receivedMacHdr.GetShortDstAddr();
            ritReqParams.m_dstExtAddr = receivedMacHdr.GetExtDstAddr();

            std::vector<uint8_t> payload(p->GetSize());
            p->CopyData(payload.data(), payload.size());
            ritReqParams.m_ritRequestPayload = payload;

            ritReqParams.m_linkQuality = lqi;
            ritReqParams.m_dsn = receivedMacHdr.GetSeqNum();

            // Timestamp is exported in symbols (16 us per symbol at 2.4 GHz O-QPSK).
            ritReqParams.m_timestamp = Simulator::Now().GetMicroSeconds() / 16;

            // Security-related fields are currently not populated for RIT extensions.
            // TODO: Fill ritReqParams with security information if/when enabled.

            if (!m_mlmeRitRequestIndicationCallback.IsNull())
            {
                NS_LOG_DEBUG("Invoking MLME-RIT-REQ.indication callback.");
                m_mlmeRitRequestIndicationCallback(ritReqParams);
            }
            else
            {
                NS_LOG_DEBUG("MLME-RIT-REQ.indication callback is not set; request ignored.");
            }
        }
        else if (m_ritMacMode == RECEIVER_MODE)
        {
            // Receiving RIT_DATA_REQ while being a receiver is unexpected in this implementation.
            NS_LOG_DEBUG("RIT_DATA_REQ received in RECEIVER_MODE; not handled (unexpected).");
        }
        else if (m_ritMacMode == BOOTSTRAP_MODE)
        {
            // TODO: Define bootstrap behavior for RIT commands if needed.
        }
        else
        {
            NS_LOG_ERROR("RIT_DATA_REQ received in an invalid RIT mode: "
                         << static_cast<int>(m_ritMacMode));
        }
        break;
    }

    case CommandPayloadHeader::RIT_DATA_RES:
    {
        // TODO: Handle RIT data response command if/when implemented.
        NS_LOG_DEBUG("RIT_DATA_RES received, but not implemented yet.");
        break;
    }

    default:
        break;
    }
}

void
RitWpanMac::ReceiveData(uint8_t lqi, Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << lqi << p);

    LrWpanMacTrailer receivedMacTrailer;
    p->RemoveTrailer(receivedMacTrailer);

    LrWpanMacHeader receivedMacHdr;
    p->RemoveHeader(receivedMacHdr);

    /*
     * NOTE [EXPERIMENTAL / DISABLED]:
     * Reception-side handling of the RIT sub-header was experimentally implemented
     * to inspect control flags (e.g., continuous transmission indication).
     *
     * This logic is currently disabled to keep the receive path aligned with the
     * minimal packet format used in the evaluation.
     * The code is preserved for future experimental extensions and debugging.
     */
    // RitSubHeader ritSubHdr;
    // p->RemoveHeader(ritSubHdr);

    NS_LOG_DEBUG("Data packet for this node; forwarding up. dst="
                 << receivedMacHdr.GetShortDstAddr() << " self=" << m_shortAddress
                 << " src=" << receivedMacHdr.GetShortSrcAddr());

    if (m_ritMacMode == SENDER_MODE)
    {
        NS_LOG_WARN("Data received in SENDER_MODE; ignoring (possible fast mode switch).");
        return;
    }

    if (!m_mcpsDataIndicationCallback.IsNull())
    {
        McpsDataIndicationParams params;
        params.m_dsn = receivedMacHdr.GetSeqNum();
        params.m_mpduLinkQuality = lqi;

        params.m_srcPanId = receivedMacHdr.GetSrcPanId();
        params.m_srcAddrMode = receivedMacHdr.GetSrcAddrMode();
        switch (params.m_srcAddrMode)
        {
        case SHORT_ADDR:
            params.m_srcAddr = receivedMacHdr.GetShortSrcAddr();
            break;
        case EXT_ADDR:
            params.m_srcExtAddr = receivedMacHdr.GetExtSrcAddr();
            break;
        default:
            break;
        }

        params.m_dstPanId = receivedMacHdr.GetDstPanId();
        params.m_dstAddrMode = receivedMacHdr.GetDstAddrMode();
        switch (params.m_dstAddrMode)
        {
        case SHORT_ADDR:
            params.m_dstAddr = receivedMacHdr.GetShortDstAddr();
            break;
        case EXT_ADDR:
            params.m_dstExtAddr = receivedMacHdr.GetExtDstAddr();
            break;
        default:
            break;
        }

        m_mcpsDataIndicationCallback(params, p);
    }
}

void
RitWpanMac::StartRitDataWaitPeriod()
{
    NS_ASSERT(IsRitModeEnabled() && m_ritMacMode == RECEIVER_MODE);
    NS_LOG_FUNCTION(this);

    // Keep the receiver on during the data-wait window after transmitting the RIT request.
    // The MAC is forced to IDLE so that incoming frames can be processed immediately.
    SetRxOnWhenIdle(true);
    SetLrWpanMacState(MAC_IDLE);

    const bool hasValidWait =
        (m_useTimeBasedRitParams && (m_macRitDataWaitDurationTime > Seconds(0))) ||
        (!m_useTimeBasedRitParams && (m_macRitDataWaitDuration > 0));

    if (!hasValidWait)
    {
        NS_LOG_ERROR("Invalid RIT data wait duration; cannot start data wait period. "
                     "useTimeBased=" << m_useTimeBasedRitParams << " waitTime="
                     << (m_useTimeBasedRitParams
                             ? m_macRitDataWaitDurationTime.Get().GetSeconds()
                             : static_cast<double>(m_macRitDataWaitDuration)));
        return;
    }

    const Time dataWaitTime = GetRitDataWaitDurationTime();
    NS_ASSERT(m_ritDataWaitTimeout.IsExpired());
    m_ritDataWaitTimeout =
        Simulator::Schedule(dataWaitTime, &RitWpanMac::ReceiverCycleTimeout, this);
}

void
RitWpanMac::StartRitTxWaitPeriod()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled() && m_ritMacMode == SENDER_MODE);

    // Mark the start of the sender-side beacon-wait phase.
    // This trace records when the sender enters a wait window in which RX is kept on.
    m_beaconWaitTrace("start", Simulator::Now());

    SetRxOnWhenIdle(true);
    SetLrWpanMacState(MAC_IDLE);

    const bool hasValidWait =
        (m_useTimeBasedRitParams && (m_macRitTxWaitDurationTime > Seconds(0))) ||
        (!m_useTimeBasedRitParams && (m_macRitTxWaitDuration > 0u));

    if (!hasValidWait)
    {
        // Keep behavior: do not schedule a timeout when the duration is invalid/zero.
        // (This matches the original control flow.)
        return;
    }

    const Time txWaitTime = GetRitTxWaitDurationTime();
    NS_ASSERT(m_ritTxWaitTimeout.IsExpired());
    m_ritTxWaitTimeout =
        Simulator::Schedule(txWaitTime, &RitWpanMac::SenderCycleTimeout, this);
}

void
RitWpanMac::SenderCycleTimeout()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled() && m_ritMacMode == SENDER_MODE);

    // Record that the sender-side beacon-wait window has timed out.
    m_beaconWaitTrace("timeout", Simulator::Now());

    // End the current sender cycle (cleanup + transition to sleep).
    EndSenderCycle();
}

void
RitWpanMac::EndSenderCycle()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());
    NS_ASSERT(m_ritMacMode == SENDER_MODE);

    // Cancel any remaining sender-side wait timer (if still pending).
    if (!m_ritTxWaitTimeout.IsExpired())
    {
        m_ritTxWaitTimeout.Cancel();
    }

    // Clear the "currently sending" guard for the sender cycle.
    m_ritSending = false;

    // Transition to sleep (PHY forced off unless rxAlwaysOn is enabled).
    SetSleep();
}


void
RitWpanMac::ReceiverCycleTimeout()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled() && m_ritMacMode == RECEIVER_MODE);
    m_dataWaitTrace("timeout", Simulator::Now());
    EndReceiverCycle(); // End the receiver cycle if the timeout occurs
}

void
RitWpanMac::EndReceiverCycle()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());
    NS_ASSERT_MSG(m_ritMacMode == RECEIVER_MODE,
                  "RIT MAC mode must be RECEIVER_MODE to end the receiver cycle. Now mode is "
                      << m_ritMacMode << this);

    // Cancel the receiver-side data wait timer if it is still pending.
    if (m_ritDataWaitTimeout.IsPending())
    {
        NS_LOG_DEBUG("End Rx Data, end RIT receiver cycle.");
        m_ritDataWaitTimeout.Cancel();
    }

    // Transition to sleep (PHY forced off unless rxAlwaysOn is enabled).
    SetSleep();
}

void
RitWpanMac::AckWaitTimeout()
{
    NS_LOG_FUNCTION(this);

    if (!IsRitModeEnabled())
    {
        LrWpanMac::AckWaitTimeout();
        return;
    }

    // In RIT mode, ACK wait timeout ends the sender cycle (the pending packet is handled
    // by the base MAC timeout path, then we switch back to sleep).
    NS_LOG_DEBUG("ACK wait timeout, ending RIT sender cycle.");

    if (m_ritMacMode != SENDER_MODE)
    {
        NS_LOG_ERROR("ACK wait timeout occurred in an invalid RIT mode: " << m_ritMacMode);
        return;
    }

    // Clear the sender-cycle guard before leaving the cycle.
    m_ritSending = false;

    // Delegate base-class ACK timeout handling (e.g., traces/state cleanup).
    LrWpanMac::AckWaitTimeout();

    // Finally, end the sender cycle and go to sleep.
    EndSenderCycle();
}

void
RitWpanMac::StartRitCycle()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());

    // Sanity check: the RIT period must be >= the receiver-side data wait duration.
    // Use the effective values (time-based or duration-based), not the raw time members.
    const Time period = GetRitPeriodTime();
    const Time dataWait = GetRitDataWaitDurationTime();
    NS_ASSERT_MSG(period >= dataWait,
                  "RIT period time must be greater than or equal to RIT data wait duration time. "
                      << period.GetSeconds() << " >= " << dataWait.GetSeconds());

    NS_LOG_DEBUG("Starting RIT cycle with period: " << period.As(Time::S) << " seconds.");

    // Only start scheduling when the base MAC is idle; otherwise defer (future work).
    if (m_macState != MAC_IDLE)
    {
        // TODO: Defer starting the RIT cycle until ongoing operations settle.
        return;
    }

    // Start the periodic RIT Data Request (beacon) scheduler.
    NS_ASSERT(!m_periodicRitDataRequestEvent.IsPending());

    // Enter sleep between periodic wakeups (unless rxAlwaysOn is enabled elsewhere).
    ChangeRitMacMode(SLEEP_MODE);
    SetRxOnWhenIdle(false);

    // Randomize the initial phase to avoid starting all nodes at the same instant.
    Ptr<UniformRandomVariable> initialDelay = CreateObject<UniformRandomVariable>();
    const double delaySec = initialDelay->GetValue(0.0, period.GetSeconds());

    m_periodicRitDataRequestEvent =
        Simulator::Schedule(Seconds(delaySec), &RitWpanMac::PeriodicRitDataRequest, this);
}

void
RitWpanMac::StopRitCycle()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());
    NS_LOG_DEBUG("Stopping RIT cycle.");

    // Stop periodic scheduling and any ongoing sender/receiver wait windows.
    // This is the symmetric counterpart of StartRitCycle(), and it forcefully returns
    // the RIT MAC to the disabled state.
    m_periodicRitDataRequestEvent.Cancel();
    m_ritDataWaitTimeout.Cancel();
    m_ritTxWaitTimeout.Cancel();

    // Clear RIT mode and leave the base MAC in a safe idle state.
    ChangeRitMacMode(RIT_MODE_DISABLED);
    SetLrWpanMacState(MAC_IDLE);

    // TODO: If StopRitCycle() is called while the MAC is busy, you may want to defer stopping
    // until ongoing operations settle. For now, we stop immediately to match the user's intent.
}

void
RitWpanMac::SetSleep()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());

    // RIT sleep transition should occur only when there is no active sender/receiver wait window.
    NS_ASSERT(m_ritDataWaitTimeout.IsExpired() && m_ritTxWaitTimeout.IsExpired());

    // Keep the base MAC state consistent before touching the PHY.
    ChangeMacState(MAC_IDLE);

    // If the user requested always-on RX, do not power down the radio.
    if (m_rxAlwaysOn)
    {
        NS_LOG_DEBUG("RX always-on is enabled; skipping PHY sleep transition.");
        return;
    }

    // Enter the RIT sleep mode and force the transceiver off.
    ChangeRitMacMode(SLEEP_MODE);
    m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_FORCE_TRX_OFF);
    SetRxOnWhenIdle(false);
}

bool
RitWpanMac::CheckTxAndStartSender()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsRitModeEnabled());

    if (m_txQueue.empty())
    {
        return false; // No packets to transmit.
    }

    // If there is at least one packet queued, switch to sender mode and open
    // the sender-side TX wait window (beacon wait window).
    NS_ASSERT(m_ritMacMode != SENDER_MODE);
    NS_LOG_DEBUG("tx queue size: " << static_cast<int>(m_txQueue.size()));

    ChangeRitMacMode(SENDER_MODE);
    StartRitTxWaitPeriod();
    return true;
}

void
RitWpanMac::ChangeRitMacMode(RitMacMode newMode)
{
    // Avoid redundant state transitions. This also prevents duplicate logs/traces
    // when the mode is already set to the requested value.
    if (m_ritMacMode == newMode)
    {
        NS_LOG_LOGIC(this << " RIT MAC mode unchanged: " << m_ritMacMode);
        return;
    }

    NS_LOG_LOGIC(this << " change RIT MAC mode from " << m_ritMacMode << " to " << newMode);
    m_ritMacMode = newMode;
}

Time
RitWpanMac::DurationToTime(uint64_t duration) const
{
    // Convert a RIT "duration" value (in units of aBaseSuperframeDuration) into a Time.
    // This path is valid only when the PIB parameters are used.
    NS_ASSERT(!m_useTimeBasedRitParams);

    const uint64_t symbols = duration * aBaseSuperframeDuration;
    const double symbolRate = m_phy->GetDataOrSymbolRate(false); // symbols per second
    return Seconds(static_cast<double>(symbols) / symbolRate);
}

bool
RitWpanMac::IsRitModeEnabled() const
{
    // RIT is considered enabled when the configured period is positive.
    // - Time-based mode: enabled iff macRitPeriodTime > 0
    // - PIB mode : enabled iff macRitPeriod > 0
    if (m_useTimeBasedRitParams)
    {
        return m_macRitPeriodTime.Get().IsPositive();
    }
    return m_macRitPeriod > 0u;
}

void
RitWpanMac::SetPreCs(Ptr<RitWpanPreCs> preCs)
{
    // Inject the Pre-CS module implementation.
    // The actual start/cancel decision is made in the TX path depending on the module flags.
    NS_LOG_FUNCTION(this << preCs);
    m_preCs = preCs;
}

void
RitWpanMac::SetRxAlwaysOn(bool alwaysOn)
{
    NS_LOG_FUNCTION(this << alwaysOn);
    // Configure whether the receiver should stay enabled even when the MAC is idle.
    m_rxAlwaysOn = alwaysOn;
}

// Return the effective RIT period as a Time value.
// Depending on the configuration, this is either taken directly from the
// time-based parameter or converted from the legacy duration-based value.
Time
RitWpanMac::GetRitPeriodTime() const
{
    if (m_useTimeBasedRitParams)
    {
        return m_macRitPeriodTime;
    }
    else
    {
        return DurationToTime(m_macRitPeriod);
    }
}

Time
RitWpanMac::GetRitDataWaitDurationTime() const
{
    Time dataWaitDurationTime;

    if (m_useTimeBasedRitParams)
    {
        dataWaitDurationTime = m_macRitDataWaitDurationTime.Get();
    }
    else
    {
        dataWaitDurationTime = DurationToTime(m_macRitDataWaitDuration);
    }

    // TODO: Consider dynamic adjustment of the receiver-side data wait duration.
    return dataWaitDurationTime;
}

Time
RitWpanMac::GetRitTxWaitDurationTime() const
{
    Time txWaitDurationTime;

    if (m_useTimeBasedRitParams)
    {
        txWaitDurationTime = m_macRitTxWaitDurationTime;
    }
    else
    {
        txWaitDurationTime = DurationToTime(m_macRitTxWaitDuration);
    }

    // TODO: Consider dynamic adjustment of the sender-side beacon wait duration.
    return txWaitDurationTime;
}

Time
RitWpanMac::GetContinuousTxTimeoutTime() const
{
    // TODO: Accurately derive the extended data-reception wait time for continuous transmission.
    //       This may depend on the last data TX duration or PHY parameters.
    // return m_lastDataTxDuration;
    return MilliSeconds(10);
}

void
RitWpanMac::SetModuleConfig(const RitWpanMacModuleConfig& config)
{
    // Apply the RIT MAC module configuration (feature flags and behavior switches).
    m_moduleConfig = config;
}

RitWpanMacModuleConfig
RitWpanMac::GetModuleConfig() const
{
    // Return the current RIT MAC module configuration.
    return m_moduleConfig;
}

void
RitWpanMac::SetMlmeRitRequestIndicationCallback(MlmeRitRequestIndicationCallback cb)
{
    NS_LOG_FUNCTION(this << &cb);
    // Register the MLME-RIT-REQ.indication callback.
    // This callback is invoked when a valid RIT Data Request command is received.
    m_mlmeRitRequestIndicationCallback = cb;
}

void
RitWpanMac::SetRitModuleConfig(const RitWpanMacModuleConfig& config)
{
    NS_LOG_FUNCTION(this);
    // Set the RIT MAC module configuration (feature flags and behavior options).
    // This overwrites the current configuration without changing ongoing state.
    m_moduleConfig = config;
}


} // namespace lrwpan
} // namespace ns3
