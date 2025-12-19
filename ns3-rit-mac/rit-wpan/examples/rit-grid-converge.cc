/*
 * Copyright (c) 2025 Tomoya Murata
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

/*
 * This example provides a reference scenario for verifying the basic operation of the
 * RIT-WPAN MAC implementation in a grid-like multi-hop topology.
 *
 * Notes:
 *  - This scenario is intended for functional validation and trace collection.
 *  - It is not intended for performance evaluation or scalability studies.
 */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/rng-seed-manager.h"

#include "ns3/mac16-address.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/random-sender-helper.h"
#include "ns3/rit-rank-helper.h"
#include "ns3/rit-wpan-helper.h"
#include "ns3/rit-wpan-net-device.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace ns3;
using namespace lrwpan;

NS_LOG_COMPONENT_DEFINE("RitGridConverge");

namespace
{

std::string
MakeScenarioType(const std::string& placement, const std::string& density, const std::string& app)
{
    return placement + "_" + density + "_" + app;
}

struct ScenarioConfig
{
    // RIT parameters (interpreted as milliseconds in the CLI as in the original code)
    double beaconIntervalMs = 5.0;     // BI
    double dataWaitDurationMs = 10.0;  // DWD
    double txWaitDurationMs = 5000.0;  // TWD

    // Topology / run control
    int32_t routerNodeCount = 12; // Nodes (if -1, derived from placement/density)
    uint32_t simulationDays = 1;
    double driftRatio = 10.0;
    uint32_t randomSeed = 1;

    // MAC module toggles
    bool dataCsmaEnabled = true;
    bool beaconCsmaEnabled = false;
    bool dataPreCsEnabled = false;
    bool beaconPreCsEnabled = true;
    bool dataPreCsBEnabled = false;
    bool beaconPreCsBEnabled = false;
    bool continuousTxEnabled = false;
    bool beaconRandomizeEnabled = false;
    bool compactRitDataRequestEnabled = false;
    bool beaconAckEnabled = false;

    // Scenario variants
    std::string nodePlacement = "edge"; // "edge" or "center"
    std::string nodeDensity = "low";    // "low" or "middle"
    std::string appType = "periodic";   // "periodic" or "random"

