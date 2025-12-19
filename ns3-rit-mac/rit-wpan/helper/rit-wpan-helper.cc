/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "rit-wpan-helper.h"

#include "periodic-sender-helper.h"

#include "ns3/names.h"
#include "ns3/periodic-sender.h"
#include "ns3/random-sender.h"
#include "ns3/rit-wpan-mac.h"
#include "ns3/rit-wpan-net-device.h"
#include "ns3/rit-wpan-nwk-header.h"
#include <ns3/log.h>
#include <ns3/lr-wpan-csmaca.h>
#include <ns3/lr-wpan-error-model.h>
#include <ns3/lr-wpan-net-device.h>
#include <ns3/mobility-model.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/single-model-spectrum-channel.h>

#include <filesystem>
#include <iostream>
#include <sstream>

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("RitWpanNetHelper");

RitWpanNetHelper::RitWpanNetHelper()
{
}

RitWpanNetHelper::~RitWpanNetHelper()
{
    m_channel->Dispose();
    m_channel = nullptr;
}

void
RitWpanNetHelper::AddMobility(Ptr<LrWpanPhy> phy, Ptr<MobilityModel> m)
{
    phy->SetMobility(m);
}

// TODO: complete implementation
NetDeviceContainer
RitWpanNetHelper::Install(NodeContainer c)
{
    if (!m_channel)
    {
        m_channel = CreateObject<SingleModelSpectrumChannel>();
        Ptr<LogDistancePropagationLossModel> propModel =
            CreateObject<LogDistancePropagationLossModel>();
        Ptr<ConstantSpeedPropagationDelayModel> delayModel =
            CreateObject<ConstantSpeedPropagationDelayModel>();
        m_channel->AddPropagationLossModel(propModel);
        m_channel->SetPropagationDelayModel(delayModel);
    }

    NetDeviceContainer devices;
    for (auto i = c.Begin(); i != c.End(); i++)
    {
        Ptr<Node> node = *i;

        Ptr<RitWpanNetDevice> netDevice = CreateObject<RitWpanNetDevice>();
        netDevice->SetChannel(m_channel);

        Ptr<RitWpanMac> ritMac = DynamicCast<RitWpanMac>(netDevice->GetMac());
        Ptr<MacPibAttributes> pibAttr = Create<MacPibAttributes>();

        ritMac->SetRxAlwaysOn(m_rxAlwaysOn);

        // RIT parameters
        // mac RIT data wait duration
        netDevice->SetMacRitPeriod(m_macRitPeriod);
        netDevice->SetMacRitDataWaitDuration(m_macRitDataWaitDuration);
        netDevice->SetMacRitTxWaitDuration(m_macRitTxWaitDuration);
        netDevice->SetRitModuleConfig(m_moduleConfig);

        node->AddDevice(netDevice);
        netDevice->SetNode(node);
        devices.Add(netDevice);
    }
    return devices;
}

Ptr<SpectrumChannel>
RitWpanNetHelper::GetChannel()
{
    return m_channel;
}

void
RitWpanNetHelper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
RitWpanNetHelper::SetChannel(std::string channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channel = channel;
}

void
RitWpanNetHelper::SetMacRitPeriod(Time macRitPeriod)
{
    m_macRitPeriod = macRitPeriod; // Beacon interval
}

void
RitWpanNetHelper::SetMacRitDataWaitDuration(Time macRitDataWaitDuration)
{
    m_macRitDataWaitDuration = macRitDataWaitDuration; // [Rx] Data wait duration after beacon
}

void
RitWpanNetHelper::SetMacRitTxWaitDuration(Time macRitTxWaitDuration)
{
    m_macRitTxWaitDuration = macRitTxWaitDuration; // [Tx] Wait duration before data transmission
}

void
RitWpanNetHelper::SetRitMacDriftRatio(double riMacDriftRatio)
{
    m_riMacDriftRatio = riMacDriftRatio; // Set drift ratio for RIT MAC
}

void
RitWpanNetHelper::SetRitMacModuleConfig(const RitWpanMacModuleConfig& config)
{
    m_moduleConfig = config; // RIT MAC module configuration
}

void
RitWpanNetHelper::SetRxAlwaysOn(bool alwaysOn)
{
    m_rxAlwaysOn = alwaysOn; // Set receiver always-on flag
}

