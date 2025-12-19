/*
 * Copyright (c) 2025 Kanazawa Institute of Technology, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Tomoya Murata <c1039548@st.kanazawa-it.ac.jp>
 */

#ifndef RIT_WPAN_MAC_H
#define RIT_WPAN_MAC_H

#include "ns3/clock-drift-applier.h"
#include "ns3/lr-wpan-mac-header.h"
#include "ns3/lr-wpan-mac.h"
#include "ns3/time-drift-applier.h"

#include <cstdint>

namespace ns3
{
namespace lrwpan
{

class RitWpanPreCs;
class RitWpanPreCsB;

/**
 * @brief Enum representing the MAC operation mode.
 */
enum RitMacMode
{
    RIT_MODE_DISABLED,
    SENDER_MODE,
    RECEIVER_MODE,
    SLEEP_MODE,
    BOOTSTRAP_MODE
};

/**
 * @ingroup lr-wpan
 *
 * MLME-RIT-REQ.indication parameters.
 * See IEEE 802.15.4-2020 8.2.25.1.
 */
struct MlmeRitRequestIndicationParams
{
    uint8_t m_srcAddrMode{0};   //!< Source address mode (NONE=0, SHORT=2, EXTENDED=3)
    uint16_t m_srcPanId{0};     //!< Source PAN ID
    Mac16Address m_srcAddr;     //!< Source short address (if applicable)
    Mac64Address m_srcExtAddr;  //!< Source extended address (if applicable)

    uint8_t m_dstAddrMode{0};   //!< Destination address mode (NONE=0, SHORT=2, EXTENDED=3)
    uint16_t m_dstPanId{0};     //!< Destination PAN ID
    Mac16Address m_dstAddr;     //!< Destination short address (if applicable)
    Mac64Address m_dstExtAddr;  //!< Destination extended address (if applicable)

    std::vector<uint8_t> m_ritRequestPayload; //!< RIT Request Payload field (octet stream)
    // std::vector<HeaderIe> m_headerIeList;   //!< List of header IEs (excluding Termination IE)
    // std::vector<PayloadIe> m_payloadIeList; //!< List of payload IEs (excluding Termination IE)

    uint8_t m_linkQuality{0};   //!< LQI value (0x00 to 0xff)
    uint8_t m_dsn{0};           //!< DSN of the received RIT Data Request command
    uint32_t m_timestamp{0};    //!< Symbol-period timestamp of reception (accurate to 16 symbols)

    uint8_t m_securityLevel{0}; //!< Security level (0x00 to 0x07)
    uint8_t m_keyIdMode{0};     //!< Key ID mode (0x00 to 0x03)
    uint64_t m_keySource{0};    //!< Key source (as in KeyIdMode)
    uint8_t m_keyIndex{0};      //!< Key index
};

/**
 * @ingroup lr-wpan
 *
 * MLME-RIT-REQ.response request parameters.
 * See IEEE 802.15.4-2020 8.2.25.2.
 */
struct MlmeRitResponseRequestParams
{
    uint8_t m_srcAddrMode{0};   //!< Source address mode (NONE=0, SHORT=2, EXTENDED=3)
    uint16_t m_srcPanId{0};     //!< Source PAN ID
    Mac16Address m_srcAddr;     //!< Source short address (if applicable)
    Mac64Address m_srcExtAddr;  //!< Source extended address (if applicable)

    uint8_t m_dstAddrMode{0};   //!< Destination address mode (NONE=0, SHORT=2, EXTENDED=3)
    uint16_t m_dstPanId{0};     //!< Destination PAN ID
    Mac16Address m_dstAddr;     //!< Destination short address (if applicable)
    Mac64Address m_dstExtAddr;  //!< Destination extended address (if applicable)

    std::vector<uint8_t> m_ritResponsePayload; //!< RIT Response Payload field (octet stream)
    // std::vector<HeaderIe> m_headerIeList;   //!< List of header IEs (excluding Termination IE)
    // std::vector<PayloadIe> m_payloadIeList; //!< List of payload IEs (excluding Termination IE)

