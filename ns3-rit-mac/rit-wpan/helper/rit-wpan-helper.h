/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef RIT_WPAN_HELPER_H
#define RIT_WPAN_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ptr.h"
#include "ns3/spectrum-channel.h"

#include "ns3/rit-wpan-mac.h"        // RitMacMode, MacState, module config
#include "ns3/rit-wpan-net-device.h" // RitWpanNetDevice
#include "ns3/lr-wpan-phy.h"         // PhyEnumeration (trace sink signature)

#include <functional>
#include <string>

namespace ns3
{
namespace lrwpan
{

/**
 * @ingroup lrwpan
 *
 * @brief Helper class to install RitWpanNetDevice instances and enable per-node ASCII traces.
 *
 * This helper provides:
 *  - Device installation with a shared SpectrumChannel
 *  - RIT MAC parameter configuration
 *  - Convenience methods to enable per-node trace logging (MAC/NWK/PHY/App/etc.)
 *
 * Note: This helper is primarily intended for research scenarios and trace collection.
 */
class RitWpanNetHelper
{
  public:
    RitWpanNetHelper();
    ~RitWpanNetHelper();

    RitWpanNetHelper(const RitWpanNetHelper&) = delete;
    RitWpanNetHelper& operator=(const RitWpanNetHelper&) = delete;

    /** @brief Get the channel used by this helper. */
    Ptr<SpectrumChannel> GetChannel();

    /** @brief Set the channel used by this helper. */
    void SetChannel(Ptr<SpectrumChannel> channel);

    /** @brief Set the channel by name (Names::Find<SpectrumChannel>). */
    void SetChannel(std::string channelName);

    /** @brief Set RIT beacon interval. */
    void SetMacRitPeriod(Time macRitPeriod);

    /** @brief Set receiver-side data wait duration after beacon. */
    void SetMacRitDataWaitDuration(Time macRitDataWaitDuration);

    /** @brief Set sender-side wait duration before data transmission. */
    void SetMacRitTxWaitDuration(Time macRitTxWaitDuration);

    /** @brief Set MAC drift ratio (stored; application to MAC depends on implementation). */
    void SetRitMacDriftRatio(double riMacDriftRatio);

    /** @brief Set receiver always-on behavior for installed devices. */
    void SetRxAlwaysOn(bool alwaysOn);

    /**
     * @brief Attach mobility model to a PHY instance.
     * @param phy PHY object
     * @param m Mobility model
     */
    void AddMobility(Ptr<LrWpanPhy> phy, Ptr<MobilityModel> m);

    /**
     * @brief Install RitWpanNetDevice on each node in the container.
     * @param c Nodes to install devices on
     * @return Container of installed devices
     */
    NetDeviceContainer Install(NodeContainer c);

    /**
     * @brief Set extended addresses for devices (reserved for future use / compatibility).
     * @param c NetDeviceContainer
     */
    void SetExtendedAddresses(NetDeviceContainer c) {}

    /** @brief Set RIT MAC module configuration. */
    void SetRitMacModuleConfig(const RitWpanMacModuleConfig& config);

    /**
     * @brief Generic helper: enable a per-node trace and write to a per-node log file.
     *
     * @param nodes Target nodes
     * @param baseDir Base directory
     * @param logName Log file name (e.g., "mac-txlog.csv")
     * @param traceSetupFn Callback that connects trace sources on the node
     */
    void EnableTracePerNode(const NodeContainer& nodes,
                            const std::string& baseDir,
                            const std::string& logName,
                            std::function<void(Ptr<Node>, Ptr<OutputStreamWrapper>)> traceSetupFn);

    // Convenience wrappers built on EnableTracePerNode()
    void EnableMacStateTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnableEnergyTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnableNwkTxTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnableNwkRxTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnableMacTxTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnableMacRxTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnablePhyStateTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnableMacTimeoutTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnableMacModeTracePerNode(const NodeContainer& nodes, const std::string& baseDir);

    /**
     * @brief Enable all traces using a precomputed base directory.
     *
     * @param nodes Target nodes
     * @param baseDir Base directory to write logs into
     * @param seed Run identifier
     */
    void EnableAllTracesPerNode(const NodeContainer& nodes, const std::string& baseDir, uint32_t seed);

    /** @brief Build log base directory path from parameters. */
    std::string GetLogBaseDir(const std::string& module,
                              uint32_t macRitPeriod,
                              uint32_t macRitTxWaitDuration,
                              uint32_t macRitDataWaitDuration,
                              uint32_t simulationTime,
                              uint32_t runNumber) const;