// Hook function for MacState events
void
RitWpanNetHelper::AsciiRitWpanMacStateSink(Ptr<OutputStreamWrapper> stream,
                                           uint32_t nodeId,
                                           lrwpan::MacState oldState,
                                           lrwpan::MacState newState)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << newState << std::endl;
}

// Hook function for energy events
void
RitWpanNetHelper::AsciiRitWpanEnergySink(Ptr<OutputStreamWrapper> stream,
                                         uint32_t nodeId,
                                         double energy)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << energy << std::endl;
}

// Hook function for PHY state trace
void
RitWpanNetHelper::AsciiRitWpanPhyStateSink(Ptr<OutputStreamWrapper> stream,
                                           uint32_t nodeId,
                                           Time time,
                                           PhyEnumeration oldState,
                                           PhyEnumeration newState)
{
    *stream->GetStream() << time.GetSeconds() << "," << newState << std::endl;
}

// Automated method to trace MacState only per node
void
RitWpanNetHelper::EnableTracePerNode(
    const NodeContainer& nodes,
    const std::string& baseDir,
    const std::string& logName,
    std::function<void(Ptr<Node>, Ptr<OutputStreamWrapper>)> traceSetupFn)
{
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<Node> node = nodes.Get(i);
        uint32_t nodeId = node->GetId();
        Ptr<OutputStreamWrapper> stream = GetNodeLogStream(baseDir, nodeId, logName);
        traceSetupFn(node, stream);
    }
}

void
RitWpanNetHelper::EnableMacStateTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(nodes,
                       baseDir,
                       "mac-statelog.csv",
                       [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
                           uint32_t nodeId = node->GetId();
                           for (uint32_t j = 0; j < node->GetNDevices(); ++j)
                           {
                               Ptr<NetDevice> rawDev = node->GetDevice(j);
                               Ptr<RitWpanNetDevice> dev = DynamicCast<RitWpanNetDevice>(rawDev);
                               if (!dev)
                               {
                                   continue;
                               }
                               Ptr<RitWpanMac> mac = dev->GetMac();
                               if (!mac)
                               {
                                   continue;
                               }
                               mac->TraceConnectWithoutContext(
                                   "MacState",
                                   MakeBoundCallback(&AsciiRitWpanMacStateSink, stream, nodeId));
                           }
                       });
}

// Automated helper method to trace per-node energy consumption and depletion events
void
RitWpanNetHelper::EnableEnergyTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(nodes,
                       baseDir,
                       "energy-node.log",
                       [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
                           uint32_t nodeId = node->GetId();
                           for (uint32_t j = 0; j < node->GetNDevices(); ++j)
                           {
                               Ptr<RitWpanNetDevice> dev =
                                   DynamicCast<RitWpanNetDevice>(node->GetDevice(j));
                               if (!dev)
                               {
                                   continue;
                               }
                               dev->TraceConnectWithoutContext(
                                   "EnergyDepletion",
                                   MakeBoundCallback(&AsciiRitWpanEnergySink, stream, nodeId));
                           }
                       });
}

// Automated helper method to enable per-node PHY state tracing
void
RitWpanNetHelper::EnablePhyStateTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    NS_LOG_DEBUG("[DEBUG] Enabling PHY state trace for nodes in " << baseDir);
    EnableTracePerNode(
        nodes,
        baseDir,
        "phy-statelog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            uint32_t nodeId = node->GetId();
            for (uint32_t j = 0; j < node->GetNDevices(); ++j)
            {
                Ptr<RitWpanNetDevice> dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(j));
                if (!dev)
                {
                    // NS_LOG_DEBUG("[DEBUG] Enabling PHY state trace for nodes in " << baseDir);
                    NS_LOG_DEBUG("[DEBUG] Device " << j << " is not a RitWpanNetDevice, skipping.");
                    continue;
                }
                Ptr<LrWpanPhy> phy = dev->GetPhy();

                if (!phy)
                {
                    NS_LOG_DEBUG("[DEBUG] PHY is not set for device " << j << " in node "
                                                                      << nodeId);
                    continue;
                }
                phy->TraceConnectWithoutContext(
                    "TrxState",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanPhyStateSink, stream, nodeId));
                NS_LOG_DEBUG("[DEBUG] PHY state trace enabled for device " << j << " in node "
                                                                           << nodeId);
            }
        });
}