    uint8_t m_linkQuality{0};   //!< LQI value (0x00 to 0xff)
    uint8_t m_dsn{0};           //!< DSN of the received RIT Data Request command
    uint32_t m_timestamp{0};    //!< Symbol-period timestamp of reception (accurate to 16 symbols)

    uint8_t m_securityLevel{0}; //!< Security level (0x00 to 0x07)
    uint8_t m_keyIdMode{0};     //!< Key ID mode (0x00 to 0x03)
    uint64_t m_keySource{0};    //!< Key source (as in KeyIdMode)
    uint8_t m_keyIndex{0};      //!< Key index
};

/**
 * @ingroup lr-wpan
 *
 * MLME-RIT-RES.indication parameters.
 * See IEEE 802.15.4-2020 8.2.25.3.
 */
struct MlmeRitResponseIndicationParams
{
    uint8_t m_srcAddrMode{0};   //!< Source address mode (NONE=0, SHORT=2, EXTENDED=3)
    uint16_t m_srcPanId{0};     //!< Source PAN ID
    Mac16Address m_srcAddr;     //!< Source short address (if applicable)
    Mac64Address m_srcExtAddr;  //!< Source extended address (if applicable)

    uint8_t m_dstAddrMode{0};   //!< Destination address mode (NONE=0, SHORT=2, EXTENDED=3)
    uint16_t m_dstPanId{0};     //!< Destination PAN ID
    Mac16Address m_dstAddr;     //!< Destination short address (if applicable)
    Mac64Address m_dstExtAddr;  //!< Destination extended address (if applicable)

    std::vector<uint8_t> m_ritResponsePayload; //!< RIT Response Payload field (octet stream)
    // std::vector<HeaderIe> m_headerIeList;   //!< List of header IEs (excluding Termination IE)
    // std::vector<PayloadIe> m_payloadIeList; //!< List of payload IEs (excluding Termination IE)

    uint8_t m_linkQuality{0};   //!< LQI value (0x00 to 0xff)
    uint8_t m_dsn{0};           //!< DSN of the received RIT Data Request command
    uint32_t m_timestamp{0};    //!< Symbol-period timestamp of reception (accurate to 16 symbols)

    uint8_t m_securityLevel{0}; //!< Security level (0x00 to 0x07)
    uint8_t m_keyIdMode{0};     //!< Key ID mode (0x00 to 0x03)
    uint64_t m_keySource{0};    //!< Key source (as in KeyIdMode)
    uint8_t m_keyIndex{0};      //!< Key index
};

/**
 * @ingroup lr-wpan
 *
 * MLME-RIT-RES.confirm parameters.
 * See IEEE 802.15.4-2020 8.2.25.4.
 */
struct MlmeRitResponseConfirmParams
{
    MacStatus m_status{MacStatus::INVALID_PARAMETER};
};

using MlmeRitRequestIndicationCallback =
    Callback<void, MlmeRitRequestIndicationParams>; //!< Callback for MLME-RIT-REQ.indication

using MlmeRitRequestConfirmCallback =
    Callback<void, MacStatus>; //!< Callback for MLME-RIT-REQ.confirm

using MlmeRitResponseIndicationCallback =
    Callback<void, MlmeRitResponseIndicationParams>; //!< Callback for MLME-RIT-RES.indication

using MlmeRitResponseConfirmCallback =
    Callback<void, MacStatus>; //!< Callback for MLME-RIT-RES.confirm

/**
 * @ingroup lr-wpan
 *
 * @brief Configuration flags for RitWpanMac.
 *
 * This structure enables/disables optional MAC mechanisms used in the evaluation.
 */
struct RitWpanMacModuleConfig
{
    // Data transmission options
    bool dataCsmaEnabled = false;
    bool dataPreCsEnabled = false;
    bool dataPreCsBEnabled = false; //!< Enable Pre-CSB for data transmission

