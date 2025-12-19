/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "ns3/make-event.h"
#include <ns3/core-module.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/rit-wpan-mac.h>
#include <ns3/rit-wpan-net-device.h>
#include <ns3/rit-wpan-nwk-header.h>
#include <ns3/rit-wpan-precs.h>
#include <ns3/single-model-spectrum-channel.h>

using namespace ns3;
using namespace ns3::lrwpan;

NS_LOG_COMPONENT_DEFINE("rit-wpan-mac-trx-test");

class RitWpanMacTrxTest : public TestCase
{
  public:
    RitWpanMacTrxTest();
    ~RitWpanMacTrxTest() override;

  private:
    bool DataIndication(Ptr<NetDevice> dev,
                        Ptr<const Packet> pkt,
                        uint16_t proto,
                        const Address& addr);
    void DoRun() override;

    std::vector<uint32_t> expectedSizes = {30, 60, 90};
    int receivedPacketIndex = 0;
    int receivedCount = 0;
};

RitWpanMacTrxTest::RitWpanMacTrxTest()
    : TestCase("RitWpanMac two-node three-transmission send/receive test (RIT)")
{
}

RitWpanMacTrxTest::~RitWpanMacTrxTest()
{
}

bool
RitWpanMacTrxTest::DataIndication(Ptr<NetDevice> dev,
                                  Ptr<const Packet> pkt,
                                  uint16_t proto,
                                  const Address& addr)
{
    NS_LOG_UNCOND("Received packet size: " << pkt->GetSize());
    receivedPacketIndex++;
    receivedCount++;

    return true;
}

void
RitWpanMacTrxTest::DoRun()
{
    // Enable logging for debug
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnable("rit-wpan-mac-trx-test", LOG_LEVEL_DEBUG);

    // 1. Create nodes
    Ptr<Node> receiverNode = CreateObject<Node>();
    Ptr<Node> senderNode = CreateObject<Node>();

    // 2. Create NetDevices
    Ptr<RitWpanNetDevice> receiverDevice = CreateObject<RitWpanNetDevice>();
    Ptr<RitWpanNetDevice> senderDevice = CreateObject<RitWpanNetDevice>();

    // 3. Create and set up channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);
    receiverDevice->SetChannel(channel);
    senderDevice->SetChannel(channel);

    // 4. Set addresses
    receiverDevice->SetAddress(Mac16Address("00:00"));
    receiverDevice->SetRitRank(0);
    senderDevice->SetAddress(Mac16Address("00:01"));
    senderDevice->SetRitRank(1);

    // 5. Register devices to nodes
    receiverNode->AddDevice(receiverDevice);
    senderNode->AddDevice(senderDevice);

    // 6. Mobility setting (optional)

    // 7. Set receive callback
    receiverDevice->SetReceiveCallback(MakeCallback(&RitWpanMacTrxTest::DataIndication, this));

    // 8. RIT period setting
    Ptr<MacPibAttributes> pibAttr = Create<MacPibAttributes>();
    pibAttr->macRitPeriodTime = Time(Seconds(1)); // About 1 second period
    MacPibAttributeIdentifier id = macRitPeriodTime;
    senderDevice->GetMac()->MlmeSetRequest(id, pibAttr);
    receiverDevice->GetMac()->MlmeSetRequest(id, pibAttr);

    // 9. Create transmission packets
    Ptr<Packet> packet1 = Create<Packet>(30);
    Ptr<Packet> packet2 = Create<Packet>(60);
    Ptr<Packet> packet3 = Create<Packet>(90);

    // 10. Transmission scheduling
    Simulator::ScheduleWithContext(senderNode->GetId(), Seconds(8.0), [=]() {
        senderDevice->Send(packet1, Mac16Address("00:00"), 0);
    });

    Simulator::ScheduleWithContext(senderNode->GetId(), Seconds(12.0), [=]() {
        senderDevice->Send(packet2, Mac16Address("00:00"), 0);
    });

    Simulator::ScheduleWithContext(senderNode->GetId(), Seconds(16.0), [=]() {
        senderDevice->Send(packet3, Mac16Address("00:00"), 0);
    });

    // 12. Start and stop simulation
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    // 13. Check that the number of received packets is 3
    NS_TEST_ASSERT_MSG_EQ(
        receivedCount,
        3,
        "The number of received packets is not 3, actual receivedCount: " << receivedCount);

    Simulator::Destroy();
}

class RitWpanMacTestSuite : public TestSuite
{
  public:
    RitWpanMacTestSuite();
};

RitWpanMacTestSuite::RitWpanMacTestSuite()
    : TestSuite("rit-wpan-mac-trx-test", Type::UNIT)
{
    AddTestCase(new RitWpanMacTrxTest, Duration::QUICK);
}

static RitWpanMacTestSuite g_ritWpanMacTestSuite;