    /** @brief Build short module name tag from module configuration. */
    std::string GetModuleShortName(const RitWpanMacModuleConfig& config) const;

    /** @brief Enable all traces with base directory automatically derived from current settings. */
    void EnableAllTracesPerNode(const NodeContainer& nodes, uint32_t simulationTime, uint32_t seed);

    // PHY Tx/Rx trace sinks (ASCII)
    static void AsciiRitWpanPhyTxSink(Ptr<OutputStreamWrapper> stream,
                                      std::string event,
                                      Ptr<const Packet> pkt);
    static void AsciiRitWpanPhyRxSink(Ptr<OutputStreamWrapper> stream,
                                      std::string event,
                                      Ptr<const Packet> pkt);
    static void AsciiRitWpanPhyRxSink(Ptr<OutputStreamWrapper> stream,
                                      std::string event,
                                      Ptr<const Packet> pkt,
                                      double sinr);

    // PHY Tx/Rx trace enablers
    void EnablePhyTxTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnablePhyRxTracePerNode(const NodeContainer& nodes, const std::string& baseDir);

    // Application trace logging (PeriodicSender / RandomSender)
    void EnableApplicationTxTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    void EnableApplicationRxTracePerNode(const NodeContainer& nodes, const std::string& baseDir);
    static void AsciiApplicationTxSink(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> pkt);
    static void AsciiApplicationRxSink(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> pkt);
    void EnableApplicationTracePerNode(const NodeContainer& nodes, const std::string& baseDir);

    /** @brief Set scenario type label used for log directory path. */
    void SetScenarioType(const std::string& scenarioType);

    /** @brief Get scenario type label used for log directory path. */
    std::string GetScenarioType() const;

  private:
    // MAC state sink
    static void AsciiRitWpanMacStateSink(Ptr<OutputStreamWrapper> stream,
                                         uint32_t nodeId,
                                         lrwpan::MacState oldState,
                                         lrwpan::MacState newState);

    // Energy sink
    static void AsciiRitWpanEnergySink(Ptr<OutputStreamWrapper> stream, uint32_t nodeId, double energy);

    // NWK sinks
    static void AsciiRitWpanNwkRxSink(Ptr<OutputStreamWrapper> stream, std::string event, Ptr<const Packet> pkt);
    static void AsciiRitWpanNwkTxSink(Ptr<OutputStreamWrapper> stream, std::string event, Ptr<const Packet> pkt);

    // MAC sinks
    static void AsciiRitWpanMacTxSink(Ptr<OutputStreamWrapper> stream, std::string event, Ptr<const Packet> pkt);
    static void AsciiRitWpanMacRxSink(Ptr<OutputStreamWrapper> stream, std::string event, Ptr<const Packet> pkt);

    // PHY state sink
    static void AsciiRitWpanPhyStateSink(Ptr<OutputStreamWrapper> stream,
                                         uint32_t nodeId,
                                         Time time,
                                         PhyEnumeration oldState,
                                         PhyEnumeration newState);

    // MAC timeout sink
    static void AsciiRitWpanMacTimeoutSink(Ptr<OutputStreamWrapper> stream, std::string event, Time timestamp);

    // MAC mode sink
    static void AsciiRitWpanMacModeSink(Ptr<OutputStreamWrapper> stream, RitMacMode oldMode, RitMacMode newMode);

    // Per-node log helpers
    std::string GetNodeLogDir(const std::string& baseDir, uint32_t nodeId) const;
    std::string GetNodeLogFilePath(const std::string& baseDir,
                                   uint32_t nodeId,
                                   const std::string& logName) const;
    Ptr<OutputStreamWrapper> GetNodeLogStream(const std::string& baseDir,
                                              uint32_t nodeId,
                                              const std::string& logName) const;

    Ptr<SpectrumChannel> m_channel;
    Time m_macRitPeriod;
    Time m_macRitDataWaitDuration;
    Time m_macRitTxWaitDuration;
    double m_riMacDriftRatio = 0.0;
    bool m_rxAlwaysOn = false;
    RitWpanMacModuleConfig m_moduleConfig;

    std::string m_baseLogDirectory;
    std::string m_scenarioType = "default";
};

} // namespace lrwpan
} // namespace ns3

#endif // RIT_WPAN_HELPER_H
