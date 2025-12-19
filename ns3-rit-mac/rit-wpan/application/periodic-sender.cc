#include "periodic-sender.h"

#include "ns3/log.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("PeriodicSender");
NS_OBJECT_ENSURE_REGISTERED(PeriodicSender);

// Define static members
const Time PeriodicSender::DEFAULT_INTERVAL = Seconds(60);
const Time PeriodicSender::DEFAULT_INITIAL_DELAY = Seconds(0);

TypeId
PeriodicSender::GetTypeId()
{
    static TypeId tid = TypeId("ns3::lrwpan::PeriodicSender")
                            .SetParent<Application>()
                            .SetGroupName("LrWpan")
                            .AddConstructor<PeriodicSender>()
                            .AddAttribute("Interval",
                                          "The interval between packet sends",
                                          TimeValue(DEFAULT_INTERVAL),
                                          MakeTimeAccessor(&PeriodicSender::m_interval),
                                          MakeTimeChecker())
                            .AddAttribute("PacketSize",
                                          "Size of packets sent",
                                          UintegerValue(DEFAULT_PACKET_SIZE),
                                          MakeUintegerAccessor(&PeriodicSender::m_packetSize),
                                          MakeUintegerChecker<uint8_t>())
                            .AddAttribute("DstAddress",
                                          "The destination Address",
                                          AddressValue(), // Use default constructor
                                          MakeAddressAccessor(&PeriodicSender::m_dstAddr),
                                          MakeAddressChecker())
                            .AddAttribute("InitialDelay",
                                          "Initial delay before starting transmissions",
                                          TimeValue(DEFAULT_INITIAL_DELAY),
                                          MakeTimeAccessor(&PeriodicSender::m_initialDelay),
                                          MakeTimeChecker())
                            .AddTraceSource("Tx",
                                            "A packet has been sent",
                                            MakeTraceSourceAccessor(&PeriodicSender::m_txTrace),
                                            "ns3::Packet::TracedCallback")
                            .AddTraceSource("Rx",
                                            "A packet has been received",
                                            MakeTraceSourceAccessor(&PeriodicSender::m_rxTrace),
                                            "ns3::Packet::TracedCallback");
    return tid;
}

PeriodicSender::PeriodicSender()
    : m_interval(DEFAULT_INTERVAL),
      m_initialDelay(DEFAULT_INITIAL_DELAY),
      m_packetSize(DEFAULT_PACKET_SIZE),
      m_dstAddr(),
      m_netDevice(nullptr),
      m_noSendFlag(false)
{
    NS_LOG_FUNCTION(this);
    m_timeDriftApplier = CreateObject<TimeDriftApplier>();
}

PeriodicSender::~PeriodicSender()
{
    NS_LOG_FUNCTION(this);
}

void
PeriodicSender::SetInterval(Time interval)
{
    NS_LOG_FUNCTION(this << interval);
    m_interval = interval;
}

void
PeriodicSender::SetInitialDelay(Time delay)
{
    NS_LOG_FUNCTION(this << delay);
    m_initialDelay = delay;
}

void
PeriodicSender::SetPacketSize(uint8_t size)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(size));
    m_packetSize = size;
}

void
PeriodicSender::SetDstAddr(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    // Verify that the address is either Mac16Address or Mac64Address
    if (Mac16Address::IsMatchingType(addr) || Mac64Address::IsMatchingType(addr))
    {
        m_dstAddr = addr;
    }
    else
    {
        NS_FATAL_ERROR("Address must be either Mac16Address or Mac64Address");
    }
}

void
PeriodicSender::SetNoSendMode(bool noSendFlag)
{
    NS_LOG_FUNCTION(this << noSendFlag);
    m_noSendFlag = noSendFlag;
}

void
PeriodicSender::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_noSendFlag)
    {
        NS_LOG_DEBUG("PeriodicSender is in no-send mode on node " << GetNode()->GetId());
        return; // Do not proceed if in no-send mode
    }

    NS_ASSERT(!m_dstAddr.IsInvalid());

    m_netDevice = m_node->GetDevice(0);
    if (m_netDevice == nullptr)
    {
        NS_LOG_ERROR("No LrWpan device found on node " << m_node->GetId());
        return;
    }

    if (!m_noSendFlag)
    {
        NS_LOG_DEBUG("(App Params)[nodeID: "
                     << m_node->GetId() << "] Interval=" << m_interval.GetSeconds() << "s, "
                     << "Initial Delay=" << m_initialDelay.GetSeconds() << "s, "
                     << "DstAddress=" << m_dstAddr << ", "
                     << "PktSize=" << static_cast<int>(m_packetSize));

        Simulator::Cancel(m_sendEvent); // Cancel any pending events
        m_sendEvent = Simulator::Schedule(m_initialDelay, &PeriodicSender::SendPacket, this);
    }
    else
    {
        NS_LOG_DEBUG("PeriodicSender is in no-send mode on node " << m_node->GetId());
    }
}

void
PeriodicSender::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_sendEvent.IsPending())
    {
        Simulator::Cancel(m_sendEvent);
    }

    // Reset for potential future restart
    m_netDevice = nullptr;
}

void
PeriodicSender::SendPacket()
{
    NS_LOG_FUNCTION(this);

    if (!m_netDevice)
    {
        NS_LOG_ERROR("Cannot send packet: network device not available");
        return;
    }

    // Create a new packet with the configured size
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    bool sendRequestIssued = false;

    // Send packet to the configured destination address (Mac16Address or Mac64Address).
    if (Mac16Address::IsMatchingType(m_dstAddr))
    {
        sendRequestIssued = m_netDevice->Send(packet, Mac16Address::ConvertFrom(m_dstAddr), 0);
    }
    else if (Mac64Address::IsMatchingType(m_dstAddr))
    {
        sendRequestIssued = m_netDevice->Send(packet, Mac64Address::ConvertFrom(m_dstAddr), 0);
    }
    else
    {
        NS_FATAL_ERROR("Unsupported address type: " << m_dstAddr);
    }

    if (sendRequestIssued)
    {
        NS_LOG_INFO("[App->NetDev]:At "
                    << Simulator::Now().GetSeconds() << "s node " << GetNode()->GetId()
                    << " issued send request for packet with size "
                    << static_cast<uint32_t>(m_packetSize) << " bytes to " << m_dstAddr);
        m_txTrace(packet);
    }
    else
    {
        NS_LOG_ERROR("Failed to issue send request from node " << GetNode()->GetId());
    }

    // Schedule the next packet transmission
    // TODO: ランダム時間を外部から設定
    m_sendEvent = Simulator::Schedule(m_timeDriftApplier->ApplyByRatio(m_interval, 1),
                                      &PeriodicSender::SendPacket,
                                      this);
}

bool
PeriodicSender::ReceivePacket(Ptr<NetDevice> device,
                              Ptr<const Packet> packet,
                              uint16_t protocol,
                              const Address& sender)
{
    NS_LOG_FUNCTION(this << device << packet << protocol << sender);
    NS_LOG_INFO("[NetDev->App]:At " << Simulator::Now().GetSeconds() << "s node "
                                    << GetNode()->GetId() << " received packet from " << sender);
    m_rxTrace(packet);
    return true;
}

} // namespace lrwpan
} // namespace ns3
