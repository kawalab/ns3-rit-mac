/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "time-drift-applier.h"

#include <ns3/double.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TimeDriftApplier");

TimeDriftApplier::TimeDriftApplier()
    : m_driftRatio(0.0)
    , m_rng(CreateObject<UniformRandomVariable>())
{
}

void
TimeDriftApplier::SetDriftRatio(double driftRatio)
{
    // Validate driftRatio (0.0 to 100.0)
    NS_ABORT_MSG_IF(driftRatio < 0.0, "driftRatio must be >= 0");
    NS_ABORT_MSG_IF(driftRatio > 100.0, "driftRatio must be <= 100");
    m_driftRatio = driftRatio;
}

Time
TimeDriftApplier::ApplyByRatio(Time inputTime) const
{
    return ApplyByRatio(inputTime, m_driftRatio);
}

Time
TimeDriftApplier::ApplyByRatio(Time inputTime, double driftRatio) const
{
    // Validate driftRatio (0.0 to 100.0)
    NS_ABORT_MSG_IF(driftRatio < 0.0, "driftRatio must be >= 0");
    NS_ABORT_MSG_IF(driftRatio > 100.0, "driftRatio must be <= 100");

    double inputTimeMs = inputTime.GetMilliSeconds();
    double minDrift = -inputTimeMs * driftRatio / 100.0;
    double maxDrift = inputTimeMs * driftRatio / 100.0;

    m_rng->SetAttribute("Min", DoubleValue(minDrift));
    m_rng->SetAttribute("Max", DoubleValue(maxDrift));
    double randomDelay = m_rng->GetValue();
    Time randomizedTime = inputTime + MilliSeconds(randomDelay);

    NS_LOG_DEBUG("Input Time: " << inputTime.GetMilliSeconds()
                                << "ms, Random Delay: " << randomDelay
                                << "ms, Output Time: " << randomizedTime.GetMilliSeconds() << "ms");
    return randomizedTime;
}

} // namespace ns3
