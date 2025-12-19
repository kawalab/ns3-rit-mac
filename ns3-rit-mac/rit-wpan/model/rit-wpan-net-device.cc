/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "rit-wpan-net-device.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-error-model.h"
#include "ns3/node.h"

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("RitWpanNetDevice");
NS_OBJECT_ENSURE_REGISTERED(RitWpanNetDevice);

TypeId
RitWpanNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RitWpanNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("LrWpan")
            .AddConstructor<RitWpanNetDevice>()
            .AddAttribute("Channel",
                          "The channel attached to this device",
                          PointerValue(),
                          MakePointerAccessor(&RitWpanNetDevice::DoGetChannel),
                          MakePointerChecker<SpectrumChannel>())
            .AddAttribute("Phy",
                          "The PHY layer attached to this device.",
                          PointerValue(),
                          MakePointerAccessor(&RitWpanNetDevice::GetPhy, &RitWpanNetDevice::SetPhy),
                          MakePointerChecker<LrWpanPhy>())
            .AddAttribute("Mac",
                          "The MAC layer attached to this device.",
                          PointerValue(),
                          MakePointerAccessor(&RitWpanNetDevice::GetMac, &RitWpanNetDevice::SetMac),
                          MakePointerChecker<RitWpanMac>())
            .AddAttribute("Nwk",
                          "The NWK layer attached to this device.",
                          PointerValue(),
                          MakePointerAccessor(&RitWpanNetDevice::GetNwk, &RitWpanNetDevice::SetNwk),
                          MakePointerChecker<RitSimpleRouting>());
    return tid;
}

RitWpanNetDevice::RitWpanNetDevice()
{
    NS_LOG_FUNCTION_NOARGS();

    m_rank = 0;
    m_configComplete = false;
    m_receiveCallback.Nullify();

    // Construct stack objects (default).
    m_phy = CreateObject<LrWpanPhy>();
    m_mac = CreateObject<RitWpanMac>();
    m_nwk = CreateObject<RitSimpleRouting>();

    m_csmaca = CreateObject<LrWpanCsmaCa>();
    m_precs = CreateObject<RitWpanPreCs>();
    m_precsb = CreateObject<RitWpanPreCsB>();

    m_channel = nullptr;
    m_node = nullptr;
}

RitWpanNetDevice::~RitWpanNetDevice()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
RitWpanNetDevice::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();

    m_phy->Dispose();
    m_mac->Dispose();
    m_nwk->Dispose();
    m_csmaca->Dispose();
    m_precs->Dispose();
    m_precsb->Dispose();

    m_phy = nullptr;
    m_mac = nullptr;
    m_nwk = nullptr;
    m_csmaca = nullptr;
    m_precs = nullptr;
    m_precsb = nullptr;

    m_channel = nullptr;
    m_node = nullptr;

    NetDevice::DoDispose();
}

void
RitWpanNetDevice::DoInitialize()
{
    NS_LOG_FUNCTION_NOARGS();

    m_phy->Initialize();
    m_mac->Initialize();

    CompleteConfig();

    NetDevice::DoInitialize();
}

