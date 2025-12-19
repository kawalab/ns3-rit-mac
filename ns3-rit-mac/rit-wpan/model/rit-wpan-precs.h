/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef RIT_WPAN_PRECS_H
#define RIT_WPAN_PRECS_H

#include "rit-wpan-mac.h"

#include "ns3/event-id.h"
#include "ns3/object.h"

namespace ns3
{
namespace lrwpan
{

/**
 * @ingroup lr-wpan
 *
 * This method informs the MAC whether the channel is idle or busy for Pre-CS.
 */
typedef Callback<void, MacState> LrWpanMacStateCallback;

/**
 * @ingroup lr-wpan
 *
 * This method implements the PD SAP: PlmeCcaConfirm
 *
 * @param status the status of CCA
 */
typedef Callback<void, PhyEnumeration> FallbackCcaConfirmCallback;

/**
 * @ingroup lr-wpan
 *
 * @brief Pre-CS carrier sense access control class for IEEE 802.15.4e RIT
 *
 * This class implements a simplified carrier sense strategy (Pre-CS),
 * designed specifically for sending RIT Data Request frames.
 *
 * Characteristics:
 *  - Only one CCA attempt (no CW, no retry)
 *  - No backoff, no slotted operation
 *  - If channel is busy, notify failure immediately
 *  - If channel is idle, notify MAC to proceed transmission
 *
 * Intended to reduce power and delay overhead when sending lightweight control frames.
 */
class RitWpanPreCs : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor.
     */
    RitWpanPreCs();

    /**
     * Destructor.
     */
    ~RitWpanPreCs() override;

    /**
     * Set the MAC to which this Pre-CS implementation is attached to.
     *
     * @param mac the used MAC
     */
    void SetMac(Ptr<RitWpanMac> mac);

    /**
     * Get the MAC to which this Pre-CS implementation is attached to.
     *
     * @return the used MAC
     */
    Ptr<RitWpanMac> GetMac() const;

    /**
     * Set the callback function to the MAC. Used at the end of a Channel Assessment, as part of the
     * interconnections between the Pre-CS and the MAC. The callback
     * lets MAC know a channel is either idle or busy.
     *
     * @param macState the mac state callback
     */
    void SetLrWpanMacStateCallback(LrWpanMacStateCallback macState);

    void SetFallbackCcaConfirmCallback(FallbackCcaConfirmCallback fallbackCcaConfirmCallback);

    /**
     * Start Pre-CS algorithm (immediate CCA request).
     * Performs a single carrier sense attempt without backoff or retry.
     */
    void Start();

    /**
     * Cancel Pre-CS algorithm.
     * Cancels any ongoing CCA request.
     */
    void Cancel();

    /**
     * Request the Phy to perform CCA (single attempt)
     */
    void RequestCCA();

    /**
     * IEEE 802.15.4-2006 section 6.2.2.2
     * PLME-CCA.confirm status
     * @param status TRX_OFF, BUSY or IDLE
     *
     * When Phy has completed CCA, it calls back here which executes the final step
     * of the Pre-CS algorithm.
     * It checks if the Channel is idle and immediately notifies the MAC.
     * If channel is busy, notifies channel access failure immediately.
     */
    void PlmeCcaConfirm(PhyEnumeration status);

  private:
    void DoDispose() override;

    /**
     * The callback to inform the configured MAC of the Pre-CS result.
     */
    LrWpanMacStateCallback m_lrWpanMacStateCallback;

    FallbackCcaConfirmCallback m_fallbackCcaConfirmCallback;

    /**
     * The MAC instance for which this Pre-CS implementation is configured.
     */
    Ptr<RitWpanMac> m_mac;

    /**
     * Flag indicating that the PHY is currently running a CCA. Used to prevent
     * reporting the channel status to the MAC while canceling the Pre-CS algorithm.
     */
    bool m_ccaRequestRunning;
};

} // namespace lrwpan
} // namespace ns3

#endif /* LR_WPAN_PRECS_H */