    // Application parameters
    uint32_t appPacketSize = 8;
    uint32_t appPeriodicIntervalSec = 300;
    uint32_t appRandomMinIntervalSec = 180;
    uint32_t appRandomMaxIntervalSec = 600;
};

void
BindCommandLine(CommandLine& cmd, ScenarioConfig& cfg)
{
    cmd.AddValue("BI", "Beacon interval (milliseconds)", cfg.beaconIntervalMs);
    cmd.AddValue("TWD", "Sender wait duration (milliseconds)", cfg.txWaitDurationMs);
    cmd.AddValue("DWD", "Receiver data wait duration (milliseconds)", cfg.dataWaitDurationMs);

    cmd.AddValue("Nodes", "Number of router nodes (-1: auto)", cfg.routerNodeCount);
    cmd.AddValue("Days", "Simulation duration in days", cfg.simulationDays);
    cmd.AddValue("DR", "Drift ratio", cfg.driftRatio);
    cmd.AddValue("Seed", "Random seed", cfg.randomSeed);

    cmd.AddValue("DataCsma", "Enable CSMA for data transmission", cfg.dataCsmaEnabled);
    cmd.AddValue("BeaconCsma", "Enable CSMA for beacon transmission", cfg.beaconCsmaEnabled);
    cmd.AddValue("DataPreCs", "Enable Pre-CS for data transmission", cfg.dataPreCsEnabled);
    cmd.AddValue("BeaconPreCs", "Enable Pre-CS for beacon transmission", cfg.beaconPreCsEnabled);
    cmd.AddValue("DataPreCsB", "Enable Pre-CSB for data transmission", cfg.dataPreCsBEnabled);
    cmd.AddValue("BeaconPreCsB", "Enable Pre-CSB for beacon transmission", cfg.beaconPreCsBEnabled);

    cmd.AddValue("ContinuousTx", "Enable continuous transmission mode", cfg.continuousTxEnabled);
    cmd.AddValue("BeaconRandomize", "Enable beacon interval randomization", cfg.beaconRandomizeEnabled);
    cmd.AddValue("CompactRitDataRequest",
                 "Enable compact RIT Data Request header",
                 cfg.compactRitDataRequestEnabled);
    cmd.AddValue("BeaconAck", "Enable ACK for beacon transmission", cfg.beaconAckEnabled);

    cmd.AddValue("Placement", "Node placement type (edge/center)", cfg.nodePlacement);
    cmd.AddValue("Density", "Node density type (low/middle)", cfg.nodeDensity);
    cmd.AddValue("App", "Application type (periodic/random)", cfg.appType);

    cmd.AddValue("AppPeriodicInterval",
                 "Interval for periodic application (seconds)",
                 cfg.appPeriodicIntervalSec);
    cmd.AddValue("AppRandomMinInterval",
                 "Minimum interval for random application (seconds)",
                 cfg.appRandomMinIntervalSec);
    cmd.AddValue("AppRandomMaxInterval",
                 "Maximum interval for random application (seconds)",
                 cfg.appRandomMaxIntervalSec);
    cmd.AddValue("AppPacketSize", "Packet size for application (bytes)", cfg.appPacketSize);
}

void
ResolveRouterNodeCount(ScenarioConfig& cfg)
{
    if (cfg.routerNodeCount != -1)
    {
        return;
    }

    if (cfg.nodePlacement == "edge")
    {
        if (cfg.nodeDensity == "low")
        {
            cfg.routerNodeCount = 15;
        }
        else if (cfg.nodeDensity == "middle")
        {
            cfg.routerNodeCount = 45;
        }
        else
        {
            NS_FATAL_ERROR("Unsupported node density for edge placement: " << cfg.nodeDensity);
        }
        return;
    }

    if (cfg.nodePlacement == "center")
    {
        if (cfg.nodeDensity == "low")
        {
            cfg.routerNodeCount = 8;
        }
        else if (cfg.nodeDensity == "middle")
        {
            cfg.routerNodeCount = 48;
        }
        else
        {
            NS_FATAL_ERROR("Unsupported node density for center placement: " << cfg.nodeDensity);
        }
        return;
    }

    NS_FATAL_ERROR("Unsupported node placement: " << cfg.nodePlacement);
}

Time
EffectiveParentBeaconInterval(const ScenarioConfig& cfg)
{
    // Preserve the original special-cases exactly.
    if (cfg.nodePlacement == "edge" && cfg.nodeDensity == "middle")
    {
        return MilliSeconds(cfg.beaconIntervalMs / 2.5);
    }
    if (cfg.nodePlacement == "center" && cfg.nodeDensity == "middle")
    {
        return MilliSeconds(cfg.beaconIntervalMs / 4.0);
    }
    return MilliSeconds(cfg.beaconIntervalMs);
}

RitWpanMacModuleConfig
MakeModuleConfig(const ScenarioConfig& cfg)
{
    RitWpanMacModuleConfig m;
    m.dataCsmaEnabled = cfg.dataCsmaEnabled;
    m.dataPreCsEnabled = cfg.dataPreCsEnabled;
    m.dataPreCsBEnabled = cfg.dataPreCsBEnabled;

    m.beaconCsmaEnabled = cfg.beaconCsmaEnabled;
    m.beaconPreCsEnabled = cfg.beaconPreCsEnabled;
    m.beaconPreCsBEnabled = cfg.beaconPreCsBEnabled;

    m.continuousTxEnabled = cfg.continuousTxEnabled;
    m.beaconRandomizeEnabled = cfg.beaconRandomizeEnabled;
    m.compactRitDataRequestEnabled = cfg.compactRitDataRequestEnabled;
    m.beaconAckEnabled = cfg.beaconAckEnabled;
    return m;
}

void
InstallTopologyEdge(const ScenarioConfig& cfg, NodeContainer routers, NodeContainer parent)
{
    if (cfg.nodeDensity == "low")
    {
        // 12 nodes (3x4), 70 m spacing
        auto positionAlloc = CreateObject<GridPositionAllocator>();
        positionAlloc->SetMinX(0.0);
        positionAlloc->SetMinY(0.0);
        positionAlloc->SetDeltaX(70.0);
        positionAlloc->SetDeltaY(70.0);
        positionAlloc->SetN(3);
        positionAlloc->SetLayoutType(GridPositionAllocator::ROW_FIRST);

        MobilityHelper mob;
        mob.SetPositionAllocator(positionAlloc);
        mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mob.Install(routers);

        RitWpanRankHelper rankHelper;
        rankHelper.Install(routers, 3);
    }
    else if (cfg.nodeDensity == "middle")
    {
        // 45 nodes, 25 m spacing, ranks provided explicitly
        auto positionAlloc = CreateObject<GridPositionAllocator>();
        positionAlloc->SetMinX(0.0);
        positionAlloc->SetMinY(0.0);
        positionAlloc->SetDeltaX(25.0);
        positionAlloc->SetDeltaY(25.0);
        positionAlloc->SetN(5);
        positionAlloc->SetLayoutType(GridPositionAllocator::ROW_FIRST);

        MobilityHelper mob;
        mob.SetPositionAllocator(positionAlloc);
        mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mob.Install(routers);

        const std::vector<uint8_t> middleRanks = {
            1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
        };

        RitWpanRankHelper rankHelper;
        rankHelper.Install(routers, middleRanks);
    }
    else
    {
        NS_FATAL_ERROR("Unsupported node density for edge placement: " << cfg.nodeDensity);
    }

    // Parent is always placed above the top edge (same as original).
    double parentX = 0.0;
    double parentY = 0.0;

    if (cfg.nodeDensity == "low")
    {
        parentX = 70.0;
        parentY = -70.0;
    }
    else if (cfg.nodeDensity == "middle")
    {
        parentX = 40.0;
        parentY = -50.0;
    }
    else if (cfg.nodeDensity == "high")
    {
        // Kept for parity with the original code path (even if high is not used currently).
        parentX = 2 * 10.0;
        parentY = -10.0;
    }

    auto parentPos = CreateObject<ListPositionAllocator>();
    parentPos->Add(Vector(parentX, parentY, 0.0));

    MobilityHelper parentMob;
    parentMob.SetPositionAllocator(parentPos);
    parentMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    parentMob.Install(parent);
}

void
InstallTopologyCenter(const ScenarioConfig& cfg, NodeContainer routers, NodeContainer parent)
{
    double spacing = 0.0;
    double row = 5.0;
    double parentX = 0.0;
    double parentY = 0.0;
    std::vector<uint8_t> ranks;

    if (cfg.nodeDensity == "low")
    {
        // Original code aborts here. Keep the behavior.
        spacing = 30.0;
        row = 5.0;
        NS_FATAL_ERROR("Unsupported node density for center placement: "
                       << cfg.nodeDensity << cfg.nodePlacement);
    }
    else if (cfg.nodeDensity == "middle")
    {
        spacing = 40.0;
        row = 7.0;
        ranks = {
            3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 1, 1, 1, 2, 3, 3, 2, 1, 0,
            1, 2, 3, 3, 2, 1, 1, 1, 2, 3, 3, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        };
        parentX = 3 * spacing;
        parentY = 3 * spacing;
    }
    else
    {
        NS_FATAL_ERROR("Unsupported node density for center placement: " << cfg.nodeDensity);
    }

    auto positionAlloc = CreateObject<GridPositionAllocator>();
    positionAlloc->SetMinX(0.0);
    positionAlloc->SetMinY(0.0);
    positionAlloc->SetDeltaX(spacing);
    positionAlloc->SetDeltaY(spacing);
    positionAlloc->SetN(row);
    positionAlloc->SetLayoutType(GridPositionAllocator::ROW_FIRST);

    MobilityHelper mob;
    mob.SetPositionAllocator(positionAlloc);
    mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mob.Install(routers);

    RitWpanRankHelper rankHelper;
    rankHelper.Install(routers, ranks);

    auto parentPos = CreateObject<ListPositionAllocator>();
    parentPos->Add(Vector(parentX, parentY, 0.0));

    MobilityHelper parentMob;
    parentMob.SetPositionAllocator(parentPos);
    parentMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    parentMob.Install(parent);
}

void
InstallApplications(const ScenarioConfig& cfg, NodeContainer routers, NodeContainer parent)
{
    const Mac16Address sink("00:00");

    if (cfg.appType == "periodic")
    {
        PeriodicSenderHelper routerApp;
        routerApp.SetPeriod(Seconds(cfg.appPeriodicIntervalSec));
        routerApp.SetPacketSize(cfg.appPacketSize);
        routerApp.SetDstAddr(sink);
        routerApp.Install(routers);

        PeriodicSenderHelper parentApp;
        parentApp.SetReceiveOnly(true);
        parentApp.Install(parent);
        return;
    }

    if (cfg.appType == "random")
    {
        RandomSenderHelper routerApp;
        routerApp.SetMinInterval(Seconds(cfg.appRandomMinIntervalSec));
        routerApp.SetMaxInterval(Seconds(cfg.appRandomMaxIntervalSec));
        routerApp.SetPacketSize(cfg.appPacketSize);
        routerApp.SetDstAddr(sink);
        routerApp.Install(routers);

        RandomSenderHelper parentApp;
        parentApp.SetReceiveOnly(true);
        parentApp.Install(parent);
        return;
    }

    NS_FATAL_ERROR("Unsupported application type: " << cfg.appType);
}

void
PrintRunSummary(const ScenarioConfig& cfg, const std::string& scenarioType)
{
    NS_LOG_UNCOND("==== Simulation parameters ====");
    NS_LOG_UNCOND("ScenarioType: " << scenarioType
                                  << " | BI: " << cfg.beaconIntervalMs << " ms"
                                  << " | DWD: " << cfg.dataWaitDurationMs << " ms"
                                  << " | TWD: " << cfg.txWaitDurationMs << " ms"
                                  << " | Nodes: " << cfg.routerNodeCount
                                  << " | Days: " << cfg.simulationDays
                                  << " | DR: " << cfg.driftRatio
                                  << " | Seed: " << cfg.randomSeed
                                  << " | DataCsma: " << (cfg.dataCsmaEnabled ? "true" : "false")
                                  << " | BeaconCsma: " << (cfg.beaconCsmaEnabled ? "true" : "false")
                                  << " | DataPreCs: " << (cfg.dataPreCsEnabled ? "true" : "false")
                                  << " | BeaconPreCs: " << (cfg.beaconPreCsEnabled ? "true" : "false")
                                  << " | ContinuousTx: " << (cfg.continuousTxEnabled ? "true" : "false")
                                  << " | BeaconRandomize: " << (cfg.beaconRandomizeEnabled ? "true" : "false")
                                  << " | CompactRitDataRequest: "
                                  << (cfg.compactRitDataRequestEnabled ? "true" : "false")
                                  << " | BeaconAck: " << (cfg.beaconAckEnabled ? "true" : "false")
                                  << " | Placement: " << cfg.nodePlacement
                                  << " | Density: " << cfg.nodeDensity
                                  << " | App: " << cfg.appType);
}

} // namespace

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LOG_PREFIX_TIME);

    ScenarioConfig cfg;

    CommandLine cmd;
    BindCommandLine(cmd, cfg);
    cmd.Parse(argc, argv);

    ResolveRouterNodeCount(cfg);
    RngSeedManager::SetSeed(cfg.randomSeed);

    const std::string scenarioType = MakeScenarioType(cfg.nodePlacement, cfg.nodeDensity, cfg.appType);

    // ----- Node creation -----
    NodeContainer parentNodes;
    NodeContainer routerNodes;
    NodeContainer allNodes;
    parentNodes.Create(1);
    routerNodes.Create(cfg.routerNodeCount);
    allNodes.Add(parentNodes);
    allNodes.Add(routerNodes);

    // ----- Device installation -----
    RitWpanNetHelper helper;

    helper.SetMacRitDataWaitDuration(MilliSeconds(cfg.dataWaitDurationMs));
    helper.SetMacRitTxWaitDuration(MilliSeconds(cfg.txWaitDurationMs));
    helper.SetRitMacDriftRatio(cfg.driftRatio);
    helper.SetRitMacModuleConfig(MakeModuleConfig(cfg));

    // Parent: RxAlwaysOn = true with an effective BI (preserve original behavior)
    helper.SetMacRitPeriod(EffectiveParentBeaconInterval(cfg));
    helper.SetRxAlwaysOn(true);
    NetDeviceContainer parentDevices = helper.Install(parentNodes);

    // Routers: RxAlwaysOn = false with baseline BI (preserve original behavior)
    helper.SetMacRitPeriod(MilliSeconds(cfg.beaconIntervalMs));
    helper.SetRxAlwaysOn(false);
    NetDeviceContainer routerDevices = helper.Install(routerNodes);

    // ----- Mobility / ranks -----
    if (cfg.nodePlacement == "edge")
    {
        InstallTopologyEdge(cfg, routerNodes, parentNodes);
    }
    else if (cfg.nodePlacement == "center")
    {
        InstallTopologyCenter(cfg, routerNodes, parentNodes);
    }
    else
    {
        NS_FATAL_ERROR("Unsupported node placement: " << cfg.nodePlacement);
    }

    // ----- Parent device metadata -----
    auto parentDev = DynamicCast<RitWpanNetDevice>(parentDevices.Get(0));
    parentDev->SetAddress(Mac16Address("00:00"));
    parentDev->SetRitRank(0);

    // ----- Applications -----
    InstallApplications(cfg, routerNodes, parentNodes);

    // ----- Traces -----
    helper.SetScenarioType(scenarioType);
    helper.EnableAllTracesPerNode(allNodes, cfg.simulationDays, cfg.randomSeed);

    // ----- Run -----
    PrintRunSummary(cfg, scenarioType);
    NS_LOG_UNCOND("Simulation starts.");
    Simulator::Stop(Days(cfg.simulationDays));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
