/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

/*
 * This file provides a minimal test scenario for verifying the basic operation of
 * the RIT-WPAN MAC implementation, including:
 *  - RIT (Receiver-Initiated Transmission) operation
 *
 * This scenario is intended for functional validation and debugging purposes only.
 * It is NOT designed for performance evaluation or large-scale simulation studies.
 */

#include "ns3/rit-wpan-mac.h"
#include "ns3/rit-wpan-net-device.h"
#include "ns3/rit-wpan-nwk-header.h"

#include <ns3/core-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/single-model-spectrum-channel.h>

using namespace ns3;
using namespace ns3::lrwpan;

/*
 * Data indication callback.
 * This handler is invoked when a packet is successfully received.
 * In this test scenario, the callback only logs the reception event.
 */
bool
DataIndication(Ptr<NetDevice> netDevice,
               Ptr<const Packet> packet,
               uint16_t x,
               const Address& addr)
{
    NS_LOG_UNCOND("DataIndication: received " << packet->GetSize() << " bytes");
    return true; // Indicate that the packet has been handled
}

int
main(int argc, char** argv)
{
    /*
     * Enable logging output for debugging.
     */
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnable("RitWpanMac", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanMac", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_DEBUG);

    /*
     * 1. Create nodes.
     */
    Ptr<Node> senderNode = CreateObject<Node>();
    Ptr<Node> receiverNode = CreateObject<Node>();

    /*
     * 2. Create RIT-WPAN net devices for each node.
     */
    Ptr<RitWpanNetDevice> senderDevice = CreateObject<RitWpanNetDevice>();
    Ptr<RitWpanNetDevice> receiverDevice = CreateObject<RitWpanNetDevice>();

    /*
     * 3. Configure MAC addresses.
     */
    senderDevice->SetAddress(Mac16Address("00:01"));
    receiverDevice->SetAddress(Mac16Address("00:02"));

    /*
     * 4. Create a wireless channel and propagation models.
     */
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    senderDevice->SetChannel(channel);
    receiverDevice->SetChannel(channel);

    /*
     * 5. Attach devices to nodes.
     */
    senderNode->AddDevice(senderDevice);
    receiverNode->AddDevice(receiverDevice);

    /*
     * 6. Register the receive callback on the receiver device.
     */
    receiverDevice->SetReceiveCallback(MakeCallback(&DataIndication));

    /*
     * 7. Enable RIT operation via MLME-SET.
     *    The RIT period is configured through the MAC PIB attribute.
     *    Both sender and receiver must be configured consistently.
     */
    Ptr<MacPibAttributes> pibAttr = Create<MacPibAttributes>();
    pibAttr->macRitPeriod = 65; // RIT period (unit depends on MAC design; approx. 1 second)
    MacPibAttributeIdentifier id = macRitPeriod;

    senderDevice->GetMac()->MlmeSetRequest(id, pibAttr);
    receiverDevice->GetMac()->MlmeSetRequest(id, pibAttr);

    /*
     * 8. Prepare a test data packet and MCPS-DATA.request parameters.
     */
    Ptr<Packet> packet = Create<Packet>(50); // 50-byte payload
    McpsDataRequestParams params;
    params.m_dstAddr = Mac16Address("00:02"); // Destination address
    params.m_msduHandle = 0;                  // Arbitrary handle
    params.m_txOptions = 0;                   // No special options

    /*
     * 9. Attach a simple RIT network header.
     */
    RitNwkHeader nwkHdr;
    nwkHdr.SetDstAddr(Mac16Address("00:02"));
    nwkHdr.SetRank(1);
    packet->AddHeader(nwkHdr);

    /*
     * 10. Schedule a single data transmission after initialization.
     *     The transmission time is chosen to avoid startup transients.
     */
    Simulator::ScheduleWithContext(senderNode->GetId(),
                                   Seconds(8.0),
                                   &RitWpanMac::McpsDataRequest,
                                   senderDevice->GetMac(),
                                   params,
                                   packet);

    /*
     * 11. Run the simulation.
     */
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
