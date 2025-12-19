/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef RIT_WPAN_NET_DEVICE_H
#define RIT_WPAN_NET_DEVICE_H

#include "rit-wpan-mac.h"
#include "rit-wpan-nwk.h"
#include "rit-wpan-precs.h"
#include "rit-wpan-precsb.h"

#include <ns3/lr-wpan-csmaca.h>
#include <ns3/lr-wpan-phy.h>
#include <ns3/net-device.h>
#include <ns3/simulator.h>
#include <ns3/spectrum-channel.h>

namespace ns3
{
namespace lrwpan
{

/**
 * \ingroup ri-pan
 *
 * \brief NetDevice implementation for RIT-based IEEE 802.15.4 networks.
 *
 * RitWpanNetDevice integrates PHY, RIT-MAC, and a lightweight
 * rank-based network layer into a single NetDevice abstraction.
 *
 * This device is designed for experimental evaluation of
 * receiver-initiated (RIT) MAC protocols and simple
 * rank-based data collection routing in low-power wireless sensor networks.
 *
 * \note This NetDevice does not support IP, ARP, or SendFrom().
 */
class RitWpanNetDevice : public NetDevice
{
  public:
    static TypeId GetTypeId();

    RitWpanNetDevice();
    ~RitWpanNetDevice() override;

    /* ---- Layer setters ---- */
    void SetNwk(Ptr<RitSimpleRouting> nwk);
    void SetMac(Ptr<RitWpanMac> mac);
    void SetPhy(Ptr<LrWpanPhy> phy);
    void SetCsmaCa(Ptr<LrWpanCsmaCa> csmaca);
    void SetChannel(Ptr<SpectrumChannel> channel);

    /* ---- RIT-specific configuration ---- */
    void SetRitRank(uint8_t rank);
    void SetMacRitPeriod(Time macRitPeriod);
    void SetMacRitDataWaitDuration(Time macRitDataWaitDuration);
    void SetMacRitTxWaitDuration(Time macRitTxWaitDuration);
    void SetRitModuleConfig(const RitWpanMacModuleConfig& config);

    /* ---- Layer getters ---- */
    Ptr<RitSimpleRouting> GetNwk() const;
    Ptr<RitWpanMac> GetMac() const;
    Ptr<LrWpanPhy> GetPhy() const;
    Ptr<LrWpanCsmaCa> GetCsmaCa() const;

    Ptr<Channel> GetChannel() const override;
    uint8_t GetRitRank() const;

    /* ---- Packet transmission ---- */
    void Send(Ptr<Packet> packet, Mac16Address dst);
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;

    /* ---- NetDevice API (used) ---- */
    void SetReceiveCallback(ReceiveCallback cb) override;
    void SetNode(Ptr<Node> node) override;
    Ptr<Node> GetNode() const override;
    void SetAddress(Address address) override;
    void SetAddress(uint32_t address);
    Address GetAddress() const override;

    bool IsLinkUp() const override;

    /* ---- NetDevice API (not supported) ---- */
    void SetIfIndex(uint32_t index) override {}
    uint32_t GetIfIndex() const override { return 0; }

    bool SetMtu(uint16_t mtu) override { return false; }
    uint16_t GetMtu() const override { return 0; }

    void AddLinkChangeCallback(Callback<void>) override {}
    bool IsBroadcast() const override { return true; }
    Address GetBroadcast() const override { return Address(); }

    bool IsMulticast() const override { return true; }
    Address GetMulticast(Ipv4Address) const override { return Address(); }
    Address GetMulticast(Ipv6Address) const override { return Address(); }

    bool IsBridge() const override { return false; }
    bool IsPointToPoint() const override { return false; }

    bool SendFrom(Ptr<Packet>, const Address&, const Address&, uint16_t) override
    {
        return false;
    }

    bool NeedsArp() const override { return false; }
    void SetPromiscReceiveCallback(PromiscReceiveCallback) override {}
    bool SupportsSendFrom() const override { return false; }

    /* ---- MAC → NetDevice entry point ---- */
    void McpsDataIndication(McpsDataIndicationParams params, Ptr<Packet> pkt);

  protected:
    void ForwardUp(Ptr<Packet>, Mac48Address, Mac48Address) override {}

  private:
    void DoDispose() override;
    void DoInitialize() override;

    Ptr<SpectrumChannel> DoGetChannel() const;
    void CompleteConfig();
    void OnNwkReceive(Ptr<Packet> packet, const Mac16Address& srcAddr);

    /* ---- Core components ---- */
    Ptr<Node> m_node;
    Ptr<LrWpanPhy> m_phy;
    Ptr<RitWpanMac> m_mac;
    Ptr<RitSimpleRouting> m_nwk;
    Ptr<LrWpanCsmaCa> m_csmaca;
    Ptr<RitWpanPreCs> m_precs;
    Ptr<RitWpanPreCsB> m_precsb;

    uint8_t m_rank;
    bool m_configComplete;

    /* ---- RIT timing parameters ---- */
    Time m_macRitPeriod;
    Time m_macRitDataWaitDuration;
    Time m_macRitTxWaitDuration;

    RitWpanMacModuleConfig m_moduleConfig;

    ReceiveCallback m_receiveCallback;
};

} // namespace lrwpan
} // namespace ns3

#endif // RIT_WPAN_NET_DEVICE_H
