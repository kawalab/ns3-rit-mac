/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

 #include "random-sender-helper.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/random-sender.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rit-wpan-net-device.h"

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("RandomSenderHelper");

RandomSenderHelper::RandomSenderHelper()
    : m_minInterval(Seconds(180)),
      m_maxInterval(Seconds(600)),
      m_pktSize(20),
      m_dstAddr(),
      m_receiveOnly(false)
{
    m_factory.SetTypeId("ns3::lrwpan::RandomSender");

    m_initialDelay = CreateObject<UniformRandomVariable>();
    m_initialDelay->SetAttribute("Min", DoubleValue(0.0));
}

void
RandomSenderHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
RandomSenderHelper::Install(Ptr<Node> node) const
{
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
RandomSenderHelper::Install(NodeContainer c) const
{
    ApplicationContainer apps;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        apps.Add(InstallPriv(*i));
    }
    return apps;
}

Ptr<Application>
RandomSenderHelper::InstallPriv(Ptr<Node> node) const
{
    Ptr<RandomSender> app = m_factory.Create<RandomSender>();
    app->SetNode(node);

    // Register receive callback to the first RitWpanNetDevice found on the node
    for (uint32_t i = 0; i < node->GetNDevices(); ++i)
    {
        if (auto dev = DynamicCast<RitWpanNetDevice>(node->GetDevice(i)))
        {
            dev->SetReceiveCallback(
                MakeCallback(&RandomSender::ReceivePacket, app));
            break;
        }
    }

    if (m_receiveOnly)
    {
        // Receive-only mode: no random transmission is scheduled
        app->SetReceiveOnly(true);
        node->AddApplication(app);
        return app;
    }

    app->SetMinInterval(m_minInterval);
    app->SetMaxInterval(m_maxInterval);
    app->SetInitialDelay(
        Seconds(m_initialDelay->GetValue(0.0, m_maxInterval.GetSeconds())));
    app->SetPacketSize(m_pktSize);
    app->SetDstAddr(m_dstAddr);

    node->AddApplication(app);
    return app;
}

void
RandomSenderHelper::SetMinInterval(Time minInterval)
{
    m_minInterval = minInterval;
}

void
RandomSenderHelper::SetMaxInterval(Time maxInterval)
{
    m_maxInterval = maxInterval;
}

void
RandomSenderHelper::SetPacketSize(uint8_t size)
{
    m_pktSize = size;
}

void
RandomSenderHelper::SetDstAddr(const Address& addr)
{
    m_dstAddr = addr;
}

void
RandomSenderHelper::SetReceiveOnly(bool enable)
{
    m_receiveOnly = enable;
}

} // namespace lrwpan
} // namespace ns3