// Hook function for NWK-layer TX logging
void
RitWpanNetHelper::AsciiRitWpanNwkTxSink(Ptr<OutputStreamWrapper> stream,
                                        std::string event,
                                        Ptr<const Packet> pkt)
{
    RitNwkHeader hdr;
    if (!pkt->PeekHeader(hdr))
    {
        return;
    }

    std::ostringstream srcNwkStream;
    std::ostringstream dstNwkStream;

    srcNwkStream << hdr.GetSrcAddr();
    dstNwkStream << hdr.GetDstAddr();

    std::string srcNwk = srcNwkStream.str();
    std::string dstNwk = dstNwkStream.str();
    NS_LOG_DEBUG("RitWpanNetHelper::AsciiRitWpanNwkTxSink: "
                 << "time=" << Simulator::Now().GetSeconds() << ", event=" << event
                 << ", srcNwk=" << srcNwk << ", dstNwk=" << dstNwk);
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << event << "," << srcNwk << ","
                         << dstNwk << "," << pkt->GetUid() << std::endl;
}

// Hook function for MAC-layer RX logging
void
RitWpanNetHelper::AsciiRitWpanNwkRxSink(Ptr<OutputStreamWrapper> stream,
                                        std::string event,
                                        Ptr<const Packet> pkt)
{
    RitNwkHeader hdr;
    if (!pkt->PeekHeader(hdr))
    {
        return;
    }

    std::ostringstream srcNwkStream;
    std::ostringstream dstNwkStream;

    srcNwkStream << hdr.GetSrcAddr();
    dstNwkStream << hdr.GetDstAddr();

    std::string srcNwk = srcNwkStream.str();
    std::string dstNwk = dstNwkStream.str();
    NS_LOG_DEBUG("RitWpanNetHelper::AsciiRitWpanNwkRxSink: "
                 << "time=" << Simulator::Now().GetSeconds() << ", event=" << event
                 << ", srcNwk=" << srcNwk << ", dstNwk=" << dstNwk);
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << event << "," << srcNwk << ","
                         << dstNwk << "," << pkt->GetUid() << std::endl;
}

void
RitWpanNetHelper::EnableNwkTxTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "nwk-txlog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            for (uint32_t j = 0; j < node->GetNDevices(); ++j)
            {
                Ptr<RitWpanNetDevice> dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(j));
                if (!dev)
                {
                    continue;
                }
                Ptr<RitSimpleRouting> nwk = dev->GetNwk();
                if (!nwk)
                {
                    continue;
                }
                nwk->TraceConnectWithoutContext(
                    "NwkTx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanNwkTxSink, stream, "Tx"));
                nwk->TraceConnectWithoutContext(
                    "NwkTxOk",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanNwkTxSink, stream, "TxOk"));
                nwk->TraceConnectWithoutContext(
                    "NwkTxDrop",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanNwkTxSink, stream, "TxDrop"));
                nwk->TraceConnectWithoutContext(
                    "NwkReTx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanNwkTxSink, stream, "ReTx"));
            }
        });
}

void
RitWpanNetHelper::EnableNwkRxTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "nwk-rxlog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            for (uint32_t j = 0; j < node->GetNDevices(); ++j)
            {
                Ptr<RitWpanNetDevice> dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(j));
                if (!dev)
                {
                    continue;
                }
                Ptr<RitSimpleRouting> nwk = dev->GetNwk();
                if (!nwk)
                {
                    continue;
                }
                nwk->TraceConnectWithoutContext(
                    "NwkRx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanNwkRxSink, stream, "RxOk"));
                nwk->TraceConnectWithoutContext(
                    "NwkRxDrop",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanNwkRxSink, stream, "RxDrop"));
            }
        });
}

