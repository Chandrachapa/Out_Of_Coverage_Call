/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * NIST-developed software is provided by NIST as a public
 * service. You may use, copy and distribute copies of the software in
 * any medium, provided that you keep intact this entire notice. You
 * may improve, modify and create derivative works of the software or
 * any portion of the software, and you may copy and distribute such
 * modifications or works. Modified works should carry a notice
 * stating that you changed the software and should note the date and
 * nature of any such change. Please explicitly acknowledge the
 * National Institute of Standards and Technology as the source of the
 * software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES
 * NO WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY
 * OPERATION OF LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * NON-INFRINGEMENT AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR
 * WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED
 * OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST DOES NOT
 * WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE
 * SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE
 * CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of
 * using and distributing the software and you assume all risks
 * associated with its use, including but not limited to the risks and
 * costs of program errors, compliance with applicable laws, damage to
 * or loss of data, programs or equipment, and the unavailability or
 * interruption of operation. This software is not intended to be used
 * in any situation where a failure could cause risk of injury or
 * damage to property. The software developed by NIST employees is not
 * subject to copyright protection within the United States.
 */


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
#include <math.h>
#include "ns3/gnuplot.h"
#include "ns3/netanim-module.h"
#include <ns3/mcptt-helper.h>
#include <ns3/wifi-module.h>


using namespace ns3;

/*
 * Scenario with configurable number of Relay UEs and Remote UEs organized in
 * a cluster topology. The Remote UEs can be configured to be in-coverage or
 * out-of-coverage, and the traffic can be configured to flow either between
 * Remote UEs and a RemoteHost in the Internet, or between Remote UEs.
 *
 * Topology:
 * The 'nRelayUes' Relay UEs are deployed around the eNB uniformly on a
 * circle of radius 'relayRadius' meters.
 * Each Relay UE has a cluster of 'nRemoteUesPerRelay' Remote UEs deployed
 * around itself uniformly on a circle of radius 'remoteRadius' meters.
 *
 * The parameter 'remoteUesOoc' set to true indicates the Remote UEs are
 * out-of-coverage (not attached to the eNodeB and use SL preconfiguration).
 *
 * One-to-one connection:
 * The cluster of Remote UEs around a given Relay UE are interested only in its
 * Relay Service Code and thus will connect only to that Relay UE.
 * The UEs start their relay service sequentially in time. First the Relay UE,
 * then the cluster of Remote UEs associated to that Relay UE (sequentially as
 * well), then the next Relay UE, and so on.
 *
 * Traffic:
 * Each Remote UE sends packets to a given node in the network, which echoes
 * back each packet to the Remote UE, showcasing both upward and downward
 * traffic through the Relay UE.
 * The parameter 'echoServerNode' determines the node towards which the Remote
 * UEs send their traffic: either a Remote Host in the Internet (when set to
 * 'RemoteHost') or the first Remote UE connected to the first Relay UE (when
 * set to 'RemoteUE').
 * Each transmitting Remote UE starts sending traffic 3.00 s after the start
 * of the one-to-one connection procedure with its Relay UE and remain active
 * during 10.0 s. The simulation time is calculated so that the last Remote UE
 * can have its 10.0s of traffic activity.
 *
 * Scenario outputs:
 * - AppPacketTrace.txt: Log of application layer transmitted and received
 *                       packets
 * - topology.plt: gnuplot script to plot the topology of the scenario.
 * - PC5SignalingPacketTrace.txt: Log of the received PC5 signaling messages
 * - DlPdcpStats.txt: PDCP layer statistics for the DL
 * - UlPdcpStats.txt: PDCP layer statistics for the UL
 *
 * Users can generate the topology plot by running:
 * $ gnuplot topology.plt
 * It will generate the file 'topology.png' with a plot of the eNodeB,
 * Relay UEs, and Remote UEs positions.
 * Users can track the one-to-one communication setup procedure between Remote
 * and Relay UEs by checking the corresponding PC5 signaling messages exchange
 * in the file 'PC5SignalingPacketTrace.txt'.
 * Users can verify the traffic flows on the file 'AppPacketTrace.txt' and also
 * can verify which nodes are using the network UL/DL in 'DlPdcpStats.txt' and
 * 'UlPdcpStats.txt'. If the one-to-one connections were successfully
 * established and maintained, only the Relay UEs should be using the UL/DL
 * to transmit and receive the Remote UEs relayed traffic.
 */

NS_LOG_COMPONENT_DEFINE ("lte-sl-relay-cluster");

/*
 * Trace sink function for logging when a packet is transmitted or received
 * at the application layer
 */
void
UePacketTrace (Ptr<OutputStreamWrapper> stream, std::string context, Ptr<const Packet> p, const Address &srcAddrs, const Address &dstAddrs)
{
  std::ostringstream oss;
  stream->GetStream ()->precision (6);

  *stream->GetStream () << Simulator::Now ().GetNanoSeconds () / (double) 1e9 << "\t"
                        << context << "\t"
                        << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 () << ":"
                        << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t"
                        << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 () << ":"
                        << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << "\t"
                        << p->GetSize () << "\t"
                        << std::endl;

}

/**
 * Function that generates a gnuplot script file that can be used to plot the
 * topology of the scenario access network (eNBs, Relay UEs and Remote UEs)
 */
