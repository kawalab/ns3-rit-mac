/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#include "clock-drift-applier.h"

#include <ns3/double.h>
#include <ns3/log.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ClockDriftApplier");

ClockDriftApplier::ClockDriftApplier()
    : m_skew(0.0),
      m_K(1e-9), // noise coefficient K (unit: s)
      m_noiseGen(CreateObject<NormalRandomVariable>()),
      m_minSkewPpm(-250.0),
      m_maxSkewPpm(250.0)
{
}

void
ClockDriftApplier::SetSkewRange(double minPpm, double maxPpm)
{
    m_minSkewPpm = minPpm;
    m_maxSkewPpm = maxPpm;
}

void
ClockDriftApplier::Initialize(uint32_t nodeId, uint32_t runId)
{
    Ptr<UniformRandomVariable> skewGen = CreateObject<UniformRandomVariable>();
    skewGen->SetStream(1000 + nodeId);

    double ppm = skewGen->GetValue(m_minSkewPpm, m_maxSkewPpm);
    m_skew = ppm / 1e6; // Convert ppm to a ratio

    m_noiseGen->SetStream(2000 + runId);

    NS_LOG_INFO("Initialized ClockDriftApplier with skew = " << ppm << " ppm (" << m_skew
                                                             << "), stream = " << (2000 + runId));
}

void
ClockDriftApplier::SetSkewPpm(double ppm)
{
    m_skew = ppm / 1e6;
}

void
ClockDriftApplier::SetK(double k)
{
    m_K = k;
}

double
ClockDriftApplier::ComputeAdjustedSeconds(double t) const
{
    double stddev = std::sqrt(m_K * t);
    double noise = m_noiseGen->GetValue() * stddev; // epsilon ~ N(0, K*t)

    // Correction formula: T = t * (1 + δ) + ε
    double delay = t * (1.0 + m_skew) + noise;

    // Safety: truncate negative delay to 0 seconds
    delay = std::max(0.0, delay);

    NS_LOG_DEBUG("inputSeconds = " << t << ", skew = " << m_skew << ", noise = " << noise
                                   << ", adjusted = " << delay);
    return delay;
}

double
ClockDriftApplier::GetAdjustedDelay(double t) const
{
    return ComputeAdjustedSeconds(t);
}

Time
ClockDriftApplier::Apply(Time t) const
{
    return Seconds(ComputeAdjustedSeconds(t.GetSeconds()));
}

} // namespace ns3