// Hook function for MAC-layer TX logging
void
RitWpanNetHelper::AsciiRitWpanMacTxSink(Ptr<OutputStreamWrapper> stream,
                                        std::string event,
                                        Ptr<const Packet> pkt)
{
    LrWpanMacHeader hdr;
    if (!pkt->PeekHeader(hdr))
    {
        return;
    }
    std::string frameType;
    switch (hdr.GetType())
    {
    case LrWpanMacHeader::LRWPAN_MAC_BEACON:
        frameType = "Beacon";
        break;
    case LrWpanMacHeader::LRWPAN_MAC_DATA:
        frameType = "Data";
        break;
    case LrWpanMacHeader::LRWPAN_MAC_ACKNOWLEDGMENT:
        frameType = "Ack";
        break;
    case LrWpanMacHeader::LRWPAN_MAC_COMMAND:
        frameType = "Command";
        break;
    case LrWpanMacHeader::LRWPAN_MAC_MULTIPURPOSE:
        frameType = "Multipurpose";
        break;
    default:
        frameType = "Unknown";
        break;
    }
    std::ostringstream srcMacStream;
    std::ostringstream dstMacStream;
    if (hdr.GetSrcAddrMode() == SHORT_ADDR)
    {
        srcMacStream << hdr.GetShortSrcAddr();
    }
    else if (hdr.GetSrcAddrMode() == EXT_ADDR)
    {
        srcMacStream << hdr.GetExtSrcAddr();
    }
    else
    {
        srcMacStream << "ff:ff";
    }
    if (hdr.GetDstAddrMode() == SHORT_ADDR)
    {
        dstMacStream << hdr.GetShortDstAddr();
    }
    else if (hdr.GetDstAddrMode() == EXT_ADDR)
    {
        dstMacStream << hdr.GetExtDstAddr();
    }
    else
    {
        dstMacStream << "ff:ff";
    }
    std::string srcMac = srcMacStream.str();
    std::string dstMac = dstMacStream.str();
    NS_LOG_DEBUG("RitWpanNetHelper::AsciiRitWpanMacTxSink: "
                 << "time=" << Simulator::Now().GetSeconds() << ", event=" << event
                 << ", frameType=" << frameType << ", srcMac=" << srcMac << ", dstMac=" << dstMac);
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << event << "," << frameType << ","
                         << srcMac << "," << dstMac << std::endl;
}

// Hook function for MAC-layer RX logging
void
RitWpanNetHelper::AsciiRitWpanMacRxSink(Ptr<OutputStreamWrapper> stream,
                                        std::string event,
                                        Ptr<const Packet> pkt)
{
    LrWpanMacHeader hdr;
    if (!pkt->PeekHeader(hdr))
    {
        return;
    }
    std::string frameType;
    switch (hdr.GetType())
    {
    case LrWpanMacHeader::LRWPAN_MAC_BEACON:
        frameType = "Beacon";
        break;
    case LrWpanMacHeader::LRWPAN_MAC_DATA:
        frameType = "Data";
        break;
    case LrWpanMacHeader::LRWPAN_MAC_ACKNOWLEDGMENT:
        frameType = "Ack";
        break;
    case LrWpanMacHeader::LRWPAN_MAC_COMMAND:
        frameType = "Command";
        break;
    case LrWpanMacHeader::LRWPAN_MAC_MULTIPURPOSE:
        frameType = "Multipurpose";
        break;
    default:
        frameType = "Unknown";
        break;
    }
    std::ostringstream srcMacStream;
    std::ostringstream dstMacStream;
    if (hdr.GetSrcAddrMode() == SHORT_ADDR)
    {
        srcMacStream << hdr.GetShortSrcAddr();
    }
    else if (hdr.GetSrcAddrMode() == EXT_ADDR)
    {
        srcMacStream << hdr.GetExtSrcAddr();
    }
    else
    {
        srcMacStream << "ff:ff";
    }
    if (hdr.GetDstAddrMode() == SHORT_ADDR)
    {
        dstMacStream << hdr.GetShortDstAddr();
    }
    else if (hdr.GetDstAddrMode() == EXT_ADDR)
    {
        dstMacStream << hdr.GetExtDstAddr();
    }
    else
    {
        dstMacStream << "ff:ff";
    }
    std::string srcMac = srcMacStream.str();
    std::string dstMac = dstMacStream.str();
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << event << "," << frameType << ","
                         << srcMac << "," << dstMac << std::endl;
}

void
RitWpanNetHelper::EnableMacTxTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "mac-txlog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            for (uint32_t j = 0; j < node->GetNDevices(); ++j)
            {
                Ptr<RitWpanNetDevice> dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(j));
                if (!dev)
                {
                    continue;
                }
                Ptr<RitWpanMac> mac = dev->GetMac();
                if (!mac)
                {
                    continue;
                }
                mac->TraceConnectWithoutContext(
                    "MacTx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanMacTxSink, stream, "Tx"));
                mac->TraceConnectWithoutContext(
                    "MacTxOk",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanMacTxSink, stream, "TxOk"));
                mac->TraceConnectWithoutContext(
                    "MacTxDrop",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanMacTxSink, stream, "TxDrop"));
            }
        });
}