void
GenerateTopologyPlotFile (NodeContainer enbNode, NodeContainer relayUeNodes, NodeContainer remoteUeNodes,
                          double relayRadius, double remoteRadius )
{
  std::string fileNameWithNoExtension = "topology";
  std::string graphicsFileName = fileNameWithNoExtension + ".png";
  std::string gnuplotFileName = fileNameWithNoExtension + ".plt";
  std::string plotTitle = "Topology (Labels = Node IDs)";

  Gnuplot plot (graphicsFileName);
  plot.SetTitle (plotTitle);
  plot.SetTerminal ("png size 1280,1024");
  plot.SetLegend ("X", "Y"); //These are the axis, not the legend
  std::ostringstream plotExtras;
  plotExtras << "set xrange [-" << 1.1 * (relayRadius + remoteRadius) << ":+" << 1.1 * (relayRadius + remoteRadius) << "]" << std::endl;
  plotExtras << "set yrange [-" << 1.1 * (relayRadius + remoteRadius) << ":+" << 1.1 * (relayRadius + remoteRadius) << "]" << std::endl;
  plotExtras << "set linetype 1 pt 3 ps 2 " << std::endl;
  plotExtras << "set linetype 2 lc rgb \"green\" pt 2 ps 2" << std::endl;
  plotExtras << "set linetype 3 pt 1 ps 2" << std::endl;
  plot.AppendExtra (plotExtras.str ());

  //eNB
  Gnuplot2dDataset datasetEnodeB;
  datasetEnodeB.SetTitle ("eNodeB");
  datasetEnodeB.SetStyle (Gnuplot2dDataset::POINTS);

  double x = enbNode.Get (0)->GetObject<MobilityModel> ()->GetPosition ().x;
  double y = enbNode.Get (0)->GetObject<MobilityModel> ()->GetPosition ().y;
  std::ostringstream strForLabel;
  strForLabel << "set label \"" << enbNode.Get (0)->GetId () << "\" at " << x << "," << y << " textcolor rgb \"grey\" center front offset 0,1";
  plot.AppendExtra (strForLabel.str ());
  datasetEnodeB.Add (x, y);
  plot.AddDataset (datasetEnodeB);

  //Relay UEs
  Gnuplot2dDataset datasetRelays;
  datasetRelays.SetTitle ("Relay UEs");
  datasetRelays.SetStyle (Gnuplot2dDataset::POINTS);
  for (uint32_t ry = 0; ry < relayUeNodes.GetN (); ry++)
    {
      double x = relayUeNodes.Get (ry)->GetObject<MobilityModel> ()->GetPosition ().x;
      double y = relayUeNodes.Get (ry)->GetObject<MobilityModel> ()->GetPosition ().y;
      std::ostringstream strForLabel;
      strForLabel << "set label \"" << relayUeNodes.Get (ry)->GetId () << "\" at " << x << "," << y << " textcolor rgb \"grey\" center front offset 0,1";
      plot.AppendExtra (strForLabel.str ());
      datasetRelays.Add (x, y);
    }
  plot.AddDataset (datasetRelays);

  //Remote UEs
  Gnuplot2dDataset datasetRemotes;
  datasetRemotes.SetTitle ("Remote UEs");
  datasetRemotes.SetStyle (Gnuplot2dDataset::POINTS);
  for (uint32_t rm = 0; rm < remoteUeNodes.GetN (); rm++)
    {
      double x = remoteUeNodes.Get (rm)->GetObject<MobilityModel> ()->GetPosition ().x;
      double y = remoteUeNodes.Get (rm)->GetObject<MobilityModel> ()->GetPosition ().y;
      std::ostringstream strForLabel;
      strForLabel << "set label \"" << remoteUeNodes.Get (rm)->GetId () << "\" at " << x << "," << y << " textcolor rgb \"grey\" center front offset 0,1";
      plot.AppendExtra (strForLabel.str ());
      datasetRemotes.Add (x, y);
    }
  plot.AddDataset (datasetRemotes);

  std::ofstream plotFile (gnuplotFileName.c_str ());
  plot.GenerateOutput (plotFile);
  plotFile.close ();
}

/*
 * Trace sink function for logging when PC5 signaling messages are received
 */
void
TraceSinkPC5SignalingPacketTrace (Ptr<OutputStreamWrapper> stream, uint32_t srcL2Id, uint32_t dstL2Id, Ptr<Packet> p)
{
  LteSlPc5SignallingMessageType lpc5smt;
  p->PeekHeader (lpc5smt);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << srcL2Id << "\t" << dstL2Id << "\t" << lpc5smt.GetMessageName () << std::endl;
}

/**
 * Function to update the remote address of a UdpEchoClient application to
 * the UE-to-Network Relay network address of a node.
 * Used when the UdpEchoServer application is installed in a Remote UE.
 * The Remote UE will use an IPv6 address from the UE-to-Network Relay network
 * once it connects to a Relay UE. Thus, the UdpEchoClient applications
 * sending to the UdpEchoServer application in the Remote UE need to update
 * their remote address to this new address to be able to reach it.
 */
void
ChangeUdpEchoClientRemote (Ptr<Node> newRemoteNode, Ptr<UdpEchoClient> app, uint16_t port, Ipv6Address network, Ipv6Prefix prefix)
{
  Ptr<Ipv6> ipv6 = newRemoteNode->GetObject<Ipv6> ();
  //Get the interface used for UE-to-Network Relay (LteSlUeNetDevices)
  int32_t ipInterfaceIndex = ipv6->GetInterfaceForPrefix (network, prefix);

  Ipv6Address remoteNodeSlIpAddress = newRemoteNode->GetObject<Ipv6L3Protocol> ()->GetAddress (ipInterfaceIndex,1).GetAddress ();
  NS_LOG_INFO (" Node id = [" << app->GetNode ()->GetId ()
                              << "] changed the UdpEchoClient Remote Ip Address to " << remoteNodeSlIpAddress);
  app->SetRemote (remoteNodeSlIpAddress, port);
}