void
RitWpanNetDevice::CompleteConfig()
{
    NS_LOG_FUNCTION_NOARGS();

    // Apply wiring only once, and only after all components exist.
    if (m_configComplete)
    {
        return;
    }
    if (!m_node || !m_phy || !m_mac || !m_nwk || !m_csmaca || !m_precs || !m_precsb)
    {
        return;
    }

    // --- Layer registration / wiring ---
    m_nwk->SetMac(m_mac);

    m_mac->SetPhy(m_phy);
    m_mac->SetCsmaCa(m_csmaca);
    m_mac->SetPreCs(m_precs);
    m_mac->SetPreCsB(m_precsb);

    m_csmaca->SetMac(m_mac);
    m_precs->SetMac(m_mac);
    m_precsb->SetMac(m_mac);

    // PHY error model + device back-pointer.
    Ptr<LrWpanErrorModel> model = CreateObject<LrWpanErrorModel>();
    m_phy->SetErrorModel(model);
    m_phy->SetDevice(this);

    // Rank propagation (NetDevice -> NWK).
    m_nwk->SetRank(m_rank);

    // --- Callback wiring ---
    // NWK -> NetDevice -> upper layer.
    m_nwk->SetNwkRxCallback(MakeCallback(&RitWpanNetDevice::OnNwkReceive, this));

    // MAC callbacks to NWK.
    m_mac->SetMlmeRitRequestIndicationCallback(
        MakeCallback(&RitSimpleRouting::MlmeRitRequestIndication, m_nwk));
    m_mac->SetMcpsDataIndicationCallback(
        MakeCallback(&RitSimpleRouting::McpsDataIndication, m_nwk));
    m_mac->SetMcpsDataConfirmCallback(MakeCallback(&RitSimpleRouting::McpsDataConfirm, m_nwk));

    // PHY callbacks to MAC.
    m_phy->SetPdDataIndicationCallback(MakeCallback(&RitWpanMac::PdDataIndication, m_mac));
    m_phy->SetPdDataConfirmCallback(MakeCallback(&RitWpanMac::PdDataConfirm, m_mac));
    m_phy->SetPlmeGetAttributeConfirmCallback(
        MakeCallback(&RitWpanMac::PlmeGetAttributeConfirm, m_mac));
    m_phy->SetPlmeSetTRXStateConfirmCallback(
        MakeCallback(&RitWpanMac::PlmeSetTRXStateConfirm, m_mac));
    m_phy->SetPlmeSetAttributeConfirmCallback(
        MakeCallback(&RitWpanMac::PlmeSetAttributeConfirm, m_mac));

    // Carrier sense chain (PreCsB -> PreCs -> CSMA/CA).
    m_phy->SetPlmeCcaConfirmCallback(MakeCallback(&RitWpanPreCsB::PlmeCcaConfirm, m_precsb));
    m_precsb->SetFallbackCcaConfirmCallback(MakeCallback(&RitWpanPreCs::PlmeCcaConfirm, m_precs));
    m_precs->SetFallbackCcaConfirmCallback(MakeCallback(&LrWpanCsmaCa::PlmeCcaConfirm, m_csmaca));

    // State callbacks back to MAC.
    m_csmaca->SetLrWpanMacStateCallback(MakeCallback(&RitWpanMac::SetLrWpanMacState, m_mac));
    m_precs->SetLrWpanMacStateCallback(MakeCallback(&RitWpanMac::SetLrWpanMacState, m_mac));
    m_precsb->SetLrWpanMacStateCallback(MakeCallback(&RitWpanMac::SetLrWpanMacState, m_mac));

    // --- Apply RIT PIB parameters (stored in this NetDevice) ---
    Ptr<MacPibAttributes> pibAttr = Create<MacPibAttributes>();

    pibAttr->macRitDataWaitDurationTime = m_macRitDataWaitDuration;
    m_mac->MlmeSetRequest(macRitDataWaitDurationTime, pibAttr);

    pibAttr->macRitTxWaitDurationTime = m_macRitTxWaitDuration;
    m_mac->MlmeSetRequest(macRitTxWaitDurationTime, pibAttr);

    pibAttr->macRitPeriodTime = m_macRitPeriod;
    m_mac->MlmeSetRequest(macRitPeriodTime, pibAttr);

    // Module config (CSMA / Pre-CS / Pre-CSB / ACK / Randomization / etc.)
    m_mac->SetModuleConfig(m_moduleConfig);

    m_configComplete = true;
}

void
RitWpanNetDevice::SetNwk(Ptr<RitSimpleRouting> nwk)
{
    NS_LOG_FUNCTION(this);
    m_nwk = nwk;
    CompleteConfig();
}

void
RitWpanNetDevice::SetMac(Ptr<RitWpanMac> mac)
{
    NS_LOG_FUNCTION(this);
    m_mac = mac;
    CompleteConfig();
}

void
RitWpanNetDevice::SetPhy(Ptr<LrWpanPhy> phy)
{
    NS_LOG_FUNCTION(this);
    m_phy = phy;
    CompleteConfig();
}

void
RitWpanNetDevice::SetCsmaCa(Ptr<LrWpanCsmaCa> csmaca)
{
    NS_LOG_FUNCTION(this);
    m_csmaca = csmaca;
    CompleteConfig();
}

void
RitWpanNetDevice::SetChannel(Ptr<SpectrumChannel> channel)
{
    NS_LOG_FUNCTION(this);
    m_channel = channel;
    m_phy->SetChannel(channel);
    channel->AddRx(m_phy);
}

void
RitWpanNetDevice::SetRitRank(uint8_t rank)
{
    NS_LOG_FUNCTION(this);
    m_rank = rank;

    if (m_nwk)
    {
        m_nwk->SetRank(rank);
    }
    else
    {
        // (should not happen with default construction)
        NS_LOG_WARN("SetRitRank called before NWK is available; rank will be applied later.");
    }
}

uint8_t
RitWpanNetDevice::GetRitRank() const
{
    return m_rank;
}

void
RitWpanNetDevice::SetMacRitPeriod(Time macRitPeriod)
{
    m_macRitPeriod = macRitPeriod;
}

void
RitWpanNetDevice::SetMacRitDataWaitDuration(Time macRitDataWaitDuration)
{
    m_macRitDataWaitDuration = macRitDataWaitDuration;
}

void
RitWpanNetDevice::SetMacRitTxWaitDuration(Time macRitTxWaitDuration)
{
    m_macRitTxWaitDuration = macRitTxWaitDuration;
}

