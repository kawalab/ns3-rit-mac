/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef RANDOM_SENDER_HELPER_H
#define RANDOM_SENDER_HELPER_H

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
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
 * Helper to install RandomSender applications on nodes.
 */
class RandomSenderHelper
{
  public:
    RandomSenderHelper();
    ~RandomSenderHelper();

    /**
     * Set an attribute on the underlying RandomSender application.
     */
    void SetAttribute(std::string name, const AttributeValue& value);

    ApplicationContainer Install(NodeContainer c) const;
    ApplicationContainer Install(Ptr<Node> node) const;

    void SetMinInterval(Time minInterval);
    void SetMaxInterval(Time maxInterval);
    void SetPacketSize(uint8_t size);
    void SetDstAddr(const Address& addr);

    /**
     * Enable receive-only mode (no random transmission).
     */
    void SetReceiveOnly(bool enable);

  private:
    Ptr<Application> InstallPriv(Ptr<Node> node) const;

    ObjectFactory m_factory;

    Time m_minInterval;
    Time m_maxInterval;
    uint8_t m_pktSize;
    Address m_dstAddr;
    bool m_receiveOnly;

    Ptr<UniformRandomVariable> m_initialDelay;
};

} // namespace lrwpan
} // namespace ns3

#endif // RANDOM_SENDER_HELPER_H