int main (int argc, char *argv[])
{
  // relay and remote ue  declaration
  double simTime = 1.0; //s //Simulation time (in seconds) updated automatically based on number of nodes
  double relayRadius = 300.0; //m
  double remoteRadius = 50.0; //m
  uint32_t nRelayUes = 2;
  uint32_t nRemoteUesPerRelay = 5;
  bool remoteUesOoc = true;
  std::string echoServerNode ("RemoteUE");

  CommandLine cmd;

  cmd.AddValue ("relayRadius", "The radius of the circle (with center on the eNB) where the Relay UEs are positioned", relayRadius);
  cmd.AddValue ("remoteRadius", "The radius of the circle (with center on the Relay UE) where the Remote UEs are positioned", remoteRadius);
  cmd.AddValue ("nRelayUes", "Number of Relay UEs in the scenario", nRelayUes);
  cmd.AddValue ("nRemoteUesPerRelay", "Number of Remote UEs per deployed Relay UE", nRemoteUesPerRelay);
  cmd.AddValue ("remoteUesOoc", "The Remote UEs are out-of-coverage", remoteUesOoc);
  cmd.AddValue ("echoServerNode", "The node towards which the Remote UE traffic is directed to (RemoteHost|RemoteUE)", echoServerNode);
  cmd.Parse (argc, argv);

 
  if (echoServerNode.compare ("RemoteHost") != 0 && echoServerNode.compare ("RemoteUE") != 0)
    {
      NS_FATAL_ERROR ("Wrong echoServerNode!. Options are (RemoteHost|RemoteUE).");
      return 1;
    }
  if (echoServerNode.compare ("RemoteUE") == 0 && nRelayUes * nRemoteUesPerRelay < 2)
    {
      NS_FATAL_ERROR ("At least 2 Remote UEs are needed when echoServerNode is a RemoteUE !");
      return 1;
    }

  //Calculate the start time of the relay service for Relay UEs and Remote UEs
  
  //Do it sequentially for easy of tractability
  double startTimeRelay [nRelayUes];
  double startTimeRemote [nRelayUes * nRemoteUesPerRelay];
  //The time between Remote UE's start of service
  //Four discovery periods (4 * 0.32 s) to ensure relay selection (measurement report every 4 periods)
  //One discovery period (0.32 s) to avoid some of the congestion for the connection messages
  double timeBetweenRemoteStarts = 4 * 0.32 + 0.32; //s
  // The time between Relay UE's start of service
  // 1.0 s of baseline
  // 0.320 s to ensure sending the 1st discovery message
  // plus the time needed to all Remote UEs to connect to it:
  // (2+2*nRemoteUesPerRelay)*0.04 is the time in the worst case for all connection messages to go through between a Remote UE and a Relay UE:
  // 2 msgs from the Remote UE, each of them in 1 single SL period (0.04 s) and
  // 2 msgs from the Relay UE, which given the SL period RR scheduling, it can take in the worst case up to
  // nRemoteUesPerRelay SL periods to be sent
  // timeBetweenRemoteStarts to ensure sequentiality of activation
  double timeBetweenRelayStarts = 1.0 + nRemoteUesPerRelay * ((2 + 2 * nRemoteUesPerRelay) * 0.04 + timeBetweenRemoteStarts); //s

  for (uint32_t ryIdx = 0; ryIdx < nRelayUes; ryIdx++)
    {
      startTimeRelay [ryIdx] = 2.0 + 0.320 + timeBetweenRelayStarts * ryIdx;

      NS_LOG_INFO ("Relay UE Idx " << ryIdx << " start time " << startTimeRelay [ryIdx] << "s");

      for (uint32_t rm = 0; rm < nRemoteUesPerRelay; ++rm)
        {
          uint32_t rmIdx = ryIdx * nRemoteUesPerRelay + rm;
          startTimeRemote [rmIdx] = startTimeRelay [ryIdx] + timeBetweenRemoteStarts * (rm + 1);
          NS_LOG_INFO ("Remote UE Idx " << rmIdx << " start time " << startTimeRemote [rmIdx] << "s");
        }
    }

  //Calculate simTime based on relay service starts and give 10 s of traffic for the last one
  simTime = startTimeRemote [(nRelayUes * nRemoteUesPerRelay - 1)] + 3.0 + 10.0; //s
  NS_LOG_INFO ("Simulation time = " << simTime << " s");

  NS_LOG_INFO ("Configuring default parameters...");

  //defaults required to set for out of coverage scenario 

  //Configure the UE for UE_SELECTED scenario
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (6)); //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (false));
  Config::SetDefault ("ns3::LteUeMac::SlScheduler", StringValue ("Random")); //Values include Fixed, Random, MinPrb, MaxCoverage

  //connection with the gNB
  //Set the frequency
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (5330));
  Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn", UintegerValue (5330));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (23330));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (50));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (50));
  
   // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlFullDuplexEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteUePhy::DownlinkCqiPeriodicity", TimeValue (MilliSeconds (79)));

  //Reduce frequency of CQI report to allow for transmissions
  Config::SetDefault ("ns3::LteUePhy::DownlinkCqiPeriodicity", TimeValue (MilliSeconds (79)));

  //Increase SRS periodicity to allow larger number of UEs in the system
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));


  //Set the eNBs power in dBm
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46.0));
  
  // Instantiating LTE, EPC and Sidelink helpers and connecting them
  //Create the lte helper
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  //Create and set the EPC helper
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  
  //core network
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  //setup sidelink 

  //Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));

  //Create helper and set lteHelper
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  //Connect controller and Helper
  Config::SetDefault ("ns3::LteSlBasicUeController::ProseHelper", PointerValue (proseHelper));

  //Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Hybrid3gppPropagationLossModel"));

  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));

