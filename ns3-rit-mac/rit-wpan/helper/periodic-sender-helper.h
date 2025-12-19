/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef PERIODIC_SENDER_HELPER_H
#define PERIODIC_SENDER_HELPER_H

#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <cstdint>
#include <string>

namespace ns3
{

class UniformRandomVariable;

namespace lrwpan
{

/**
 * @ingroup lrwpan
 *
 * Helper to install PeriodicSender applications on nodes.
 */
class PeriodicSenderHelper
{
  public:
    PeriodicSenderHelper();
    ~PeriodicSenderHelper();

    /**
     * Set an attribute on the underlying PeriodicSender application.
     */
    void SetAttribute(std::string name, const AttributeValue& value);

    ApplicationContainer Install(NodeContainer c) const;
    ApplicationContainer Install(Ptr<Node> node) const;

    void SetPeriod(Time period);
    void SetPacketSize(uint8_t size);
    void SetDstAddr(const Address& addr);

    /**
     * Enable receive-only mode (no periodic transmission).
     */
    void SetReceiveOnly(bool enable);

  private:
    Ptr<Application> InstallPriv(Ptr<Node> node) const;

    ObjectFactory m_factory;

    Ptr<UniformRandomVariable> m_initialDelay;

    Time m_period;
    uint8_t m_pktSize;
    Address m_dstAddr;
    bool m_receiveOnly;
};

} // namespace lrwpan
} // namespace ns3

#endif // PERIODIC_SENDER_HELPER_H