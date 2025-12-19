/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef TIME_DRIFT_APPLIER_H
#define TIME_DRIFT_APPLIER_H

#include <ns3/log.h>
#include <ns3/nstime.h>
#include <ns3/object.h>
#include <ns3/random-variable-stream.h>

namespace ns3
{

class TimeDriftApplier : public Object
{
  public:
    TimeDriftApplier();
    void SetDriftRatio(double driftRatio);
    Time ApplyByRatio(Time inputTime) const;
    Time ApplyByRatio(Time inputTime, double driftRatio) const;

  private:
    double m_driftRatio; // percent (e.g. 10.0 means ±10%)
    Ptr<UniformRandomVariable> m_rng;
};

} // namespace ns3

#endif // TIME_DRIFT_APPLIER_H