void
RitWpanNetHelper::EnableMacRxTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "mac-rxlog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            for (uint32_t j = 0; j < node->GetNDevices(); ++j)
            {
                Ptr<RitWpanNetDevice> dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(j));
                if (!dev)
                {
                    continue;
                }
                Ptr<RitWpanMac> mac = dev->GetMac();
                if (!mac)
                {
                    continue;
                }
                mac->TraceConnectWithoutContext(
                    "MacRx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanMacRxSink, stream, "RxOk"));
                mac->TraceConnectWithoutContext(
                    "MacRxDrop",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanMacRxSink, stream, "RxDrop"));
            }
        });
}

// Hook function for PHY-layer TX/RX logging
void
RitWpanNetHelper::AsciiRitWpanPhyTxSink(Ptr<OutputStreamWrapper> stream,
                                        std::string event,
                                        Ptr<const Packet> pkt)
{
    LrWpanMacHeader hdr;
    if (!pkt->PeekHeader(hdr))
    {
        return;
    }
    std::ostringstream dstMacStream;
    if (hdr.GetDstAddrMode() == SHORT_ADDR)
    {
        dstMacStream << hdr.GetShortDstAddr();
    }
    else if (hdr.GetDstAddrMode() == EXT_ADDR)
    {
        dstMacStream << hdr.GetExtDstAddr();
    }
    else
    {
        dstMacStream << "ff:ff";
    }
    std::string dstMac = dstMacStream.str();
    NS_LOG_DEBUG("RitWpanNetHelper::AsciiRitWpanPhyTxSink: "
                 << "time=" << Simulator::Now().GetSeconds() << ", event=" << event
                 << ", dstMac=" << dstMac);
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << event << "," << dstMac
                         << std::endl;
}

// Hook function for PHY-layer RX logging (without SINR)）
void
RitWpanNetHelper::AsciiRitWpanPhyRxSink(Ptr<OutputStreamWrapper> stream,
                                        std::string event,
                                        Ptr<const Packet> pkt)
{
    LrWpanMacHeader hdr;
    std::ostringstream srcMacStream;
    if (pkt && pkt->PeekHeader(hdr))
    {
        if (hdr.GetSrcAddrMode() == SHORT_ADDR)
        {
            srcMacStream << hdr.GetShortSrcAddr();
        }
        else if (hdr.GetSrcAddrMode() == EXT_ADDR)
        {
            srcMacStream << hdr.GetExtSrcAddr();
        }
        else
        {
            srcMacStream << "ff:ff";
        }
    }
    std::string srcMac = srcMacStream.str();
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << event << "," << srcMac
                         << std::endl;
}

// Hook function for PHY-layer RX logging (with SINR)
void
RitWpanNetHelper::AsciiRitWpanPhyRxSink(Ptr<OutputStreamWrapper> stream,
                                        std::string event,
                                        Ptr<const Packet> pkt,
                                        double sinr)
{
    LrWpanMacHeader hdr;
    std::ostringstream srcMacStream;
    if (pkt && pkt->PeekHeader(hdr))
    {
        if (hdr.GetSrcAddrMode() == SHORT_ADDR)
        {
            srcMacStream << hdr.GetShortSrcAddr();
        }
        else if (hdr.GetSrcAddrMode() == EXT_ADDR)
        {
            srcMacStream << hdr.GetExtSrcAddr();
        }
        else
        {
            srcMacStream << "ff:ff";
        }
    }
    std::string srcMac = srcMacStream.str();
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << event << "," << srcMac << ","
                        //  << sinr
                         << std::endl;
}

void
RitWpanNetHelper::EnablePhyTxTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "phy-txlog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            for (uint32_t j = 0; j < node->GetNDevices(); ++j)
            {
                Ptr<RitWpanNetDevice> dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(j));
                if (!dev)
                {
                    continue;
                }
                Ptr<LrWpanPhy> phy = dev->GetPhy();
                if (!phy)
                {
                    continue;
                }
                phy->TraceConnectWithoutContext(
                    "PhyTxBegin",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanPhyTxSink, stream, "TxBegin"));
                phy->TraceConnectWithoutContext(
                    "PhyTxEnd",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanPhyTxSink, stream, "TxEnd"));
                phy->TraceConnectWithoutContext(
                    "PhyTxDrop",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanPhyTxSink, stream, "TxDrop"));
            }
        });
}

