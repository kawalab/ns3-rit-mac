/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "periodic-sender-helper.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/periodic-sender.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rit-wpan-net-device.h"

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("PeriodicSenderHelper");

PeriodicSenderHelper::PeriodicSenderHelper()
    : m_period(Seconds(60)),
      m_pktSize(20),
      m_dstAddr(),
      m_receiveOnly(false)
{
    m_factory.SetTypeId("ns3::lrwpan::PeriodicSender");

    m_initialDelay = CreateObject<UniformRandomVariable>();
    m_initialDelay->SetAttribute("Min", DoubleValue(0.0));
}

void
PeriodicSenderHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
PeriodicSenderHelper::Install(Ptr<Node> node) const
{
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
PeriodicSenderHelper::Install(NodeContainer c) const
{
    ApplicationContainer apps;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        apps.Add(InstallPriv(*i));
    }
    return apps;
}

Ptr<Application>
PeriodicSenderHelper::InstallPriv(Ptr<Node> node) const
{
    Ptr<PeriodicSender> app = m_factory.Create<PeriodicSender>();
    app->SetNode(node);

    // Register receive callback to the first RitWpanNetDevice found on the node
    for (uint32_t i = 0; i < node->GetNDevices(); ++i)
    {
        if (auto dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(i)))
        {
            dev->SetReceiveCallback(
                MakeCallback(&PeriodicSender::ReceivePacket, app));
            break;
        }
    }

    if (m_receiveOnly)
    {
        // Receive-only mode: no periodic transmission is scheduled
        app->SetReceiveOnly(true);
        node->AddApplication(app);
        return app;
    }

    app->SetInterval(m_period);
    app->SetInitialDelay(
        Seconds(m_initialDelay->GetValue(0.0, m_period.GetSeconds())));
    app->SetPacketSize(m_pktSize);
    app->SetDstAddr(m_dstAddr);

    node->AddApplication(app);
    return app;
}

void
PeriodicSenderHelper::SetPeriod(Time period)
{
    m_period = period;
}

void
PeriodicSenderHelper::SetPacketSize(uint8_t size)
{
    m_pktSize = size;
}

void
PeriodicSenderHelper::SetDstAddr(const Address& addr)
{
    m_dstAddr = addr;
}

void
PeriodicSenderHelper::SetReceiveOnly(bool enable)
{
    m_receiveOnly = enable;
}

} // namespace lrwpan
} // namespace ns3