    // Beacon transmission options
    bool beaconCsmaEnabled = false;
    bool beaconPreCsEnabled = false;
    bool beaconPreCsBEnabled = false; //!< Enable Pre-CSB for beacon transmission

    // Additional mechanisms
    bool continuousTxEnabled = false;
    bool beaconRandomizeEnabled = false;
    bool compactRitDataRequestEnabled = false;
    bool beaconAckEnabled = false;
};

class RitWpanMac : public LrWpanMac
{
  public:
    static TypeId GetTypeId();
    RitWpanMac();
    ~RitWpanMac() override;

    /**
     * @brief MCPS-DATA.request from upper layer.
     *
     * The packet is enqueued and is not transmitted immediately.
     */
    void McpsDataRequest(McpsDataRequestParams params, Ptr<Packet> p) override;

    /**
     * @brief MLME-SET.request (PIB attribute update) from upper layer.
     * @param id Attribute identifier
     * @param attribute Attribute value
     */
    void MlmeSetRequest(MacPibAttributeIdentifier id, Ptr<MacPibAttributes> attribute) override;

    /**
     * @brief MLME-GET.request from upper layer.
     * @param id Attribute identifier
     */
    void MlmeGetRequest(MacPibAttributeIdentifier id) override;

    /**
     * @brief PLME-SET-TRX-STATE.confirm callback from PHY layer.
     */
    void PlmeSetTRXStateConfirm(PhyEnumeration status) override;

    /**
     * @brief Set the MAC state.
     */
    void SetLrWpanMacState(MacState macState) override;

    /**
     * @brief Set callback for MLME-RIT-REQ.indication.
     */
    void SetMlmeRitRequestIndicationCallback(MlmeRitRequestIndicationCallback c);

    /**
     * @brief PD-DATA.indication callback from PHY layer.
     */
    void PdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi) override;

    /**
     * @brief PD-DATA.confirm callback from PHY layer.
     */
    void PdDataConfirm(PhyEnumeration status) override;

    /**
     * @brief IFS wait timeout handler.
     */
    void IfsWaitTimeout(Time ifsTime) override;

    void SetModuleConfig(const RitWpanMacModuleConfig& config);
    RitWpanMacModuleConfig GetModuleConfig() const;

    /**
     * @brief Set RIT module configuration (alias for SetModuleConfig).
     */
    void SetRitModuleConfig(const RitWpanMacModuleConfig& config);

    void SetPreCs(Ptr<RitWpanPreCs> preCs);

    /**
     * @brief Trigger RIT data transmission (sender-side).
     */
    void SendRitData();

    /**
     * @brief Set receiver always-on behavior (e.g., for a parent node).
     */
    void SetRxAlwaysOn(bool alwaysOn);

    // Time-based parameter getters
    Time GetRitPeriodTime() const;
    Time GetRitDataWaitDurationTime() const;
    Time GetRitTxWaitDurationTime() const;

    /**
     * @brief Duration of the last data transmission round-trip.
     *
     * Used for continuous transmission timeout calculation.
     */
    Time m_lastDataTxDuration;

    /**
     * @brief Start time of the last data transmission.
     */
    Time m_lastDataTxStartTime;

  private:
    void DoInitialize() override;
    void DoDispose() override;

    void Sleep();
    void Bootstrap();
    void RelayRequest(Ptr<Packet> relayPkt);

    void ChangeRitMacMode(RitMacMode ritMacMode);

    void PeriodicRitDataRequest(); //!< Periodic RIT data request in sender mode
    void DoSendRitData();          //!< Process RIT data transmission
    void DoSendRitDataRequest();   //!< Send RIT Data Request command
    void DoSendRitBeaconAck();     //!< Send RIT Beacon Acknowledgment command

    void ReceiveCommand(uint8_t lqi, Ptr<Packet> p);
    void ReceiveData(uint8_t lqi, Ptr<Packet> p);

    void StartRitDataWaitPeriod();
    void StartRitTxWaitPeriod();

    /**
     * @brief Start the RIT cycle (request, waiting, etc.).
     */
    void StartRitCycle();