void
RitWpanNetHelper::EnablePhyRxTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "phy-rxlog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            for (uint32_t j = 0; j < node->GetNDevices(); ++j)
            {
                Ptr<RitWpanNetDevice> dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(j));
                if (!dev)
                {
                    continue;
                }
                Ptr<LrWpanPhy> phy = dev->GetPhy();
                if (!phy)
                {
                    continue;
                }
                phy->TraceConnectWithoutContext(
                    "PhyRxBegin",
                    MakeBoundCallback(
                        static_cast<
                            void (*)(Ptr<OutputStreamWrapper>, std::string, Ptr<const Packet>)>(
                            &RitWpanNetHelper::AsciiRitWpanPhyRxSink),
                        stream,
                        "RxBegin"));
                phy->TraceConnectWithoutContext(
                    "PhyRxEnd",
                    MakeBoundCallback(
                        static_cast<void (*)(Ptr<OutputStreamWrapper>,
                                             std::string,
                                             Ptr<const Packet>,
                                             double)>(&RitWpanNetHelper::AsciiRitWpanPhyRxSink),
                        stream,
                        "RxEnd"));
                phy->TraceConnectWithoutContext(
                    "PhyRxDrop",
                    MakeBoundCallback(
                        static_cast<
                            void (*)(Ptr<OutputStreamWrapper>, std::string, Ptr<const Packet>)>(
                            &RitWpanNetHelper::AsciiRitWpanPhyRxSink),
                        stream,
                        "RxDrop"));
            }
        });
}

void
RitWpanNetHelper::SetScenarioType(const std::string& scenarioType)
{
    m_scenarioType = scenarioType;
}

std::string
RitWpanNetHelper::GetScenarioType() const
{
    return m_scenarioType;
}

std::string
RitWpanNetHelper::GetLogBaseDir(const std::string& module,
                                uint32_t macRitPeriod,
                                uint32_t macRitTxWaitDuration,
                                uint32_t macRitDataWaitDuration,
                                uint32_t simulationTime,
                                uint32_t runNumber) const
{
    std::ostringstream oss;
    oss << "logs/" << m_scenarioType << "/" << module << "/"
        << "BI" << macRitPeriod << "_TWD" << macRitTxWaitDuration << "_DWD"
        << macRitDataWaitDuration << "_Days" << simulationTime << "/SEED" << std::setw(2)
        << std::setfill('0') << runNumber << "/";
    return oss.str();
}

// Module name utilities
std::string
RitWpanNetHelper::GetModuleShortName(const RitWpanMacModuleConfig& config) const
{
    std::vector<std::string> tags;
    // DATA CSMA/CA module
    if (config.dataCsmaEnabled && config.dataPreCsEnabled)
    {
        tags.emplace_back("csma_precs");
    }
    else if (config.dataCsmaEnabled)
    {
        tags.emplace_back("csma");
    }
    else if (config.dataPreCsEnabled)
    {
        tags.emplace_back("precs");
    }
    else if (config.dataPreCsBEnabled)
    {
        tags.emplace_back("precsb");
    }
    else
    {
        tags.emplace_back("nocsma");
    }
    // Beacon CSMA/CA module
    if (config.beaconCsmaEnabled && config.beaconPreCsEnabled)
    {
        tags.emplace_back("bcsma_bprecs");
    }
    else if (config.beaconCsmaEnabled)
    {
        tags.emplace_back("bcsma");
    }
    else if (config.beaconPreCsEnabled)
    {
        tags.emplace_back("bprecs");
    }
    else if (config.beaconPreCsBEnabled)
    {
        tags.emplace_back("bprecsb");
    }
    else
    {
        tags.emplace_back("bnocsma");
    }
    // others
    if (config.continuousTxEnabled)
    {
        tags.emplace_back("cont");
    }
    if (config.beaconRandomizeEnabled)
    {
        tags.emplace_back("random");
    }
    if (config.compactRitDataRequestEnabled)
    {
        tags.emplace_back("compact");
    }
    if (config.beaconAckEnabled)
    {
        tags.emplace_back("back");
    }
    // combine
    std::ostringstream oss;
    for (size_t i = 0; i < tags.size(); ++i)
    {
        if (i > 0)
        {
            oss << "_";
        }
        oss << tags[i];
    }
    return oss.str();
}