/*****************************************/
  //setup physical layer
  //create nodes and specify location and mobility
  
  //Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  
  std::cout << "remote host id :" <<remoteHostContainer.Get(0)->GetId() << std::endl;

  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  //Create nodes (eNb + UEs)
  NodeContainer enbNode;
  enbNode.Create (1);
  NS_LOG_INFO ("eNb node id = [" << enbNode.Get (0)->GetId () << "]");


  NodeContainer relayUeNodes;
  relayUeNodes.Create (nRelayUes);

  std::cout << "relay 1 id :" << relayUeNodes.Get(0)->GetId() << std::endl;
  std::cout << "relay 2 id :" << relayUeNodes.Get(1)->GetId() << std::endl;


  NodeContainer remoteUeNodes;
  remoteUeNodes.Create (nRelayUes * nRemoteUesPerRelay);
  std::cout << "remote 1 id :" << remoteUeNodes.Get(0)->GetId() << std::endl;
  std::cout << "remote 2 id :" << remoteUeNodes.Get(1)->GetId() << std::endl;

  for (uint32_t ry = 0; ry < relayUeNodes.GetN (); ry++)
    {
      NS_LOG_INFO ("Relay UE " << ry + 1 << " node id = [" << relayUeNodes.Get (ry)->GetId () << "]");
    }
  for (uint32_t rm = 0; rm < remoteUeNodes.GetN (); rm++)
    {
      NS_LOG_INFO ("Remote UE " << rm + 1 << " node id = [" << remoteUeNodes.Get (rm)->GetId () << "]");
    }
  NodeContainer allUeNodes = NodeContainer (relayUeNodes,remoteUeNodes);

   std::cout << "node 0 id :" << allUeNodes.Get(0)->GetId() << std::endl;
  std::cout << "node 1 id :" <<  allUeNodes.Get(1)->GetId() << std::endl;

   std::cout << "node 2 id :" << allUeNodes.Get(2)->GetId() << std::endl;
  std::cout << "node 3 id :" <<  allUeNodes.Get(3)->GetId() << std::endl;



  //Position of the nodes
  //eNodeB
  Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator> ();
  positionAllocEnb->Add (Vector (0.0, 0.0, 30.0));

  //UEs
  Ptr<ListPositionAllocator> positionAllocRelays = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAllocRemotes = CreateObject<ListPositionAllocator> ();
  for (uint32_t ry = 0; ry < relayUeNodes.GetN (); ++ry)
    {
      //Relay UE
      double ry_angle = ry * (360.0 / relayUeNodes.GetN ()); //degrees
      double ry_pos_x = std::floor (relayRadius * std::cos (ry_angle * M_PI / 180.0) );
      double ry_pos_y = std::floor (relayRadius * std::sin (ry_angle * M_PI / 180.0) );

      positionAllocRelays->Add (Vector (ry_pos_x, ry_pos_y, 1.5));

      NS_LOG_INFO ("Relay UE " << ry + 1 << " node id = [" << relayUeNodes.Get (ry)->GetId () << "]"
                   " x " << ry_pos_x << " y " << ry_pos_y);
      //Remote UEs
      for (uint32_t rm = 0; rm < nRemoteUesPerRelay; ++rm)
        {
          double rm_angle = rm * (360.0 / nRemoteUesPerRelay); //degrees
          double rm_pos_x = std::floor (ry_pos_x + remoteRadius * std::cos (rm_angle * M_PI / 180.0));
          double rm_pos_y = std::floor (ry_pos_y + remoteRadius * std::sin (rm_angle * M_PI / 180.0));

          positionAllocRemotes->Add (Vector (rm_pos_x, rm_pos_y, 1.5));

          uint32_t remoteIdx = ry * nRemoteUesPerRelay + rm;
          NS_LOG_INFO ("Remote UE " << remoteIdx << " node id = [" << remoteUeNodes.Get (remoteIdx)->GetId () << "]"
                       " x " << rm_pos_x << " y " << rm_pos_y);
        }
    }

  //Install mobility
  //eNodeB
  MobilityHelper mobilityeNodeB;
  mobilityeNodeB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityeNodeB.SetPositionAllocator (positionAllocEnb);
  mobilityeNodeB.Install (enbNode);

  //Relay UEs
  MobilityHelper mobilityRelays;
  mobilityRelays.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityRelays.SetPositionAllocator (positionAllocRelays);
  mobilityRelays.Install (relayUeNodes);

  //Remote UE
  MobilityHelper mobilityRemotes;
  mobilityRemotes.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityRemotes.SetPositionAllocator (positionAllocRemotes);
  mobilityRemotes.Install (remoteUeNodes);

  //Generate gnuplot file with the script to generate the topology plot
  GenerateTopologyPlotFile (enbNode, relayUeNodes, remoteUeNodes, relayRadius, remoteRadius);

  //Install LTE devices to the nodes
  NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNode);
  NetDeviceContainer relayUeDevs = lteHelper->InstallUeDevice (relayUeNodes);
  NetDeviceContainer remoteUeDevs = lteHelper->InstallUeDevice (remoteUeNodes);
  NetDeviceContainer allUeDevs = NetDeviceContainer (relayUeDevs, remoteUeDevs);

  //connecting sidelink and gnB
  //Configure eNodeB for Sidelink
  Ptr<LteSlEnbRrc> enbSidelinkConfiguration = CreateObject<LteSlEnbRrc> ();
  enbSidelinkConfiguration->SetSlEnabled (true);

  //-Configure communication pool
  //resource setup : UE selected
  enbSidelinkConfiguration->SetDefaultPool (proseHelper->GetDefaultSlCommTxResourcesSetupUeSelected ());
  
  //-Enable discovery
  enbSidelinkConfiguration->SetDiscEnabled (true);

  //-Configure discovery pool
  enbSidelinkConfiguration->AddDiscPool (proseHelper->GetDefaultSlDiscTxResourcesSetupUeSelected ());
  LteRrcSap::SlDiscTxResourcesSetup pool;
  LteSlDiscResourcePoolFactory pDiscFactory;
  pDiscFactory.SetDiscCpLen ("NORMAL");
  pDiscFactory.SetDiscPeriod ("rf32");
  pDiscFactory.SetNumRetx (0);
  pDiscFactory.SetNumRepetition (1);
  pDiscFactory.SetDiscPrbNum (10);
  pDiscFactory.SetDiscPrbStart (10);
  pDiscFactory.SetDiscPrbEnd (40);
  pDiscFactory.SetDiscOffset (0);
  pDiscFactory.SetDiscBitmap (0x11111);
  pDiscFactory.SetDiscTxProbability ("p100");

  pool.ueSelected.poolToAddModList.pools[0].pool =  pDiscFactory.CreatePool ();


   LteRrcSap::SlCommTxResourcesSetup commpool;
  LteSlResourcePoolFactory pfactory;
  //Control
  pfactory.SetControlPeriod ("sf40");
  pfactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
  pfactory.SetControlOffset (0);
  pfactory.SetControlPrbNum (22);
  pfactory.SetControlPrbStart (0);
  pfactory.SetControlPrbEnd (49);
  
  //Data
  pfactory.SetDataBitmap (0xFFFFFFFFFF);
  pfactory.SetDataOffset (8); //After 8 subframes of PSCCH
  pfactory.SetDataPrbNum (25);
  pfactory.SetDataPrbStart (0);
  pfactory.SetDataPrbEnd (49);

  commpool.ueSelected.poolToAddModList.pools[0].pool =  pfactory.CreatePool ();

  //-Configure UE-to-Network Relay parameters
  //The parameters for UE-to-Network Relay (re)selection are broadcasted in the SIB19 to the UEs attached to the eNB.
  //they are required by the
  // LteUeRrc for SD-RSRP measurement and Relay Selection
  enbSidelinkConfiguration->SetDiscConfigRelay (proseHelper->GetDefaultSib19DiscConfigRelay ());

  //Install eNodeB configuration on the eNodeB devices
  lteHelper->InstallSidelinkConfiguration (enbDevs, enbSidelinkConfiguration);

  //Preconfigure UEs for Sidelink
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);

  //-Configure preconfiguration
  // Relay UEs: Empty configuration
  LteRrcSap::SlPreconfiguration preconfigurationRelay;
  // Remote UEs: Empty configuration if in-coverage
  //             Custom configuration (see below) if out-of-coverage
  LteRrcSap::SlPreconfiguration preconfigurationRemote;

  if (remoteUesOoc)
    {
      //Configure general preconfiguration parameters
      preconfigurationRemote.preconfigGeneral.carrierFreq = enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlEarfcn ();
      preconfigurationRemote.preconfigGeneral.slBandwidth = enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlBandwidth ();
      std::cout << 1 << std::endl;

      //-Configure preconfigured communication pool
      preconfigurationRemote.preconfigComm = proseHelper->GetDefaultSlPreconfigCommPoolList ();
      
//        LteRrcSap::SlPreconfigCommPoolList preconfigComm;
//   // preconfigComm.nbPools = 1;
//         std::cout << 1 << std::endl;
//   // LteSlPreconfigPoolFactory preconfCommPoolFactory;
  
      //Discovery
      preconfigurationRemote.preconfigDisc.nbPools = 1;
      LteSlDiscPreconfigPoolFactory preconfDiscPoolFactory;
      preconfDiscPoolFactory.SetDiscCpLen ("NORMAL");
      preconfDiscPoolFactory.SetDiscPeriod ("rf32");
      preconfDiscPoolFactory.SetNumRetx (0);
      preconfDiscPoolFactory.SetNumRepetition (1);
      preconfDiscPoolFactory.SetDiscPrbNum (10);
      preconfDiscPoolFactory.SetDiscPrbStart (10);
      preconfDiscPoolFactory.SetDiscPrbEnd (40);
      preconfDiscPoolFactory.SetDiscOffset (0);
      preconfDiscPoolFactory.SetDiscBitmap (0x11111);
      preconfDiscPoolFactory.SetDiscTxProbability ("p100");

      preconfigurationRemote.preconfigDisc.pools[0] = preconfDiscPoolFactory.CreatePool ();

       //Communication
      preconfigurationRemote.preconfigComm.nbPools = 1;
      LteSlPreconfigPoolFactory preconfCommPoolFactory;
      //-Control
      preconfCommPoolFactory.SetControlPeriod ("sf40");
      preconfCommPoolFactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
      preconfCommPoolFactory.SetControlOffset (0);
      preconfCommPoolFactory.SetControlPrbNum (22);
      preconfCommPoolFactory.SetControlPrbStart (0);
      preconfCommPoolFactory.SetControlPrbEnd (49);
      //-Data
      preconfCommPoolFactory.SetDataBitmap (0xFFFFFFFFFF);
      preconfCommPoolFactory.SetDataOffset (8); //After 8 subframes of PSCCH
      preconfCommPoolFactory.SetDataPrbNum (25);
      preconfCommPoolFactory.SetDataPrbStart (0);
      preconfCommPoolFactory.SetDataPrbEnd (49);

      preconfigurationRemote.preconfigComm.pools[0] = preconfCommPoolFactory.CreatePool ();

      //-Relay UE (re)selection
      preconfigurationRemote.preconfigRelay.haveReselectionInfoOoc = true;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.filterCoefficient = 0;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.minHyst = 0;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.qRxLevMin = -125;


// preconfigurationRemote.preconfigComm.nbPools = 1;
//    std::cout << 1 << std::endl;
//   LteSlPreconfigPoolFactory pfactory;
//    std::cout << 2 << std::endl;

//   //Control
//   pfactory.SetControlPeriod ("sf40");
//   pfactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
//   pfactory.SetControlOffset (0);
//   pfactory.SetControlPrbNum (22);
//   pfactory.SetControlPrbStart (0);
//   pfactory.SetControlPrbEnd (49);

//   //Data
//   pfactory.SetDataBitmap (0xFFFFFFFFFF);
//   pfactory.SetDataOffset (8); //After 8 subframes of PSCCH
//   pfactory.SetDataPrbNum (25);
//   pfactory.SetDataPrbStart (0);
//   pfactory.SetDataPrbEnd (49);
//    std::cout << 3 << std::endl;
//    preconfigComm.pools[0] = pfactory.CreatePool ();
//     std::cout << 4 << std::endl;      


      //-Configure preconfigured discovery pool
    //   preconfigurationRemote.preconfigDisc = proseHelper->GetDefaultSlPreconfigDiscPoolList ();
       std::cout << 5 << std::endl;
      //-Configure preconfigured UE-to-Network Relay parameters
    //   preconfigurationRemote.preconfigRelay = proseHelper->GetDefaultSlPreconfigRelay ();
       std::cout << 6 << std::endl;

    }


  uint8_t nb = 3;
  ueSidelinkConfiguration->SetDiscTxResources (nb);

  //-Enable discovery
  ueSidelinkConfiguration->SetDiscEnabled (true);
  //-Set frequency for discovery messages monitoring
  ueSidelinkConfiguration->SetDiscInterFreq (enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlEarfcn ());

  //Install UE configuration on the UE devices with the corresponding preconfiguration
  ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRelay);
  lteHelper->InstallSidelinkConfiguration (relayUeDevs, ueSidelinkConfiguration);

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRemote);
  lteHelper->InstallSidelinkConfiguration (remoteUeDevs, ueSidelinkConfiguration);
  
  //NAS layer 
  WifiHelper wifi;