    /**
     * @brief Stop the RIT cycle.
     */
    void StopRitCycle();

    void SenderCycleTimeout();   //!< Timeout for sender cycle
    void EndSenderCycle();       //!< End sender cycle after transmission
    void ReceiverCycleTimeout(); //!< Timeout for receiver cycle
    void EndReceiverCycle();     //!< End receiver cycle after reception

    /**
     * @brief ACK wait timeout handler.
     */
    void AckWaitTimeout() override;

    void SetWakeUp(); //!< Set MAC into wake-up state
    void SetSleep();  //!< Set MAC into sleep state

    bool CheckTxAndStartSender();
    bool IsRitModeEnabled() const;
    Time DurationToTime(uint64_t duration) const;

    /**
     * @brief Get timeout duration for continuous transmission mode.
     *
     * The timeout can be adjusted using module configuration and internal state.
     */
    Time GetContinuousTxTimeoutTime() const;

    /* Member variables */

    // Behavior flags
    bool m_rxAlwaysOn;            //!< Receiver always-on flag (e.g., for a parent device)
    bool m_continuousRxEnabled;   //!< Continuous reception enable flag

    bool m_useTimeBasedRitParams = true; //!< Use time-based RIT parameters
    bool m_ritSending = false;           //!< Whether RIT data is currently being sent

    // RIT parameters (IEEE 802.15.4-2020)
    TracedValue<uint32_t> m_macRitPeriod; //!< RIT interval (0x000000 ~ 0xFFFFFF)
    TracedValue<uint8_t> m_macRitDataWaitDuration; //!< RX wait after RIT (0x00 ~ 0xFF)
    TracedValue<uint32_t> m_macRitTxWaitDuration; //!< Beacon wait (>= macRitPeriod, up to 0xFFFFFF)
    std::vector<uint8_t> m_macRitRequestPayload; //!< Payload for RIT command transmission

    // Time-based RIT parameters
    TracedValue<Time> m_macRitPeriodTime;           //!< RIT period time
    TracedValue<Time> m_macRitDataWaitDurationTime; //!< RIT data wait duration time
    TracedValue<Time> m_macRitTxWaitDurationTime;   //!< RIT transmission wait duration time

    // RIT mode
    TracedValue<RitMacMode> m_ritMacMode; //!< Current RIT MAC mode

    // RIT events
    EventId m_ritDataWaitTimeout;          //!< Data wait timeout event
    EventId m_ritTxWaitTimeout;            //!< TX wait timeout event
    EventId m_periodicRitDataRequestEvent; //!< Periodic data request event

    MlmeRitRequestIndicationCallback m_mlmeRitRequestIndicationCallback; //!< MLME-RIT-REQ.indication

    Mac16Address m_lastRxRitReqFrameSrcAddr; //!< Source address of last received RIT request frame

    Ptr<TimeDriftApplier> m_timeDriftApplier;   //!< Used for beacon interval randomization
    Ptr<ClockDriftApplier> m_clockDriftApplier; //!< Used for clock drift correction

    Ptr<RitWpanPreCs> m_preCs;     //!< Pre-CS implementation
    Ptr<RitWpanPreCsB> m_preCsB;   //!< Pre-CSB implementation

    RitWpanMacModuleConfig m_moduleConfig;

    // Trace: MAC timeout events
    TracedCallback<Time, std::string> m_macTimeoutEventTrace;

    // Trace values for timeout durations
    TracedValue<Time> m_beaconWaitTimeoutTime;
    TracedValue<Time> m_dataWaitTimeoutTime;

    // Measurement: start times for waiting
    Time m_beaconWaitStartTime;
    Time m_dataWaitStartTime;

    // Trace: measured waiting durations
    TracedCallback<std::string, Time> m_beaconWaitTrace;
    TracedCallback<std::string, Time> m_dataWaitTrace;
};

} // namespace lrwpan
} // namespace ns3

#endif /* RIT_WPAN_MAC_H */