void
RitWpanNetHelper::EnableAllTracesPerNode(const NodeContainer& nodes,
                                         uint32_t simulationTime,
                                         uint32_t seed)
{
    // Retrieve parameters
    std::string moduleName = GetModuleShortName(m_moduleConfig);
    NS_LOG_UNCOND(moduleName);
    uint32_t macRitPeriod = m_macRitPeriod.GetMilliSeconds();
    uint32_t macRitTxWaitDuration = m_macRitTxWaitDuration.GetMilliSeconds();
    uint32_t macRitDataWaitDuration = m_macRitDataWaitDuration.GetMilliSeconds();
    uint32_t simulationDays = simulationTime;
    uint32_t runNumber = seed;
    std::string baseDir = GetLogBaseDir(moduleName,
                                        macRitPeriod,
                                        macRitTxWaitDuration,
                                        macRitDataWaitDuration,
                                        simulationDays,
                                        runNumber);
    NS_LOG_DEBUG("[DEBUG] Enabling all traces for nodes in " << baseDir);
    EnableApplicationTracePerNode(nodes, baseDir);
    EnableMacStateTracePerNode(nodes, baseDir);
    EnableMacModeTracePerNode(nodes, baseDir);
    EnableNwkTxTracePerNode(nodes, baseDir);
    EnableNwkRxTracePerNode(nodes, baseDir);
    EnableMacTxTracePerNode(nodes, baseDir);
    EnableMacRxTracePerNode(nodes, baseDir);
    EnableMacTimeoutTracePerNode(nodes, baseDir);
    EnablePhyStateTracePerNode(nodes, baseDir);
    EnablePhyTxTracePerNode(nodes, baseDir);
    EnablePhyRxTracePerNode(nodes, baseDir);
    // EnableEnergyTracePerNode(nodes, baseDir);
}

std::string
RitWpanNetHelper::GetNodeLogDir(const std::string& baseDir, uint32_t nodeId) const
{
    return baseDir + "node-" + std::to_string(nodeId) + "/";
}

std::string
RitWpanNetHelper::GetNodeLogFilePath(const std::string& baseDir,
                                     uint32_t nodeId,
                                     const std::string& logName) const
{
    std::string nodeDir = GetNodeLogDir(baseDir, nodeId);
    std::filesystem::create_directories(nodeDir);
    return nodeDir + logName;
}

Ptr<OutputStreamWrapper>
RitWpanNetHelper::GetNodeLogStream(const std::string& baseDir,
                                   uint32_t nodeId,
                                   const std::string& logName) const
{
    std::string filePath = GetNodeLogFilePath(baseDir, nodeId, logName);
    return Create<OutputStreamWrapper>(filePath, std::ios::out);
}

void
RitWpanNetHelper::AsciiApplicationTxSink(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> pkt)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << pkt->GetUid() << std::endl;
}

void
RitWpanNetHelper::AsciiApplicationRxSink(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> pkt)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << pkt->GetUid() << std::endl;
}

void
RitWpanNetHelper::EnableApplicationTracePerNode(const NodeContainer& nodes,
                                                const std::string& baseDir)
{
    EnableApplicationTxTracePerNode(nodes, baseDir);
    EnableApplicationRxTracePerNode(nodes, baseDir);
}

void
RitWpanNetHelper::EnableApplicationTxTracePerNode(const NodeContainer& nodes,
                                                  const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "app-txlog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            if (node->GetNApplications() == 0)
            {
                NS_LOG_DEBUG("No Applications found on node "
                             << node->GetId() << ". Skipping Application Tx trace setup.");
                return;
            }
            Ptr<Application> app = node->GetApplication(0);

            // Try PeriodicSender first
            Ptr<PeriodicSender> periodicSender = DynamicCast<PeriodicSender>(app);
            if (periodicSender)
            {
                periodicSender->TraceConnectWithoutContext(
                    "Tx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiApplicationTxSink, stream));
                return;
            }

            // Try RandomSender
            Ptr<RandomSender> randomSender = DynamicCast<RandomSender>(app);
            if (randomSender)
            {
                randomSender->TraceConnectWithoutContext(
                    "Tx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiApplicationTxSink, stream));
                return;
            }
        });
}