wifi.SetStandard (WIFI_PHY_STANDARD_80211g); //2.4Ghz
wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("ErpOfdmRate54Mbps"));
 
WifiMacHelper wifiMac;
YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
YansWifiChannelHelper wifiChannel;
wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel",
                                "Frequency", DoubleValue (2.407e9)); //2.4Ghz
wifiMac.SetType ("ns3::AdhocWifiMac");
YansWifiPhyHelper phy = wifiPhy;
phy.SetChannel (wifiChannel.Create ());
WifiMacHelper mac = wifiMac;
allUeDevs = wifi.Install (phy, mac, allUeNodes);



  //Install the IP stack on the UEs and assign network IP addresses
  internet.Install (relayUeNodes);
  internet.Install (remoteUeNodes);
  Ipv6InterfaceContainer ueIpIfaceRelays;
  Ipv6InterfaceContainer ueIpIfaceRemotes;
  ueIpIfaceRelays = epcHelper->AssignUeIpv6Address (relayUeDevs);
  ueIpIfaceRemotes = epcHelper->AssignUeIpv6Address (remoteUeDevs);

  //Set the default gateway for the UEs
  Ipv6StaticRoutingHelper Ipv6RoutingHelper;
  for (uint32_t u = 0; u < allUeNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = allUeNodes.Get (u);
      Ptr<Ipv6StaticRouting> ueStaticRouting = Ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
    }

  //Configure IP for the nodes in the Internet (PGW and RemoteHost)
  Ipv6AddressHelper ipv6h;
  ipv6h.SetBase (Ipv6Address ("6001:db80::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer internetIpIfaces = ipv6h.Assign (internetDevices);
  internetIpIfaces.SetForwarding (0, true);
  internetIpIfaces.SetDefaultRouteInAllNodes (0);

  //Set route for the Remote Host to join the LTE network nodes
  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> remoteHostStaticRouting = ipv6RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv6> ());
  remoteHostStaticRouting->AddNetworkRouteTo ("7777:f000::", Ipv6Prefix (60), internetIpIfaces.GetAddress (0, 1), 1, 0);

  //Configure UE-to-Network Relay network
  proseHelper->SetIpv6BaseForRelayCommunication ("7777:f00e::", Ipv6Prefix (48));

  //Configure route between PGW and UE-to-Network Relay network
  proseHelper->ConfigurePgwToUeToNetworkRelayRoute (pgw);

  //Attach Relay UEs to the eNB
  lteHelper->Attach (relayUeDevs);
  //If the Remote UEs are not OOC attach them to the eNodeB as well
  if (!remoteUesOoc)
    {
      lteHelper->Attach (remoteUeDevs);
    }
  // interface 0 is localhost, 1 is the p2p device
  Ipv6Address pgwAddr = internetIpIfaces.GetAddress (0,1);
  Ipv6Address remoteHostAddr = internetIpIfaces.GetAddress (1,1);
  // Ipv6Address remoteUeaddress =  internet_uedevices.GetAddress(1,1);
  // Ipv6Address relayUeaddress =  internet_uedevices.GetAddress(0,1);
  std::cout << " pgw address : " << pgwAddr << std::endl;
  std::cout << "remote host address : " << remoteHostAddr << std::endl;
  // std::cout << "remote ue address : " << remoteUeaddress << std::endl;
  // std::cout << "relay ue address : " << relayUeaddress << std::endl;


  ///*** Configure applications ***///
  //For each Remote UE, we have a pair (UpdEchoClient, UdpEchoServer)
  //Each Remote UE has an assigned port
  //UdpEchoClient installed in the Remote UE, sending to the echoServerAddr
  //and to the corresponding Remote UE port
  //UdpEchoServer installed in the echoServerNode, listening to the
  //corresponding Remote UE port

  Ipv6Address echoServerAddr;
  if (echoServerNode.compare ("RemoteHost") == 0)
    {
      echoServerAddr = internetIpIfaces.GetAddress (1, 1);
    }
  else if (echoServerNode.compare ("RemoteUE") == 0)
    {
      // We use a dummy IP address for initial configuration as we don't know the
      // IP address of the 'Remote UE (0)' before it connects to the Relay UE
      echoServerAddr = Ipv6Address::GetOnes ();
    }
  uint16_t echoPortBase = 50000;
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  AsciiTraceHelper ascii;

  std::ostringstream oss;
  Ptr<OutputStreamWrapper> packetOutputStream = ascii.CreateFileStream ("AppPacketTrace.txt");
  *packetOutputStream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;

  for (uint16_t remUeIdx = 0; remUeIdx < remoteUeNodes.GetN (); remUeIdx++)
    {
      if (echoServerNode.compare ("RemoteUE") == 0 && remUeIdx == 0)
        {
          //No own traffic applications for Remote UE (0) as it is the echoServerNode
          continue;
        }

      uint16_t remUePort = echoPortBase + remUeIdx;
      uint32_t echoServerNodeId = 0;
      //UdpEchoServer listening in the Remote UE port
      UdpEchoServerHelper echoServerHelper (remUePort);
      ApplicationContainer singleServerApp;
      if (echoServerNode.compare ("RelayUE") == 0)
        {
          singleServerApp.Add (echoServerHelper.Install (relayUeNodes));
          echoServerNodeId = relayUeNodes.Get(0)->GetId ();
        }
      else if (echoServerNode.compare ("RemoteUE") == 0)
        {
          singleServerApp.Add (echoServerHelper.Install (remoteUeNodes.Get (0)));
          echoServerNodeId = remoteUeNodes.Get (0)->GetId ();

        }
      singleServerApp.Start (Seconds (1.0));
      singleServerApp.Stop (Seconds (simTime));

      //Tracing packets on the UdpEchoServer (S)
      oss << "rx\tS\t" << echoServerNodeId;
      singleServerApp.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");
      oss << "tx\tS\t" << echoServerNodeId;
      singleServerApp.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");

      serverApps.Add (singleServerApp);

      //UdpEchoClient in the Remote UE
      UdpEchoClientHelper echoClientHelper (echoServerAddr);
      echoClientHelper.SetAttribute ("MaxPackets", UintegerValue (20));
      echoClientHelper.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
      echoClientHelper.SetAttribute ("PacketSize", UintegerValue (150));
      echoClientHelper.SetAttribute ("RemotePort", UintegerValue (remUePort));

      ApplicationContainer singleClientApp = echoClientHelper.Install (remoteUeNodes.Get (remUeIdx));
      //Start the application 3.0 s after the remote UE started the relay service
      //normally it should be enough time to connect
      singleClientApp.Start (Seconds (3.0 + startTimeRemote [remUeIdx]) );
      //Stop the application after 10.0 s
      singleClientApp.Stop (Seconds (3.0 + startTimeRemote [remUeIdx] + 10.0));
 

    /* mcptt app**************************************/ 
    //creating mcptt application for each device 

McpttHelper mcpttHelper;
ApplicationContainer mcpttclientApps;
McpttTimer mcpttTimer;

Time startTime = Seconds (1);
Time stopTime = Seconds (20); 
double pushTimeMean = 5.0; // seconds
double pushTimeVariance = 2.0; // seconds
double releaseTimeMean = 5.0; // seconds
double releaseTimeVariance = 2.0; // seconds
TypeId socketFacTid = UdpSocketFactory::GetTypeId ();
DataRate dataRate = DataRate ("24kb/s");
Ipv6Address PeerAddress = Ipv6Address ("6001:db80::");
Ipv4Address groupAddress4 = Ipv4Address ("255.255.255.255");
uint32_t msgSize = 60; //60 + RTP header = 60 + 12 = 72
//creating mcptt service on each node

mcpttHelper.SetPttApp ("ns3::McpttPttApp",
                        "PeerAddress", Ipv4AddressValue (groupAddress4), 
                        "PushOnStart", BooleanValue (true));
mcpttHelper.SetMediaSrc ("ns3::McpttMediaSrc",
                        "Bytes", UintegerValue (msgSize),
                        "DataRate", DataRateValue (dataRate));
mcpttHelper.SetPusher ("ns3::McpttPusher",
                        "Automatic", BooleanValue (false));
mcpttHelper.SetPusherPushVariable ("ns3::NormalRandomVariable",
                        "Mean", DoubleValue (pushTimeMean),
                        "Variance", DoubleValue (pushTimeVariance));
mcpttHelper.SetPusherReleaseVariable ("ns3::NormalRandomVariable",
                        "Mean", DoubleValue (releaseTimeMean),
                        "Variance", DoubleValue (releaseTimeVariance));

mcpttHelper.EnableStateMachineTraces();       

mcpttclientApps.Add (mcpttHelper.Install (allUeNodes));
mcpttclientApps.Start (startTime);
mcpttclientApps.Stop (stopTime);

//creating a call initiated by the press of push button 
ObjectFactory callFac;
callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");

ObjectFactory floorFac;
floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");
  


//creating location to store application info of UEs A, B 
Ptr<McpttPttApp> ueAPttApp = DynamicCast<McpttPttApp, Application> (mcpttclientApps.Get (0));

//floor and call machines generate*******************************
ueAPttApp->CreateCall (callFac, floorFac);
ueAPttApp->SelectLastCall ();

Ipv4AddressValue grpAddr;

Simulator::Schedule (Seconds (2.2), &McpttPttApp::TakePushNotification, ueAPttApp);
uint16_t floorPort = McpttPttApp::AllocateNextPortNumber ();
uint16_t speechPort = McpttPttApp::AllocateNextPortNumber ();
Ptr<McpttCall> ueACall = ueAPttApp->GetSelectedCall ();
Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueACall, grpAddr.Get (), floorPort);
Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueACall, grpAddr.Get (), speechPort);

