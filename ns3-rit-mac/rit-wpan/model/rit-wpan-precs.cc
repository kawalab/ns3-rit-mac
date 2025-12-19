/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "rit-wpan-precs.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("RitWpanPreCs");
NS_OBJECT_ENSURE_REGISTERED(RitWpanPreCs);

TypeId
RitWpanPreCs::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RitWpanPreCs")
                            .SetParent<Object>()
                            .SetGroupName("RitWpan")
                            .AddConstructor<RitWpanPreCs>();
    return tid;
}

RitWpanPreCs::RitWpanPreCs()
{
    NS_LOG_FUNCTION(this);
    m_ccaRequestRunning = false;
}

RitWpanPreCs::~RitWpanPreCs()
{
    NS_LOG_FUNCTION(this);
    m_mac = nullptr;
}

void
RitWpanPreCs::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_lrWpanMacStateCallback = MakeNullCallback<void, MacState>();
    Cancel();
    m_mac = nullptr;
    Object::DoDispose();
}

void
RitWpanPreCs::SetMac(Ptr<RitWpanMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_mac = mac;
}

Ptr<RitWpanMac>
RitWpanPreCs::GetMac() const
{
    NS_LOG_FUNCTION(this);
    return m_mac;
}

void
RitWpanPreCs::SetLrWpanMacStateCallback(LrWpanMacStateCallback lrWpanMacStatusCallback)
{
    NS_LOG_FUNCTION(this);
    m_lrWpanMacStateCallback = lrWpanMacStatusCallback;
}

void
RitWpanPreCs::SetFallbackCcaConfirmCallback(FallbackCcaConfirmCallback fallbackCcaConfirmCallback)
{
    NS_LOG_FUNCTION(this);
    m_fallbackCcaConfirmCallback = fallbackCcaConfirmCallback;
}

void
RitWpanPreCs::Start()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Starting Pre-CS algorithm - immediate CCA request");

    // Pre-CS performs immediate CCA without any backoff or delay
    RequestCCA();
}

void
RitWpanPreCs::Cancel()
{
    NS_LOG_FUNCTION(this);

    if (m_ccaRequestRunning)
    {
        NS_LOG_DEBUG("Canceling ongoing CCA request");
        m_mac->GetPhy()->CcaCancel();
        m_ccaRequestRunning = false;
    }
}

void
RitWpanPreCs::RequestCCA()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Requesting CCA from PHY");

    m_ccaRequestRunning = true;
    m_mac->GetPhy()->PlmeCcaRequest();
}

void
RitWpanPreCs::PlmeCcaConfirm(PhyEnumeration status)
{
    NS_LOG_FUNCTION(this << status);

    // Only react on this event, if we are actually waiting for a CCA.
    // If the Pre-CS algorithm was canceled, we could still receive this event from
    // the PHY. In this case we ignore the event.
    if (m_ccaRequestRunning)
    {
        m_ccaRequestRunning = false;

        if (status == IEEE_802_15_4_PHY_IDLE)
        {
            NS_LOG_DEBUG("Channel assessed as IDLE - notifying MAC to proceed");

            // Channel is idle, notify MAC that transmission can proceed
            if (!m_lrWpanMacStateCallback.IsNull())
            {
                m_lrWpanMacStateCallback(CHANNEL_IDLE);
            }
        }
        else
        {
            NS_LOG_DEBUG("Channel assessed as BUSY - notifying MAC of access failure");

            // Channel is busy, notify MAC of immediate failure
            // No retry or backoff in Pre-CS
            if (!m_lrWpanMacStateCallback.IsNull())
            {
                m_lrWpanMacStateCallback(CHANNEL_ACCESS_FAILURE);
            }
        }
    }
    else
    {
        NS_LOG_DEBUG("Fallback to CSMA-CA via fallback CCA confirm callback.");
        if (!m_fallbackCcaConfirmCallback.IsNull())
        {
            m_fallbackCcaConfirmCallback(status);
        }
        else
        {
            NS_LOG_WARN("FallbackCcaConfirmCallback is not set — ignoring CCA confirm.");
        }
    }
}

} // namespace lrwpan
} // namespace ns3