void
RitWpanNetHelper::EnableApplicationRxTracePerNode(const NodeContainer& nodes,
                                                  const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "app-rxlog.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            if (node->GetNApplications() == 0)
            {
                NS_LOG_DEBUG("No Applications found on node "
                             << node->GetId() << ". Skipping Application Rx trace setup.");
                return;
            }
            Ptr<Application> app = node->GetApplication(0);

            // Try PeriodicSender first
            Ptr<PeriodicSender> periodicSender = DynamicCast<PeriodicSender>(app);
            if (periodicSender)
            {
                periodicSender->TraceConnectWithoutContext(
                    "Rx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiApplicationRxSink, stream));
                return;
            }

            // Try RandomSender
            Ptr<RandomSender> randomSender = DynamicCast<RandomSender>(app);
            if (randomSender)
            {
                randomSender->TraceConnectWithoutContext(
                    "Rx",
                    MakeBoundCallback(&RitWpanNetHelper::AsciiApplicationRxSink, stream));
                return;
            }
        });
}

// Hook function for MAC timeout events
void
RitWpanNetHelper::AsciiRitWpanMacTimeoutSink(Ptr<OutputStreamWrapper> stream,
                                             std::string event,
                                             Time timestamp)
{
    *stream->GetStream() << timestamp.GetSeconds() << "," << event << std::endl;
}

void
RitWpanNetHelper::EnableMacTimeoutTracePerNode(const NodeContainer& nodes,
                                               const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "mac-beacon-wait.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            Ptr<NetDevice> netDevice = node->GetDevice(0);
            Ptr<RitWpanNetDevice> ritDevice = DynamicCast<RitWpanNetDevice>(netDevice);
            if (ritDevice)
            {
                Ptr<RitWpanMac> mac = DynamicCast<RitWpanMac>(ritDevice->GetMac());
                if (mac)
                {
                    mac->TraceConnectWithoutContext(
                        "BeaconWaitEvent",
                        MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanMacTimeoutSink, stream));
                }
            }
        });

    EnableTracePerNode(
        nodes,
        baseDir,
        "mac-data-wait.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            Ptr<NetDevice> netDevice = node->GetDevice(0);
            Ptr<RitWpanNetDevice> ritDevice = DynamicCast<RitWpanNetDevice>(netDevice);
            if (ritDevice)
            {
                Ptr<RitWpanMac> mac = DynamicCast<RitWpanMac>(ritDevice->GetMac());
                if (mac)
                {
                    mac->TraceConnectWithoutContext(
                        "DataWaitEvent",
                        MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanMacTimeoutSink, stream));
                }
            }
        });
}

// Hook function for MAC mode events
void
RitWpanNetHelper::AsciiRitWpanMacModeSink(Ptr<OutputStreamWrapper> stream,
                                          RitMacMode oldMode,
                                          RitMacMode newMode)
{
    std::string macMode;
    switch (newMode)
    {
    case RitMacMode::BOOTSTRAP_MODE:
        macMode = "Bootstrap";
        break;
    case RitMacMode::RECEIVER_MODE:
        macMode = "Receiver";
        break;
    case RitMacMode::SLEEP_MODE:
        macMode = "Sleep";
        break;
    case RitMacMode::SENDER_MODE:
        macMode = "Sender";
        break;
    case RitMacMode::RIT_MODE_DISABLED:
        macMode = "RIT Disabled";
        break;
    }
    *stream->GetStream() << Simulator::Now().GetSeconds() << "," << macMode << std::endl;
}

void
RitWpanNetHelper::EnableMacModeTracePerNode(const NodeContainer& nodes, const std::string& baseDir)
{
    EnableTracePerNode(
        nodes,
        baseDir,
        "mac-mode.csv",
        [](Ptr<Node> node, Ptr<OutputStreamWrapper> stream) {
            Ptr<NetDevice> netDevice = node->GetDevice(0);
            Ptr<RitWpanNetDevice> ritDevice = DynamicCast<RitWpanNetDevice>(netDevice);
            if (ritDevice)
            {
                Ptr<RitWpanMac> mac = DynamicCast<RitWpanMac>(ritDevice->GetMac());
                if (mac)
                {
                    mac->TraceConnectWithoutContext(
                        "MacMode",
                        MakeBoundCallback(&RitWpanNetHelper::AsciiRitWpanMacModeSink, stream));
                }
            }
        });
}
} // namespace lrwpan
} // namespace ns3
