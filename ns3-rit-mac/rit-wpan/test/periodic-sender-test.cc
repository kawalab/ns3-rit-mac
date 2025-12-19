/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-module.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/periodic-sender.h"
#include "ns3/test.h"

using namespace ns3;
using namespace ns3::lrwpan;

NS_LOG_COMPONENT_DEFINE("periodic-sender-test");

/**
 * @brief Test case for the PeriodicSender application and LrWpan functionality.
 *
 * This test verifies the following aspects:
 *  - Periodic packet transmission at a fixed interval
 *  - Correct packet reception via LrWpanNetDevice
 *  - Correct handling of packet size, timing, and transmission count
 */
class PeriodicSenderTRxTestCase : public TestCase
{
  public:
    PeriodicSenderTRxTestCase();
    virtual ~PeriodicSenderTRxTestCase();

  private:
    void DoRun() override;
    bool ReceivePacket(Ptr<NetDevice> device,
                       Ptr<const Packet> packet,
                       uint16_t protocol,
                       const Address& sender);

    static constexpr uint8_t TEST_PKT_SIZE = 8;
    static constexpr double TEST_INTERVAL = 1.0;
    uint8_t m_receivedPktCount;
    std::vector<uint8_t> m_receivedPktSizes;
    std::vector<Time> m_receivedPktTimestamps;
};

PeriodicSenderTRxTestCase::PeriodicSenderTRxTestCase()
    : TestCase("Verification of the periodic sender application and LrWpan functionality"),
      m_receivedPktCount(0)
{
}

PeriodicSenderTRxTestCase::~PeriodicSenderTRxTestCase()
{
}

bool
PeriodicSenderTRxTestCase::ReceivePacket(Ptr<NetDevice> device,
                                         Ptr<const Packet> packet,
                                         uint16_t protocol,
                                         const Address& sender)
{
    NS_LOG_UNCOND("Packet received by node.");
    m_receivedPktCount++;
    m_receivedPktSizes.push_back(packet->GetSize());
    m_receivedPktTimestamps.push_back(Simulator::Now());
    return true;
}

void
PeriodicSenderTRxTestCase::DoRun()
{
    // Configure logging for debugging
    LogComponentEnable("PeriodicSender", LOG_LEVEL_DEBUG);
    LogComponentEnable("LrWpanNetDevice", LOG_LEVEL_DEBUG);
    LogComponentEnable("periodic-sender-test", LOG_LEVEL_ALL);

    // 1. Create nodes
    NodeContainer nodes;
    nodes.Create(2);

    // 2. Install LrWpanNetDevice on nodes
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer devices = lrWpanHelper.Install(nodes);

    // 3. Configure node positions to ensure connectivity
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // Node 0
    positionAlloc->Add(Vector(1.0, 0.0, 0.0)); // Node 1 (placed nearby)

    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // 4. Set MAC short addresses
    Ptr<LrWpanNetDevice> lrWpanDevice0 = DynamicCast<LrWpanNetDevice>(devices.Get(0));
    Ptr<LrWpanNetDevice> lrWpanDevice1 = DynamicCast<LrWpanNetDevice>(devices.Get(1));
    lrWpanDevice0->GetMac()->SetShortAddress(Mac16Address("00:00"));
    lrWpanDevice1->GetMac()->SetShortAddress(Mac16Address("00:01"));

    // 5. Register receive callback on the receiver device
    lrWpanDevice0->SetReceiveCallback(
        MakeCallback(&PeriodicSenderTRxTestCase::ReceivePacket, this));

    // 6. [Sender side] Configure PeriodicSender application
    Ptr<PeriodicSender> senderApp = CreateObject<PeriodicSender>();
    senderApp->SetNode(nodes.Get(1));
    senderApp->SetDstAddr(Mac16Address("00:00"));
    senderApp->SetPacketSize(TEST_PKT_SIZE);
    senderApp->SetInterval(Seconds(TEST_INTERVAL));
    senderApp->SetInitialDelay(Seconds(1.0));

    nodes.Get(1)->AddApplication(senderApp);
    senderApp->SetStartTime(Seconds(2.0));
    senderApp->SetStopTime(Seconds(10.1));

    // 7. Run simulation
    Simulator::Run();

    // 8. Verify that 8 packets are received
    //    (from 2.0s to 10.1s, transmitted every 1 second)
    NS_TEST_ASSERT_MSG_EQ(m_receivedPktCount, 8, "The number of received packets is incorrect.");

    // Verify packet sizes
    for (size_t i = 0; i < m_receivedPktSizes.size(); i++)
    {
        NS_TEST_ASSERT_MSG_EQ(m_receivedPktSizes[i],
                              TEST_PKT_SIZE,
                              "The packet size is incorrect for packet " << i + 1);
    }

    // Verify transmission interval
    for (size_t i = 1; i < m_receivedPktTimestamps.size(); i++)
    {
        Time interval = m_receivedPktTimestamps[i] - m_receivedPktTimestamps[i - 1];
        NS_TEST_ASSERT_MSG_EQ_TOL(
            interval.GetSeconds(),
            TEST_INTERVAL,
            0.01,
            "Packet " << i + 1 << " interval is incorrect. Expected interval: " << TEST_INTERVAL
                      << " seconds, but got: " << interval.GetSeconds() << " seconds.");
    }

    // Verify sender-side transmission count
    uint32_t sentPackets = senderApp->GetSentPackets();
    NS_TEST_ASSERT_MSG_EQ(
        sentPackets,
        8,
        "The number of sent packets is incorrect. Expected: 8, but got: " << sentPackets);

    Simulator::Destroy();
}
