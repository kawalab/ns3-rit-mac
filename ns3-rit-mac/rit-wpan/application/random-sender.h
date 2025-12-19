#pragma once

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/lr-wpan-mac.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ns3/net-device.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/time-drift-applier.h"
#include "ns3/traced-callback.h"

namespace ns3
{
namespace lrwpan
{

/**
 * @ingroup lr-wpan
 *
 * @brief A random sender application that generates packets at random intervals.
 *
 * This application sends packets at random intervals (between min and max) to a specific
 * destination address. It's designed to work with standard LrWpan networks for testing and
 * simulation.
 */
class RandomSender : public Application
{
  public:
    /**
     * Default values for RandomSender attributes
     */
    static const Time DEFAULT_MIN_INTERVAL;            //!< Default minimum packet sending interval
    static const Time DEFAULT_MAX_INTERVAL;            //!< Default maximum packet sending interval
    static const Time DEFAULT_INITIAL_DELAY;           //!< Default initial delay
    static constexpr uint8_t DEFAULT_PACKET_SIZE = 20; //!< Default packet size in bytes

    /**
     * @brief Get the TypeId
     *
     * @return The TypeId for this class
     */
    static TypeId GetTypeId(void);

    /**
     * @brief Default constructor
     */
    RandomSender();

    /**
     * @brief Destructor
     */
    ~RandomSender() override;

    /**
     * @brief Set the minimum sending interval
     * @param minInterval The minimum interval between packet transmissions
     */
    void SetMinInterval(Time minInterval);

    /**
     * @brief Set the maximum sending interval
     * @param maxInterval The maximum interval between packet transmissions
     */
    void SetMaxInterval(Time maxInterval);

    /**
     * @brief Set initial delay before starting transmissions
     * @param delay The initial delay
     */
    void SetInitialDelay(Time delay);

    /**
     * @brief Set the size of packets to be sent
     * @param size The packet size in bytes
     */
    void SetPacketSize(uint8_t size);

    /**
     * @brief Set the destination address for packets
     * @param addr The destination address (can be Mac16Address or Mac64Address)
     */
    void SetDstAddr(const Address& addr);

    void SetNoSendMode(bool noSendFlag);

    /**
     * @brief Start the application
     */
    void StartApplication() override;

    /**
     * @brief Stop the application
     */
    void StopApplication() override;

    bool ReceivePacket(Ptr<NetDevice> device,
                       Ptr<const Packet> packet,
                       uint16_t protocol,
                       const Address& sender);

  private:
    /**
     * @brief Send a packet
     */
    void SendPacket();

    /**
     * @brief Get a random interval between min and max
     * @return Random time interval
     */
    Time GetRandomInterval();

    Time m_minInterval;         //!< Minimum packet sending interval
    Time m_maxInterval;         //!< Maximum packet sending interval
    Time m_initialDelay;        //!< Initial delay before starting transmissions
    uint8_t m_packetSize;       //!< Size of packets to send
    Address m_dstAddr;          //!< Destination address (Mac16Address or Mac64Address)
    Ptr<NetDevice> m_netDevice; //!< Network device used for sending
    EventId m_sendEvent;        //!< Event to schedule the next packet sending
    bool m_noSendFlag;          //!< Flag to indicate if the application should not send packets

    Ptr<TimeDriftApplier> m_timeDriftApplier;    //!< For randomizing beacon interval
    Ptr<UniformRandomVariable> m_randomVariable; //!< Random variable for interval generation

    TracedCallback<Ptr<const Packet>> m_txTrace; //!< Trace of transmitted packets
    TracedCallback<Ptr<const Packet>> m_rxTrace; //!< Trace of received packets
};

} // namespace lrwpan
} // namespace ns3