void
RitWpanNetDevice::SetRitModuleConfig(const RitWpanMacModuleConfig& config)
{
    NS_LOG_FUNCTION(this);

    // Mutual exclusion checks (same as your original intent).
    int dataTxModes = 0;
    dataTxModes += config.dataCsmaEnabled ? 1 : 0;
    dataTxModes += config.dataPreCsEnabled ? 1 : 0;
    dataTxModes += config.dataPreCsBEnabled ? 1 : 0;
    if (dataTxModes > 1)
    {
        NS_FATAL_ERROR("Invalid module config: only one of dataCsmaEnabled, dataPreCsEnabled, "
                       "or dataPreCsBEnabled can be true.");
    }

    int beaconTxModes = 0;
    beaconTxModes += config.beaconCsmaEnabled ? 1 : 0;
    beaconTxModes += config.beaconPreCsEnabled ? 1 : 0;
    beaconTxModes += config.beaconPreCsBEnabled ? 1 : 0;
    if (beaconTxModes > 1)
    {
        NS_FATAL_ERROR("Invalid module config: only one of beaconCsmaEnabled, beaconPreCsEnabled, "
                       "or beaconPreCsBEnabled can be true.");
    }

    m_moduleConfig = config;

    // If MAC already exists, propagate immediately.
    if (m_mac)
    {
        m_mac->SetModuleConfig(config);
    }
}

Ptr<RitSimpleRouting>
RitWpanNetDevice::GetNwk() const
{
    NS_LOG_FUNCTION(this);
    return m_nwk;
}

Ptr<RitWpanMac>
RitWpanNetDevice::GetMac() const
{
    NS_LOG_FUNCTION(this);
    return m_mac;
}

Ptr<LrWpanPhy>
RitWpanNetDevice::GetPhy() const
{
    NS_LOG_FUNCTION(this);
    return m_phy;
}

Ptr<LrWpanCsmaCa>
RitWpanNetDevice::GetCsmaCa() const
{
    NS_LOG_FUNCTION(this);
    return m_csmaca;
}

Ptr<Channel>
RitWpanNetDevice::GetChannel() const
{
    NS_LOG_FUNCTION(this);
    return m_phy->GetChannel();
}

Ptr<SpectrumChannel>
RitWpanNetDevice::DoGetChannel() const
{
    NS_LOG_FUNCTION(this);
    return m_phy->GetChannel();
}

void
RitWpanNetDevice::Send(Ptr<Packet> packet, Mac16Address m16DstAddr)
{
    NS_LOG_FUNCTION(this << packet);

    // Non-IP: delegate to NWK layer.
    m_nwk->SendRequest(packet, m16DstAddr);
}

bool
RitWpanNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    (void)protocolNumber; // unused for non-IP device
    Send(packet, Mac16Address::ConvertFrom(dest));
    return true;
}

void
RitWpanNetDevice::SetReceiveCallback(ReceiveCallback cb)
{
    NS_LOG_FUNCTION_NOARGS();
    m_receiveCallback = cb;
}

void
RitWpanNetDevice::SetNode(Ptr<Node> node)
{
    m_node = node;
    CompleteConfig();
}

Ptr<Node>
RitWpanNetDevice::GetNode() const
{
    return m_node;
}

void
RitWpanNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this);
    if (Mac16Address::IsMatchingType(address))
    {
        const auto a = Mac16Address::ConvertFrom(address);
        m_mac->SetShortAddress(a);
        m_nwk->SetShortAddress(a);
    }
}

Address
RitWpanNetDevice::GetAddress() const
{
    NS_LOG_FUNCTION(this);
    // Your original returns empty Address(). Keep it to preserve behavior.
    return Address();
}

void
RitWpanNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    (void)index; // non-IP device: keep no ifIndex state (same behavior)
}

uint32_t
RitWpanNetDevice::GetIfIndex() const
{
    NS_LOG_FUNCTION(this);
    // Your original returns 0. Keep it to preserve behavior.
    return 0;
}

bool
RitWpanNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION(this);
    return m_phy != nullptr;
}

void
RitWpanNetDevice::McpsDataIndication(McpsDataIndicationParams params, Ptr<Packet> pkt)
{
    NS_LOG_FUNCTION_NOARGS();

    // This path is not used by your current wiring (NWK handles it),
    // but keep it as-is for compatibility.
    if (params.m_dstAddrMode == SHORT_ADDR)
    {
        m_receiveCallback(this, pkt, 0, params.m_srcAddr);
    }
}

void
RitWpanNetDevice::OnNwkReceive(Ptr<Packet> packet, const Mac16Address& srcAddr)
{
    NS_LOG_FUNCTION(this << packet << srcAddr);
    NS_LOG_DEBUG("[RIT-NWK] Received packet from " << srcAddr);

    // Forward to upper layer (Application's ReceiveCallback is installed on NetDevice).
    m_receiveCallback(this, packet, 0, srcAddr);
}

} // namespace lrwpan
} // namespace ns3
