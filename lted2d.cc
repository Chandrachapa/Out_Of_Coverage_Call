#include "ns3/lte-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store.h"
#include <cfloat>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <numeric>
#include <typeinfo>
#include <cmath>

#include "ns3/epc-helper.h"
#include "ns3/netanim-module.h"
/*
#include "ns3/ns2-mobility-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-address.h"*/
//#include "ns3/packet-sink-helper.h"
//#include "ns3/udp-client-server-helper.h"



using namespace ns3;

// This trace will log packet transmissions and receptions from the application
// layer.  The parameter 'localAddrs' is passed to this trace in case the
// address passed by the trace is not set (i.e., is '0.0.0.0' or '::').  The
// trace writes to a file stream provided by the first argument; by default,
// this trace file is 'UePacketTrace.tr'
void
UePacketTrace (Ptr<OutputStreamWrapper> stream, const Address &localAddrs, std::string context, Ptr<const Packet> p, const Address &srcAddrs, const Address &dstAddrs)
{
  std::ostringstream oss;
  *stream->GetStream () << Simulator::Now ().GetNanoSeconds () / (double) 1e9 << "\t" << context << "\t" << p->GetSize () << "\t";
  if (InetSocketAddress::IsMatchingType (srcAddrs))
    {
      oss << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 ();
      if (!oss.str ().compare ("0.0.0.0")) //srcAddrs not set
        {
          *stream->GetStream () << Ipv4Address::ConvertFrom (localAddrs) << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
        }
      else
        {
          oss.str ("");
          oss << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 ();
          if (!oss.str ().compare ("0.0.0.0")) //dstAddrs not set
            {
              *stream->GetStream () << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" <<  Ipv4Address::ConvertFrom (localAddrs) << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
          else
            {
              *stream->GetStream () << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (srcAddrs))
    {
      oss << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 ();
      if (!oss.str ().compare ("::")) //srcAddrs not set
        {
          *stream->GetStream () << Ipv6Address::ConvertFrom (localAddrs) << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
        }
      else
        {
          oss.str ("");
          oss << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 ();
          if (!oss.str ().compare ("::")) //dstAddrs not set
            {
              *stream->GetStream () << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Ipv6Address::ConvertFrom (localAddrs) << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
          else
            {
              *stream->GetStream () << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
        }
    }
  else
    {
      *stream->GetStream () << "Unknown address type!" << std::endl;
    }
}

/*
 * The topology is the following:
 *
 *    UE2 (-10.0, 0.0, 1.5).....(10 m).....eNB (0.0, 0.0, 30.0).....(10 m).....UE1 (10.0, 0.0, 1.5)
 *
 * Please refer to the Sidelink section of the LTE user documentation for more details.
 *
 */

NS_LOG_COMPONENT_DEFINE ("LteSlInCovrgCommMode1");

int main (int argc, char *argv[])
{
  Time simTime = Seconds (10);
  bool enableNsLogs = false;
  bool useIPv6 = false;
  int ueNodeNum =25;
  bool useCa = false;
  int m_nodeSpeed = 20;
  int m_nodePause = 0;

  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);
  cmd.AddValue ("useIPv6", "Use IPv6 instead of IPv4", useIPv6);
  cmd.Parse (argc, argv);

  // Configure the scheduler
  Config::SetDefault ("ns3::RrSlFfMacScheduler::Itrp", UintegerValue (0));
  //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::RrSlFfMacScheduler::SlGrantSize", UintegerValue (5));
  
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(4194304));
  //Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (40.0));
  Config::SetDefault ("ns3::LteEnbPhy::NoiseFigure", DoubleValue (5.0));
  Config::SetDefault ("ns3::LteUePhy::NoiseFigure", DoubleValue (7.0));
  //Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (20.0));
  Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::MiErrorModel));

  //Set the frequency

  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (2100));
  Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn", UintegerValue (2100));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (20000));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (50));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (50));
  
//  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1100));
//  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (6291456));
//  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (4194304));
//  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));

//  Config::SetDefault ("ns3::QueueBase::MaxSize", QueueSizeValue (QueueSize ("20000p")));

//  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType", TypeIdValue (TypeId::LookupByName (recovery)));

  Config::SetDefault ("ns3::WeibullRandomVariable::Scale", DoubleValue(6.17702));
  Config::SetDefault ("ns3::WeibullRandomVariable::Shape", DoubleValue(10.2063));

  Config::SetDefault ("ns3::LogNormalRandomVariable::Mu", DoubleValue(0.4026));
  Config::SetDefault ("ns3::LogNormalRandomVariable::Sigma", DoubleValue(0.0352));

  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (40));

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
   }


  // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));


  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  // parse again so we can override input file default values via command line
  cmd.Parse (argc, argv);

  if (enableNsLogs)
    {
      LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);

      LogComponentEnable ("LteUeRrc", logLevel);
      LogComponentEnable ("LteUeMac", logLevel);
      LogComponentEnable ("LteSpectrumPhy", logLevel);
      LogComponentEnable ("LteUePhy", logLevel);
      LogComponentEnable ("LteEnbPhy", logLevel);
    }

  //Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (20.0));
  //Set the eNBs power in dBm
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (40.0));

  //Sidelink bearers activation time
  Time slBearersActivationTime = Seconds (2.0);

  //Create the helpers
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  //Create and set the EPC helper
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ////Create Sidelink helper and set lteHelper
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  //Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"));
  lteHelper->SetPathlossModelType (ns3::FriisPropagationLossModel::GetTypeId ());

  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));

  //Sidelink Round Robin scheduler
  lteHelper->SetSchedulerType ("ns3::RrSlFfMacScheduler");

  //Create nodes (eNb + UEs)
  NodeContainer enbNode;
  enbNode.Create (1);
  //NS_LOG_INFO ("eNb node id = [" << enbNode.Get (0)->GetId () << "]");
  std::cout << "eNB node id =[" << enbNode.Get (0)->GetId () << "]";

    //Positioning the nodes 
  Ptr<ListPositionAllocator> eNbpositionAlloc = CreateObject<ListPositionAllocator> ();
  eNbpositionAlloc->Add (Vector (25, 25 , 7.5));
  //Setup the mobility and position of the nodes 
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(eNbpositionAlloc);
  mobility.Install (enbNode);
 

  NodeContainer ueNodes;
  ueNodes.Create (ueNodeNum);

  //for( int i=0; i < ueNodeNum ; ++i)
  //{
  //  NS_LOG_INFO ("UE"<< i << " node id = [" << ueNodes.Get (i)->GetId () << "]");
    //NS_LOG_INFO ("UE 2 node id = [" << ueNodes.Get (1)->GetId () << "]");
  //}
  
  for (uint16_t i=0; i< ueNodeNum  ; ++i)
  {
	std::cout << "ip address of ue node " << i<< ": "<< ueNodes.Get (i)->GetId () <<std::endl;
  }

   MobilityHelper mobilityAdhoc;

   ObjectFactory pos;
   pos.SetTypeId ("ns3::RandomBoxPositionAllocator");
   pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));
   pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
   // we need antenna height uniform [1.0 .. 2.0] for loss model
   pos.Set ("Z", StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"));

   Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
      //m_streamIndex += taPositionAlloc->AssignStreams (m_streamIndex);

   std::stringstream ssSpeed;
   ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << m_nodeSpeed << "]";
   std::stringstream ssPause;
   ssPause << "ns3::ConstantRandomVariable[Constant=" << m_nodePause << "]";
   mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                      "Speed", StringValue (ssSpeed.str ()),
                                      "Pause", StringValue (ssPause.str ()),
                                      "PositionAllocator", PointerValue (taPositionAlloc));
   mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
   mobilityAdhoc.Install (ueNodes);


  //Install LTE devices to the nodes and fix the random number stream
  int64_t randomStream = 1;
  NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNode);
  randomStream += lteHelper->AssignStreams (enbDevs, randomStream);
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);
  randomStream += lteHelper->AssignStreams (ueDevs, randomStream);

  //Configure Sidelink Pool
  Ptr<LteSlEnbRrc> enbSidelinkConfiguration = CreateObject<LteSlEnbRrc> ();
  enbSidelinkConfiguration->SetSlEnabled (true);

  //Preconfigure pool for the group
  LteRrcSap::SlCommTxResourcesSetup pool;
  pool.setup = LteRrcSap::SlCommTxResourcesSetup::SCHEDULED;

  //BSR timers
  pool.scheduled.macMainConfig.periodicBsrTimer.period = LteRrcSap::PeriodicBsrTimer::sf16;//sf is spreading factor
  pool.scheduled.macMainConfig.retxBsrTimer.period = LteRrcSap::RetxBsrTimer::sf640;
  //MCS
  pool.scheduled.haveMcs = true;
  pool.scheduled.mcs = 16; // modulation and coding scheme.

  //resource pool
  LteSlResourcePoolFactory pfactory;
  pfactory.SetHaveUeSelectedResourceConfig (false); //since we want eNB to schedule

  //Control
  pfactory.SetControlPeriod ("sf40");
  pfactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
  pfactory.SetControlOffset (0);
  pfactory.SetControlPrbNum (22);
  pfactory.SetControlPrbStart (0);
  pfactory.SetControlPrbEnd (49);

  //Data: The ns3::RrSlFfMacScheduler is responsible to handle the parameters


  pool.scheduled.commTxConfig = pfactory.CreatePool ();

  uint32_t groupL2Address = 255;

  enbSidelinkConfiguration->AddPreconfiguredDedicatedPool (groupL2Address, pool);
  lteHelper->InstallSidelinkConfiguration (enbDevs, enbSidelinkConfiguration);

  //pre-configuration for the UEs
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);
  LteRrcSap::SlPreconfiguration preconfiguration;
  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);

  InternetStackHelper internet;
  internet.Install (ueNodes);
  Ipv4Address groupAddress4 ("225.0.0.0");     //use multicast address as destination
  Ipv6Address groupAddress6 ("ff0e::1");     //use multicast address as destination
  Address remoteAddress;
  Address localAddress;
  Ptr<LteSlTft> tft;
  if (!useIPv6)
    {
      //Install the IP stack on the UEs and assign IP address
      Ipv4InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

      // set the default gateway for the UE
      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
        {
          Ptr<Node> ueNode = ueNodes.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
        }
      remoteAddress = InetSocketAddress (groupAddress4, 8000);
      localAddress = InetSocketAddress (Ipv4Address::GetAny (), 8000);
      tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress4, groupL2Address);
    }
  else
    {
      Ipv6InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv6Address (NetDeviceContainer (ueDevs));

      // set the default gateway for the UE
      Ipv6StaticRoutingHelper Ipv6RoutingHelper;
      for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
        {
          Ptr<Node> ueNode = ueNodes.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv6StaticRouting> ueStaticRouting = Ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
        }
      remoteAddress = Inet6SocketAddress (groupAddress6, 8000);
      localAddress = Inet6SocketAddress (Ipv6Address::GetAny (), 8000);
      tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress6, groupL2Address);
    }

  //Attach each UE to the best available eNB
  lteHelper->Attach (ueDevs);

  ///*** Configure applications ***///

  //Set Application in the UEs
  OnOffHelper sidelinkClient ("ns3::UdpSocketFactory", remoteAddress);
  sidelinkClient.SetConstantRate (DataRate ("16kb/s"), 200);

  ApplicationContainer clientApps = sidelinkClient.Install (ueNodes.Get (0));
  //onoff application will send the first packet at :
  //(2.9 (App Start Time) + (1600 (Pkt size in bits) / 16000 (Data rate)) = 3.0 sec
  clientApps.Start (slBearersActivationTime + Seconds (0.9));
  clientApps.Stop (simTime - slBearersActivationTime + Seconds (1.0));

  ApplicationContainer serverApps;
  PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory", localAddress);
  serverApps = sidelinkSink.Install (ueNodes.Get (1));
  serverApps.Start (Seconds (2.0));

  proseHelper->ActivateSidelinkBearer (slBearersActivationTime, ueDevs, tft);
  ///*** End of application configuration ***///

  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("UePacketTrace.tr");

  //Trace file table header
  *stream->GetStream () << "time(sec)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;

  std::ostringstream oss;
  for (uint16_t i=0; i< ueNodes.GetN()  ; i++)
{
  if (!useIPv6)
    {
      // Set Tx traces
      for (uint16_t ac = 0; ac < clientApps.GetN (); ac++)
        {
          Ipv4Address localAddrs =  clientApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
          std::cout << "Tx address: " << localAddrs << std::endl;
          oss << "tx\t" << ueNodes.Get (i)->GetId () << "\t" << ueNodes.Get (0)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          clientApps.Get (ac)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }

      // Set Rx traces
      for (uint16_t ac = 0; ac < serverApps.GetN (); ac++)
        {
          Ipv4Address localAddrs =  serverApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
          std::cout << "Rx address: " << localAddrs << std::endl;
          oss << "rx\t" << ueNodes.Get (i)->GetId () << "\t" << ueNodes.Get (1)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          serverApps.Get (ac)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }
    }
  else
    {
      // Set Tx traces
      for (uint16_t ac = 0; ac < clientApps.GetN (); ac++)
        {
          clientApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->AddMulticastAddress (groupAddress6);
          Ipv6Address localAddrs =  clientApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->GetAddress (1,1).GetAddress ();
          std::cout << "Tx address: " << localAddrs << std::endl;
          oss << "tx\t" << ueNodes.Get (i)->GetId () << "\t" << ueNodes.Get (0)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          clientApps.Get (ac)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }

      // Set Rx traces
      for (uint16_t ac = 0; ac < serverApps.GetN (); ac++)
        {
          serverApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->AddMulticastAddress (groupAddress6);
          Ipv6Address localAddrs =  serverApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->GetAddress (1,1).GetAddress ();
          std::cout << "Rx address: " << localAddrs << std::endl;
          oss << "rx\t" << ueNodes.Get (i)->GetId () << "\t" << ueNodes.Get (1)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          serverApps.Get (ac)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }
    }
} 

  NS_LOG_INFO ("Enabling Sidelink traces...");

  lteHelper->EnableSlPscchMacTraces ();
  lteHelper->EnableSlPsschMacTraces ();

  lteHelper->EnableSlRxPhyTraces ();
  lteHelper->EnableSlPscchRxPhyTraces ();

  NS_LOG_INFO ("Starting simulation...");
  Ptr<Node> pgw = epcHelper->GetPgwNode();
  Ptr<Node> sgw = epcHelper->GetSgwNode();

  Simulator::Stop (simTime);
  AnimationInterface anim("ltetryd2d.xml");
  anim.SetConstantPosition (pgw, 0,0,0);
  anim.SetConstantPosition (sgw, 1,0,0);
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;

}