/** tracing ***************/
    uint32_t mcpttClientAppNodeId = 0;
     
     mcpttClientAppNodeId = remoteUeNodes.Get(0)->GetId ();
            //Tracing packets on the UdpEchoServer (S)
      oss << "rx\tS\t" << mcpttClientAppNodeId;
      mcpttclientApps.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");
      oss << "tx\tS\t" << mcpttClientAppNodeId;
      mcpttclientApps.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");



  NS_LOG_INFO ("Enabling MCPTT traces...");
  mcpttHelper.EnableMsgTraces ();
  mcpttHelper.EnableStateMachineTraces ();


      //Tracing packets on the UdpEchoClient (C)
      oss << "tx\tC\t" << remoteUeNodes.Get (remUeIdx)->GetId ();

      std::cout << "remote ue id: " << remoteUeNodes.Get (remUeIdx)->GetId () << std::endl;

      singleClientApp.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");
      oss << "rx\tC\t" << remoteUeNodes.Get (remUeIdx)->GetId ();
      singleClientApp.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");

      clientApps.Add (singleClientApp);

      if (echoServerNode.compare ("RemoteUE") == 0 && remUeIdx != 0)
        {
          //Schedule the change of the RemoteAddress to 100 ms before the start of the application
          //normally 'Remote UE (0)' should be already connected to its Relay UE so we can
          //assign its address as RemoteAddress
          Simulator::Schedule (Seconds (3.0 + startTimeRemote [remUeIdx] - 0.100),
                               &ChangeUdpEchoClientRemote, remoteUeNodes.Get (0),
                               singleClientApp.Get (0)->GetObject<UdpEchoClient> (),
                               remUePort,
                               proseHelper->GetIpv6NetworkForRelayCommunication (),
                               proseHelper->GetIpv6PrefixForRelayCommunication ());
        }
    }

  ///*** Configure Relaying ***///

  //Setup dedicated bearer for the Relay UEs
  Ptr<EpcTft> tft = Create<EpcTft> ();
  EpcTft::PacketFilter dlpf;
  dlpf.localIpv6Address = proseHelper->GetIpv6NetworkForRelayCommunication ();
  dlpf.localIpv6Prefix = proseHelper->GetIpv6PrefixForRelayCommunication ();
  tft->Add (dlpf);
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
  lteHelper->ActivateDedicatedEpsBearer (relayUeDevs, bearer, tft);

  Ptr<EpcTft> remote_tft = Create<EpcTft> ();
      EpcTft::PacketFilter rem_dlpf;
      rem_dlpf.localIpv6Address.Set ("7777:f00e::");
      rem_dlpf.localIpv6Prefix = Ipv6Prefix (64);
      remote_tft->Add (rem_dlpf);
      EpsBearer remote_bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
      lteHelper->ActivateDedicatedEpsBearer (remoteUeDevs.Get (0), remote_bearer, remote_tft);


  //Schedule the start of the relay service in each UE with their corresponding
  //roles and service codes
  for (uint32_t ryDevIdx = 0; ryDevIdx < relayUeDevs.GetN (); ryDevIdx++)
    {
      uint32_t serviceCode = relayUeDevs.Get (ryDevIdx)->GetObject<LteUeNetDevice> ()->GetImsi ();

      Simulator::Schedule (Seconds (startTimeRelay [ryDevIdx]), &LteSidelinkHelper::StartRelayService, proseHelper, relayUeDevs.Get (ryDevIdx), serviceCode, LteSlUeRrc::ModelA, LteSlUeRrc::RelayUE);
      NS_LOG_INFO ("Relay UE " << ryDevIdx << " node id = [" << relayUeNodes.Get (ryDevIdx)->GetId () << "] provides Service Code " << serviceCode << " and start service at " << startTimeRelay [ryDevIdx] << " s");
      //Remote UEs
      for (uint32_t rm = 0; rm < nRemoteUesPerRelay; ++rm)
        {
          uint32_t rmDevIdx = ryDevIdx * nRemoteUesPerRelay + rm;
          Simulator::Schedule ((Seconds (startTimeRemote [rmDevIdx])), &LteSidelinkHelper::StartRelayService, proseHelper, remoteUeDevs.Get (rmDevIdx), serviceCode, LteSlUeRrc::ModelA, LteSlUeRrc::RemoteUE);
          NS_LOG_INFO ("Remote UE " << rmDevIdx << " node id = [" << remoteUeNodes.Get (rmDevIdx)->GetId () << "] interested in Service Code " << serviceCode << " and start service at " << startTimeRemote [rmDevIdx] << " s");
        }
    }

  //Tracing PC5 signaling messages
  Ptr<OutputStreamWrapper> PC5SignalingPacketTraceStream = ascii.CreateFileStream ("PC5SignalingPacketTrace.txt");
  *PC5SignalingPacketTraceStream->GetStream () << "time(s)\ttxId\tRxId\tmsgType" << std::endl;

  for (uint32_t ueDevIdx = 0; ueDevIdx < relayUeDevs.GetN (); ueDevIdx++)
    {
      Ptr<LteUeRrc> rrc = relayUeDevs.Get (ueDevIdx)->GetObject<LteUeNetDevice> ()->GetRrc ();
      PointerValue ptrOne;
      rrc->GetAttribute ("SidelinkConfiguration", ptrOne);
      Ptr<LteSlUeRrc> slUeRrc = ptrOne.Get<LteSlUeRrc> ();
      slUeRrc->TraceConnectWithoutContext ("PC5SignalingPacketTrace",
                                           MakeBoundCallback (&TraceSinkPC5SignalingPacketTrace,
                                                              PC5SignalingPacketTraceStream));
    }
  for (uint32_t ueDevIdx = 0; ueDevIdx < remoteUeDevs.GetN (); ueDevIdx++)
    {
      Ptr<LteUeRrc> rrc = remoteUeDevs.Get (ueDevIdx)->GetObject<LteUeNetDevice> ()->GetRrc ();
      PointerValue ptrOne;
      rrc->GetAttribute ("SidelinkConfiguration", ptrOne);
      Ptr<LteSlUeRrc> slUeRrc = ptrOne.Get<LteSlUeRrc> ();
      slUeRrc->TraceConnectWithoutContext ("PC5SignalingPacketTrace",
                                           MakeBoundCallback (&TraceSinkPC5SignalingPacketTrace,
                                                              PC5SignalingPacketTraceStream));
    }
  


   std::ostringstream txWithAddresses;
  txWithAddresses << "/NodeList/" << remoteUeDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/TxWithAddresses";
  //Config::ConnectWithoutContext (txWithAddresses.str (), MakeCallback (&SlOoc1Relay1RemoteRegularTestCase::DataPacketSinkTxNode, this));

  std::ostringstream rxWithAddresses;
  rxWithAddresses << "/NodeList/" << remoteUeDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/RxWithAddresses";


  lteHelper->EnablePdcpTraces ();
  lteHelper->EnableSlPscchMacTraces ();
  lteHelper->EnableSlPsschMacTraces ();

  lteHelper->EnableSlRxPhyTraces ();
  lteHelper->EnableSlPscchRxPhyTraces ();

  NS_LOG_INFO ("Simulation time " << simTime << " s");
  NS_LOG_INFO ("Starting simulation...");

  AnimationInterface anim("b_lte_sl_relay_cluster.xml");
  std::cout << 7 << std::endl;
  anim.SetMaxPktsPerTraceFile(500000); 
  anim.EnablePacketMetadata (true);

  Simulator::Stop (Seconds (simTime));
  std::cout << 8 << std::endl;
  Simulator::Run ();
  std::cout << 9 << std::endl;
  Simulator::Destroy ();
  return 0;

}