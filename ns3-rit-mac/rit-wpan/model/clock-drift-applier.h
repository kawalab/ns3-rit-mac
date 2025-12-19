/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef CLOCK_DRIFT_APPLIER_H
#define CLOCK_DRIFT_APPLIER_H

#include <ns3/core-module.h>
#include <ns3/nstime.h>
#include <ns3/random-variable-stream.h>

namespace ns3
{

class ClockDriftApplier : public Object
{
  public:
    ClockDriftApplier();

    /**
     * Initialize: set skew and noise based on node ID and run ID
     */
    void Initialize(uint32_t nodeId, uint32_t runId);

    /**
     * Return the global time difference corresponding to n seconds in local time (double version)
     */
    double GetAdjustedDelay(double nSeconds) const;

    /**
     * Return the global time difference corresponding to inputTime seconds in local time (Time version)
     */
    Time Apply(Time inputTime) const;

    /**
     * Explicitly set skew (ppm) from outside (if omitted, uniform random in ±20 ppm)
     */
    void SetSkewPpm(double ppm);

    void SetSkewRange(double minPpm, double maxPpm);

    /**
     * Set the intensity K of the random-walk noise from outside
     */
    void SetK(double k);

  private:
    /**
     * Common logic: return adjusted seconds
     */
    double ComputeAdjustedSeconds(double inputSeconds) const;

    double m_skew;                        // Ratio (ppm / 1e6)
    double m_K;                           // Random-walk intensity (linear coefficient of variance)
    Ptr<NormalRandomVariable> m_noiseGen; // N(0,1) random number generator

    double m_minSkewPpm = -20.0;
    double m_maxSkewPpm = 20.0;
};

} // namespace ns3

#endif // CLOCK_DRIFT_APPLIER_H
