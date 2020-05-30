/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include "ns3/log.h"
#include <ns3/buildings-module.h>
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/eps-bearer-tag.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-apps-module.h"
#include "ns3/epc-helper.h"
#include "ns3/netanim-module.h"
// #include "ns3/mmwave-helper.h"


//#include "ns3/gtk-config-store.h"

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
struct info {
  uint16_t rnti; uint16_t cellId; double rsrp; double rsrq; bool servingCell; uint8_t carrier;
};


uint16_t counterN310FirsteNB = 0;
Time t310StartTimeFirstEnb = Seconds (0);
std::vector<info> info_vector;

void
PrintUePosition (uint64_t imsi)
{

  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (uedev)
            {
              if (imsi == uedev->GetImsi ())
                {
                  Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
                  std::cout << "IMSI : " << uedev->GetImsi () << " at " << pos.x << "," << pos.y << std::endl;
                }
            }
        }
    }
}

/// Map each of UE RRC states to its string representation.
static const std::string g_ueRrcStateName[LteUeRrc::NUM_STATES] =
{
  "IDLE_START",
  "IDLE_CELL_SEARCH",
  "IDLE_WAIT_MIB_SIB1",
  "IDLE_WAIT_MIB",
  "IDLE_WAIT_SIB1",
  "IDLE_CAMPED_NORMALLY",
  "IDLE_WAIT_SIB2",
  "IDLE_RANDOM_ACCESS",
  "IDLE_CONNECTING",
  "CONNECTED_NORMALLY",
  "CONNECTED_HANDOVER",
  "CONNECTED_PHY_PROBLEM",
  "CONNECTED_REESTABLISHING"
};

/**
 * \param s The UE RRC state.
 * \return The string representation of the given state.
 */
static const std::string & ToString (LteUeRrc::State s)
{
  return g_ueRrcStateName[s];
}

void
UeStateTransition (uint64_t imsi, uint16_t cellId, uint16_t rnti, LteUeRrc::State oldState, LteUeRrc::State newState)
{
  std::cout << Simulator::Now ().GetSeconds ()
            << " UE with IMSI " << imsi << " RNTI " << rnti << " connected to cell " << cellId <<
  " transitions from " << ToString (oldState) << " to " << ToString (newState) << std::endl;
}

void
EnbRrcTimeout (uint64_t imsi, uint16_t rnti, uint16_t cellId, std::string cause)
{
  std::cout << Simulator::Now ().GetSeconds ()
            << " IMSI " << imsi << ", RNTI " << rnti << ", Cell id " << cellId
            << ", ENB RRC " << cause << std::endl;
}

void
NotifyConnectionReleaseAtEnodeB (uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  std::cout << Simulator::Now ()
            << " IMSI " << imsi << ", RNTI " << rnti << ", Cell id " << cellId
            << ", UE context destroyed at eNodeB" << std::endl;
}

void PhySyncDetection (uint16_t n310, uint64_t imsi, uint16_t rnti, uint16_t cellId, std::string type, uint8_t count)
{
  std::cout << Simulator::Now ().GetSeconds ()
            << " IMSI " << imsi << ", RNTI " << rnti
            << ", Cell id " << cellId << ", " << type << ", no of sync indications: " << +count
            << std::endl;

  if (type == "Notify out of sync" && cellId == 1)
    {
      ++counterN310FirsteNB;
      if (counterN310FirsteNB == n310)
        {
          t310StartTimeFirstEnb = Simulator::Now ();
        }
      // NS_LOG_DEBUG ("counterN310FirsteNB = " << counterN310FirsteNB);
    }
}

void RadioLinkFailure (Time t310, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  std::cout << Simulator::Now ()
            << " IMSI " << imsi << ", RNTI " << rnti
            << ", Cell id " << cellId << ", radio link failure detected"
            << std::endl << std::endl;

  PrintUePosition (imsi);

  if (cellId == 1)
    {
      NS_ABORT_MSG_IF ((Simulator::Now () - t310StartTimeFirstEnb) != t310, "T310 timer expired at wrong time");
    }
}

void
NotifyRandomAccessErrorUe (uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds ()
            << " IMSI " << imsi << ", RNTI " << rnti << ", Cell id " << cellId
            << ", UE RRC Random access Failed" << std::endl;
}

void
NotifyConnectionTimeoutUe (uint64_t imsi, uint16_t cellId, uint16_t rnti,
                           uint8_t connEstFailCount)
{
  std::cout << Simulator::Now ().GetSeconds ()
            << " IMSI " << imsi << ", RNTI " << rnti
            << ", Cell id " << cellId
            << ", T300 expiration counter " << (uint16_t) connEstFailCount
            << ", UE RRC Connectio timeout" << std::endl;
}

void
NotifyRaResponseTimeoutUe (uint64_t imsi, bool contention,
                           uint8_t preambleTxCounter,
                           uint8_t maxPreambleTxLimit)
{
  std::cout << Simulator::Now ().GetSeconds ()
            << " IMSI " << imsi << ", Contention flag " << contention
            << ", preamble Tx Counter " << (uint16_t) preambleTxCounter
            << ", Max Preamble Tx Limit " << (uint16_t) maxPreambleTxLimit
            << ", UE RA response timeout" << std::endl;
  NS_FATAL_ERROR ("NotifyRaResponseTimeoutUe");
}

static void
RxDrop (Ptr<const Packet> p)
{
  std::cout<<"RxDrop at " << Simulator::Now ().GetSeconds ()<<std::endl;
}

void 
ReportUeMeasurementsCallbackDetail (Ptr<OutputStreamWrapper> stream,std::string path, uint16_t rnti, uint16_t cellId, double rsrp, double rsrq, bool servingCell, uint8_t carrier){
    // std::cout << "Ue Measurements"<< std::endl;
    // std::cout << path <<std::endl;
    // std::cout << "cellid:" << cellId<< std::endl;
    // std::cout << "servingCell:" << servingCell << std::endl;
    // std::cout << "carrier:" << carrier << std::endl;
    // std::cout << "rnti:" << rnti<< std::endl;
    // std::cout << "rsrp:" << rsrp<< std::endl;
    // std::cout << "rsrq:" << rsrq<< std::endl; 
    // std::cout << "Time:" << Simulator::Now().GetSeconds()<<std::endl; 
    
    // info actual_info;
    // actual_info.rnti = rnti;

    // info_vector.push_back(actual_info);

    // std::ofstream fout;
    // fout.open("Ue_Measurements.txt");
     *stream->GetStream() <<Simulator::Now()<< cellId << "\t"<< servingCell<< "\t"<< rnti << "\t"<< rsrp << "\t"<< rsrq << std::endl;


  //   for (uint16_t l = 0; l < info_vector.size(); l++){
  //   fout << info_vector.at(l).cellId  << "\t";
  //   fout << info_vector.at(l).servingCell  << "\t";
  //   fout << info_vector.at(l).rnti  << "\t";
  //   fout << info_vector.at(l).rsrp  << "\t";
  //   fout << info_vector.at(l).rsrq  << std::endl;
  // }

  //   fout.close();
   }
  // }

void
NotifyUeReport (Ptr<OutputStreamWrapper> stream, std::string context, uint16_t cellid, uint16_t rnti, double rsrp , double avsinr, uint8_t componentCarrier)
{
     // std::cout << " eNB CellId " << cellid << std::endl
     //            << "rnti:" << rnti<< std::endl
     //            << " RSRP " << rsrp << std::endl
     //            << " SINR " << 10*log(avsinr) / log(10) << std::endl
     //            <<  "Time:" << Simulator::Now().GetSeconds()
     //            << std::endl;


    int nth=3;  //looking for the second ocurrence of "/"
    int cnt=0; 
    size_t pos=0;

    while( cnt != nth )
    {
        pos = context.find("/", pos);
        if ( pos == std::string::npos )
          std::cout << nth << "th ocurrence not found!"<< std::endl;
        pos+=1;
        cnt++;
    }

    // std::cout << "test" <<context<< std::endl;


    //manipulate the string to get the node ID
    std::string str1;
    str1 = context.substr(10,pos-11);
    int ue= atoi(str1.c_str());



    //Get the UE position from the NodeList
    Ptr<MobilityModel> m1 = ns3::NodeList::GetNode(ue)->GetObject<MobilityModel>();
    

//    Get the eNB position from the eNB
     Ptr<MobilityModel> m2 = ns3::NodeList::GetNode(0)->GetObject<MobilityModel>();
   

    // //print UE and eNB location
    Vector v1 = m1->GetPosition();
 
    Vector v2 = Vector (25, 25 , 7);

    // std::cout << Simulator::Now().GetSeconds()<<std::endl; 
    // std::cout <<"\tUE:" << rnti << " - (" << v1.x << ", " << v1.y << "," <<v1.z <<")"<<std::endl;
    // std::cout << " \teNB:" << 1<< " - (" << v2.x << ", " << v2.y <<"," <<v2.z << ")"<<std::endl;

    //Print the distante between UE and eNB
    // double distance = m1->GetDistanceFrom (m2);
    Vector distance = v2-v1;
    // std::cout << " Distance : " << distance << std::endl;
     *stream->GetStream() << Simulator::Now().GetSeconds() <<"\t"<<rnti<<"\t"<<rsrp<< "\t" << 10*log(avsinr) / log(10) << "\t"<<v1<<"\t"<<distance<<std::endl;
}


void NotifyConnectionEstablishedEnb (Ptr<OutputStreamWrapper> stream,std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " "
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;

       *stream->GetStream() << Simulator::Now().GetSeconds() <<"\t"<<cellid<<"\t"<<rnti<< "\t" << imsi << std::endl;
    // std::ofstream fout;
    // fout.open("Ue_imsi.txt");
    // fout<<cellid <<"\t";
    // fout<<imsi<<"\t";
    // fout<<rnti<<"\t" << std::endl;

    // fout.close();


            // << "Message" << msg.measResults.rsrpResult
}



NS_LOG_COMPONENT_DEFINE ("RelayingSimulator");

int
main (int argc, char *argv[])
{
  double simTime = 10;
  uint16_t n310 = 1;
  Time t310 = Seconds (1);
  uint16_t n311 = 1;
  // bool useCa = false;
   
 
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  // cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.Parse (argc, argv);
  // Command line arguments

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);
    // Configure the scheduler
  Config::SetDefault ("ns3::RrSlFfMacScheduler::Itrp", UintegerValue (0));
  //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::RrSlFfMacScheduler::SlGrantSize", UintegerValue (5));

  Config::SetDefault ("ns3::LteHelper::Scheduler", StringValue("ns3::PfFfMacScheduler"));

  // Config::SetDefault ("ns3::LteHelper::PathlossModel", StringValue( "ns3::ThreeGppIndoorFactoryPropagationLossModel"));
  Config::SetDefault ("ns3::LteHelper::PathlossModel", StringValue( "ns3::Cost231PropagationLossModel"));
  // Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Frequency", DoubleValue(6e9));
  //   Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue(true));
  Config::SetDefault ("ns3::ThreeGppPropagationLossModel::ShadowingEnabled",  BooleanValue (true));
  Config::SetDefault ("ns3::ThreeGppPropagationLossModel::Frequency",  DoubleValue (800e6));
  
  // Config::SetDefault ("ns3::ThreeGppPropagationLossModel::ChannelModel",  Ptr<ThreeGppIndoorFactoryChannelConditionModel>); 
    // Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue("InF-Factory"));
  Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(100)); // 20 mhz 
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(100)); //20 mhz  
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue( 6250)); // 800 mhz 
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue(24250)); // 800 mhz
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue(1)); //0.1 W =20
  Config::SetDefault ("ns3::LteUePhy::NoiseFigure", DoubleValue(9));
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue(1)); // 0.5 W =26
  Config::SetDefault ("ns3::LteEnbPhy::NoiseFigure", DoubleValue(5)); 
  Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));
  Config::SetDefault ("ns3::LteAmc::Ber", DoubleValue (0.01));
  Config::SetDefault ("ns3::PfFfMacScheduler::HarqEnabled", BooleanValue (true));

  //Error Model
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue(true));
  // Config::SetDefault ("ns3::UrbanMacroCellPropagationLossModel::LosEnabled", BooleanValue (true));
  // Config::SetDefault ("ns3::Hybrid3gppPropagationLossModel::ShadowingEnabled", BooleanValue (false));
  // Config::SetDefault ("ns3::Hybrid3gppPropagationLossModel::CacheLoss", BooleanValue (false));

    //Radio link failure detection parameters
  Config::SetDefault ("ns3::LteUeRrc::N310", UintegerValue (n310));
  Config::SetDefault ("ns3::LteUeRrc::N311", UintegerValue (n311));
  Config::SetDefault ("ns3::LteUeRrc::T310", TimeValue (t310));
  // Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (LteEnbRrc::RLC_AM_ALWAYS) );


  // if (useCa)
  //  {
  //    Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
  //    Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
  //    Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
  //  }

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSpectrumChannelType ("ns3::MultiModelSpectrumChannel");

  lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
  std::ifstream ifTraceFile;
  ifTraceFile.open ("../../src/lte/model/fading-traces/fading_trace_ETU_3kmph.fad", std::ifstream::in);
  if (ifTraceFile.good ())
  {
    // script launched by test.py
    lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("../../src/lte/model/fading-traces/fading_trace_ETU_3kmph.fad"));
  }
  else
  {
    // script launched as an example
    lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("src/lte/model/fading-traces/fading_trace_ETU_3kmph.fad"));
    NS_LOG_UNCOND ("Trace file load fail");
  }

  //Sidelink Helper
  Ptr<LteSidelinkHelper> SideLinkHelper = CreateObject<LteSidelinkHelper> ();
  SideLinkHelper->SetLteHelper (lteHelper);



  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));
  //SideLink Scheduler
  lteHelper->SetSchedulerType ("ns3::RrSlFfMacScheduler");
    // lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::ThreeGppIndoorOfficePropagationLossModel"));
    // Set pathloss model
  // lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Hybrid3gppPropagationLossModel"));
  //   lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));
  // lteHelper->SetAttribute ("ChannelModel", StringValue ("ns3::MmWave3gppChannel"));

   //Create nodes
    NodeContainer eNbNodes;
    NodeContainer ueNodes;
    NodeContainer ueNodesMobile;
    NodeContainer ueNodesStatic;
    eNbNodes.Create (1);
    // ueNodes.Create (ueNum);
    uint16_t ueNumMobile = 10;
    uint16_t ueNumStatic = 10;

    ueNodesMobile.Create (ueNumMobile);
    ueNodesStatic.Create (ueNumStatic);
    ueNodes.Add(ueNodesMobile);
    ueNodes.Add(ueNodesStatic);

   // double ueX = 1.00, ueY = 0.00;

    //Positioning the nodes 
  Ptr<ListPositionAllocator> eNbpositionAlloc = CreateObject<ListPositionAllocator> ();

      eNbpositionAlloc->Add (Vector (25, 25 , 7.5));
  
  // Ptr<ListPositionAllocator> UEpositionAlloc = CreateObject<ListPositionAllocator> ();
  //   for (uint16_t i = 0; i < ueNum; i++)
  //   {
  //     UEpositionAlloc->Add (Vector (distance * i, 0, 1));
  //   }
  //Setup the mobility and position of the nodes 
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(eNbpositionAlloc);
  mobility.Install (eNbNodes);
  // mobility.SetPositionAllocator(UEpositionAlloc);
  // mobility.Install (ueNodes);
  // RngSeedManager::SetSeed (1);
  // RngSeedManager::SetRun (1);
  Ptr<RandomBoxPositionAllocator> randomUePositionAlloc = CreateObject<RandomBoxPositionAllocator> ();
  Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
  xVal->SetAttribute ("Min", DoubleValue (0));
  xVal->SetAttribute ("Max", DoubleValue (50));
  randomUePositionAlloc->SetAttribute ("X", PointerValue (xVal));
  Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
  yVal->SetAttribute ("Min", DoubleValue (0));
  yVal->SetAttribute ("Max", DoubleValue (50));
  randomUePositionAlloc->SetAttribute ("Y", PointerValue (yVal));
     Ptr<ConstantRandomVariable> ueRandomVarZ = CreateObject<ConstantRandomVariable>();
   ueRandomVarZ->SetAttribute("Constant", DoubleValue(0.3));
 
  // zVal->SetAttribute ("Min", DoubleValue (macroUeBox.zMin));
  // zVal->SetAttribute ("Max", DoubleValue (macroUeBox.zMax));
  randomUePositionAlloc->SetZ(ueRandomVarZ);
  MobilityHelper mobilityHelperUeMobile;
   // mobilityHelperUeMobile.SetMobilityModel( "ns3::RandomWaypointMobilityModel");
    mobilityHelperUeMobile.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                            "Mode", StringValue ("Time"),
                            "Time", StringValue ("1s"),
                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=3.0]"),
                            "Bounds", StringValue ("0|50|0|50"));
  
  mobilityHelperUeMobile.SetPositionAllocator (randomUePositionAlloc);
  mobilityHelperUeMobile.Install (ueNodesMobile);

//    Ptr<UniformRandomVariable> ueRandomVarX = CreateObject<UniformRandomVariable>();
//  // Ptr<RandomBoxPositionAllocator> ueRandomRectPosAlloc = CreateObject<RandomBoxPositionAllocator> ();
//    ueRandomVarX->SetAttribute ("Min", DoubleValue (0));
//    ueRandomVarX->SetAttribute ("Max", DoubleValue (50));
//    // ueRandomRectPosAlloc->SetX(ueRandomVarX);
//    Ptr<UniformRandomVariable> ueRandomVarY = CreateObject<UniformRandomVariable>();
//    ueRandomVarY->SetAttribute ("Min", DoubleValue (1));
//    ueRandomVarY->SetAttribute ("Max", DoubleValue (50));
//    // // ueRandomRectPosAlloc->SetY(ueRandomVarY);
//    Ptr<ConstantRandomVariable> ueRandomVarZ = CreateObject<ConstantRandomVariable>();
//    ueRandomVarZ->SetAttribute("Constant", DoubleValue(0.3));
//    // ueRandomRectPosAlloc->SetZ(ueRandomVarZ);
//    Ptr<ListPositionAllocator> posAllocUeMobile = CreateObject<ListPositionAllocator>();

//  for (uint8_t j = 0; j < ueNumMobile; j++)
//    {
//        for (uint8_t i = 0; i < 10; i++)
//        {
//          posAllocUeMobile->Add( Vector(ueX*i,ueY*j,0.3) );
//    }
//  }
//    Ptr<RandomBoxPositionAllocator> wpAllocUeMobile = CreateObject<RandomBoxPositionAllocator>();
//    wpAllocUeMobile->SetX(ueRandomVarX);    wpAllocUeMobile->SetX(ueRandomVarY);    wpAllocUeMobile->SetZ(ueRandomVarZ);
//  // Ptr<ListPositionAllocator> posAllocUeMobile = CreateObject<ListPositionAllocator>();

//    MobilityHelper mobilityHelperUeMobile;
//    mobilityHelperUeMobile.SetMobilityModel( "ns3::RandomWaypointMobilityModel");
//     mobilityHelperUeMobile.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
//                             "Mode", StringValue ("Time"),
//                             "Time", StringValue ("1s"),
//                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=3.0]"),
//                             "Bounds", StringValue ("0|50|0|50"));
//    //
//    //     "Speed", StringValue("ns3::ConstantRandomVariable[Constant=16.67]"),    // in 'm/s'
//    //     "Pause", StringValue("ns3::ConstantRandomVariable[Constant=0.00]"),
//    //     "PositionAllocator", PointerValue(wpAllocUeMobile) );
//    mobilityHelperUeMobile.SetPositionAllocator(wpAllocUeMobile);
//    mobilityHelperUeMobile.Install( ueNodesMobile );
// //    MobilityHelper mobilityHelperUeMobile;
//    mobilityHelperUeMobile.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//    mobilityHelperUeMobile.SetPositionAllocator (posAllocUeMobile);
//    mobilityHelperUeMobile.Install (ueNodesMobile);
// //
   Ptr<ListPositionAllocator> posAllocUeStatic = CreateObject<ListPositionAllocator> ();

   for (uint8_t j = 0; j < 8; j++)
     {
       for (uint8_t i = 0; i < 10; i++)
         {
           posAllocUeStatic->Add(Vector( i*5, j*6, 1));
         }
     }
   MobilityHelper mobilityHelperUeStatic;
   mobilityHelperUeStatic.SetMobilityModel ( "ns3::RandomWaypointMobilityModel");
    mobilityHelperUeStatic.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                            "Mode", StringValue ("Time"),
                            "Time", StringValue ("1s"),
                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=0.1]"),
                            "Bounds", StringValue ("0|50|0|50"));

   mobilityHelperUeStatic.SetPositionAllocator (posAllocUeStatic);
   mobilityHelperUeStatic.Install (ueNodesStatic);




  //Get the position of the nodes 
  // for (NodeContainer::Iterator j = ueNodes.Begin ();j != ueNodes.End (); ++j)
  //   {
  //     Ptr<Node> object = *j;
  //     Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
  //     NS_ASSERT (position != 0);
  //     Vector pos = position->GetPosition ();
  //     std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
  //   }

    for (NodeContainer::Iterator j = eNbNodes.Begin ();j != eNbNodes.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Vector pos = position->GetPosition ();
      std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
    }

    // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;
  // Default scheduler is PF, uncomment to use RR
  //lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  enbDevs = lteHelper->InstallEnbDevice (eNbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);

  //Configure SideLink Pool
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

  uint32_t groupL2Address = 250;

  enbSidelinkConfiguration->AddPreconfiguredDedicatedPool (groupL2Address, pool);
  lteHelper->InstallSidelinkConfiguration (enbDevs, enbSidelinkConfiguration);

  //pre-configuration for the UEs
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);
  LteRrcSap::SlPreconfiguration preconfiguration;
  // ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);

  preconfiguration.preconfigGeneral.carrierFreq = 18100;
  preconfiguration.preconfigGeneral.slBandwidth = 100;
  preconfiguration.preconfigComm.nbPools = 1;
    LteSlDiscPreconfigPoolFactory pfactory_disc;
  //discovery
  pfactory_disc.SetDiscCpLen ("NORMAL");
  pfactory_disc.SetDiscPeriod ("rf32");
  pfactory_disc.SetNumRetx (0);
  pfactory_disc.SetNumRepetition (1);

  pfactory_disc.SetDiscPrbNum (2);
  pfactory_disc.SetDiscPrbStart (1);
  pfactory_disc.SetDiscPrbEnd (2);
  pfactory_disc.SetDiscOffset (0);
  pfactory_disc.SetDiscBitmap (0x00001);
  
  // LteSlPreconfigPoolFactory pfactory;
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

  // preconfiguration.preconfigDisc.pools[0] = pfactory_disc.CreatePool ();
  // preconfiguration.preconfigDisc.nbPools = 1;

  // preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();
  // preconfiguration.preconfigComm.nbPools = 1;
     /* Synchronization*/
  //Synchronization parameters
  preconfiguration.preconfigSync.syncOffsetIndicator1 = 18;
  preconfiguration.preconfigSync.syncOffsetIndicator2 = 29;
  preconfiguration.preconfigSync.syncTxThreshOoC = -60; //dBm;
  preconfiguration.preconfigSync.syncRefDiffHyst = 0; //dB;
  preconfiguration.preconfigSync.syncRefMinHyst = 0; //dB;
  preconfiguration.preconfigSync.filterCoefficient = 0;  //k = 4 ==> a = 0.5, k = 0 ==> a = 1 No filter;
  /* END Synchronization*/

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);

   // Create a single RemoteHost
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);
  // uint32_t groupL2Address =255;

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0.000)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Address remoteAddress;
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  Ipv4Address groupIP =  ("255.0.0.0");
  remoteAddress = InetSocketAddress (groupIP, 8000);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  internet.Install (ueNodes);
  Ptr<LteSlTft> tft;
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));
  
  
    // Set the default gateway for the UEs
    for (uint32_t j = 0; j < ueNodes.GetN(); ++j)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get(j)->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
      
    } 
    tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupIP,groupL2Address );
      //Printing IMSI, IP address and Position 
     for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
      Ptr<MobilityModel> m1 = ueNodes.Get(u)->GetObject<MobilityModel>();
      std::cout<<"IP Address: "<<ueNodes.Get(u)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal() <<"IMSI: "
      << ueNodes.Get(u)->GetDevice(0)->GetObject <LteUeNetDevice>()->GetImsi() << "POS:"<< m1->GetPosition() <<std::endl;
      
    } 

    if (Simulator::Now () == 2.00)
    {  
      std::cout<<"IMSI:"<<ueNodes.Get(100)->GetDevice(0)->GetObject <LteUeNetDevice>()->GetImsi()<<std::endl;
      std::cout<<"Power"<< ueNodes.Get(100)-> GetDevice(0)->GetObject<LteUePhy>()-> GetTxPower();
      ueNodes.Get(100)-> GetDevice(0)->GetObject<LteUePhy>()-> SetTxPower(1);
    }
    std::cout<<remoteHostAddr<<std::endl;

  // Attach a UE to a eNB
  lteHelper->Attach (ueDevs, enbDevs.Get(0));

  // error model
Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
internetDevices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  Ptr<ExponentialRandomVariable> emergencyIntervalSeconds = CreateObject<ExponentialRandomVariable> ();
  emergencyIntervalSeconds->SetAttribute("Mean", DoubleValue(simTime));     // To be defined
  emergencyIntervalSeconds->SetAttribute("Bound", DoubleValue(simTime));
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  // uint16_t d2dPort = 4000;
  double interPacketInterval = 15;
  uint16_t packetSize =250 ; //bytes 
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  AsciiTraceHelper asciiTraceHelper;
  std::ostringstream oss;
  Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream ("UePacketTrace.tr");

  //Trace file table header
  *stream3->GetStream () << "time(sec)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      ++otherPort;
      // PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (),d2dPort) );
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));
      // serverApps.Add (sidelinkSink.Install(ueNodes.Get(u)));
      // serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute("PacketSize", UintegerValue(packetSize));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute("PacketSize", UintegerValue(packetSize));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
      client.SetAttribute ("Interval", TimeValue (Seconds(emergencyIntervalSeconds->GetValue())));
      client.SetAttribute ("MaxPackets", UintegerValue(1000000));
      client.SetAttribute("PacketSize", UintegerValue(packetSize));

      // OnOffHelper sidelinkClient ("ns3::UdpSocketFactory", remoteAddress);
      // sidelinkClient.SetConstantRate (DataRate ("16kb/s"), 200);
     
      // clientApps.Add(sidelinkClient.Install(ueNodes.Get(u+1)));
      clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      clientApps.Add (client.Install (remoteHost));

    
   for (uint16_t ac = 0; ac < clientApps.GetN (); ac++)
  {
    Ipv4Address localAddrs =  clientApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
    std::cout << "Tx address: " << localAddrs << std::endl;
    oss << "tx\t" << ueNodes.Get (u)->GetId () << "\t" << ueNodes.Get (u)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
    clientApps.Get (ac)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream3, localAddrs));
    oss.str ("");
  }

  // Set Rx traces
  for (uint16_t ac = 0; ac < serverApps.GetN (); ac++)
  {
    Ipv4Address localAddrs =  serverApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
    std::cout << "Rx address: " << localAddrs << std::endl;
    oss << "rx\t" << ueNodes.Get (u)->GetId () << "\t" << ueNodes.Get (u)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
    serverApps.Get (ac)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream3, localAddrs));
    oss.str ("");
  }

  }
  SideLinkHelper->ActivateSidelinkBearer (Seconds(0.1), ueDevs, tft);  
  serverApps.Start (Seconds (1));
  clientApps.Start (Seconds (1));
  serverApps.Stop(Seconds (simTime));
  clientApps.Stop(Seconds (simTime));


   // std::cout<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"<<Simulator::Now().GetSeconds()<<std::endl;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("ueReport.txt");

  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("Ueimsi.txt");
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream ("Ue_Measurement.txt");

      //check wheather the packet is dropped at the physical layer
    internetDevices.Get (1)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback (&RxDrop));

  // std::cout<<Simulator::Now().GetSeconds()<<std::endl;
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::LteUeNetDevice/ComponentCarrierMapUe/*/LteUePhy/ReportCurrentCellRsrpSinr",  MakeBoundCallback (&NotifyUeReport, stream));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::LteUeNetDevice/ComponentCarrierMapUe/*/LteUePhy/ReportUeMeasurements", MakeBoundCallback (&ReportUeMeasurementsCallbackDetail, stream2));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished", MakeBoundCallback (&NotifyConnectionEstablishedEnb,stream1));
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteUeRrc/StateTransition",
                                 MakeCallback (&UeStateTransition));
      Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteEnbRrc/NotifyConnectionRelease",
                                 MakeCallback (&NotifyConnectionReleaseAtEnodeB));
        Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteEnbRrc/RrcTimeout",
                                 MakeCallback (&EnbRrcTimeout));
         Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteUeRrc/RandomAccessError",
                                 MakeCallback (&NotifyRandomAccessErrorUe));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionTimeout",
                                   MakeCallback (&NotifyConnectionTimeoutUe));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteUeMac/RaResponseTimeout",
                                   MakeCallback (&NotifyRaResponseTimeoutUe));
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteUeRrc/PhySyncDetection",
                                 MakeBoundCallback (&PhySyncDetection, n310));
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteUeRrc/RadioLinkFailure",
                                 MakeBoundCallback (&RadioLinkFailure, t310));
 

  lteHelper->EnableTraces ();
  ns3::PacketMetadata::Enable ();
  //Side Link Traces 
  lteHelper->EnableSlPscchMacTraces ();
  lteHelper->EnableSlPsschMacTraces ();

  lteHelper->EnableSlRxPhyTraces ();
  lteHelper->EnableSlPscchRxPhyTraces ();
   lteHelper->EnableDiscoveryMonitoringRrcTraces ();
  // Uncomment to enable PCAP tracing
  p2ph.EnablePcapAll("basic_first");

  Simulator::Stop(Seconds(simTime+0.5));
  AnimationInterface anim("ltetryd2d.xml");
  anim.SetMaxPktsPerTraceFile(500000);
  anim.EnablePacketMetadata (true);
  Simulator::Run();

  std::string outputDir = "./";
    std::string simTag= "test1";
    flowMonitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats ();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;
    uint32_t sumLostPackets = 0.0;
    uint32_t sumTxPackets = 0.0;
    uint32_t delayPacketSum = 0.0; 
    double delayDrop = 0.0; 

    std::ofstream outFile;
    std::string filename = outputDir + "/" + simTag;
    outFile.open (filename.c_str (), std::ofstream::out | std::ofstream::app);
    if (!outFile.is_open ())
    {
      std::cout<<"Can't open file " << filename;
      return 1;
    }
    outFile.setf (std::ios_base::fixed);

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
      std::stringstream protoStream;
      protoStream << (uint16_t) t.protocol;
      if (t.protocol == 6)
        {
          protoStream.str ("TCP");
        }
      if (t.protocol == 17)
        {
          protoStream.str ("UDP");
        }
    //   for (uint32_t reasonCode = 0; reasonCode < i->second.packetsDropped.size (); reasonCode++)
    // {
    //   outFile<<"Print in";
    //   outFile << "<packetsDropped reasonCode=" << reasonCode
    //       << " number=" << i->second.packetsDropped[reasonCode] << "\n";

    // }
        outFile << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> " << t.destinationAddress << ":" << t.destinationPort << ") proto " << protoStream.str () << "\n"; 
        outFile << "  Tx Packets: " << i->second.txPackets << "\n";
        outFile << "  Tx Bytes:   " << i->second.txBytes << "\n";
        outFile << "  TxOffered:  " << i->second.txBytes * 8.0 / (simTime - 0.01) / 1000 / 1000  << " Mbps\n";
        outFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        outFile << "  Lost Packets: " << i->second.lostPackets<< "\n";
        outFile << "  Packet Forwards: "<< i->second.timesForwarded<<"\n";
        // outFile << " Packets Dropped : " << i-> second.packetsDropped<<"\n";
        // outFile << " Byte Dropped: " << i->second.bytesDropped<<"\n";
        sumTxPackets += i->second.txPackets;
        sumLostPackets+= i->second.lostPackets;
      if (i->second.rxPackets > 0)
        {
          // Measure the duration of the flow from receiver's perspective
          double rxDuration = i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ();

          averageFlowThroughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
          averageFlowDelay += 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets;
          delayDrop = i->second.lastDelay.GetSeconds(); 
          std::cout << "Delay per packet"<< delayDrop; 
        if (delayDrop > 10)
        {
          delayPacketSum ++; 

        }



          outFile << "  Throughput: " << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000  << " Mbps\n";
          outFile << "  Mean delay:  " << 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets << " ms\n";
          //outFile << "  Mean upt:  " << i->second.uptSum / i->second.rxPackets / 1000/1000 << " Mbps \n";
          outFile << "  Mean jitter:  " << 1000 * i->second.jitterSum.GetSeconds () / i->second.rxPackets  << " ms\n";

//          for (uint32_t reasonCode = 0; reasonCode < i->second.packetsDropped.size (); reasonCode++)
//        {
//          outFile<<"Print in";
//          outFile << "<packetsDropped reasonCode=" << reasonCode
//              << " number=" << i->second.packetsDropped[reasonCode] << "\n";
//
//        }
        // std::cout<<i->second.packetsDropped(0)<<std::endl;
        // for (uint32_t reasonCode = 0; reasonCode < i->second.bytesDropped.size (); reasonCode++)
        // {
        //   std::cout<<"\n"<< "<bytesDropped reasonCode=\"" << reasonCode << "\""
        //       << " bytes=\"" << i->second.bytesDropped[reasonCode] << std::endl;
        // }
        }
      else
        {
          outFile << "  Throughput:  0 Mbps\n";
          outFile << "  Mean delay:  0 ms\n";
          outFile << "  Mean upt:  0  Mbps \n";
          outFile << "  Mean jitter: 0 ms\n";
        }
      outFile << "\n" << "Loss Packet %" <<(((i->second.txPackets-i->second.rxPackets)*1.0)/i->second.txPackets);
      // outFile << "\n" << "Lost Packets:" << sumLostPackets;
      outFile << "  Rx Packets: " << i->second.rxPackets << "\n";

    }


    outFile << "\n" << "Packets Transmitted" << sumTxPackets << "\n";
    outFile << "\n" << "Packets Lost" << sumLostPackets << "\n";
    outFile << "\n" << "Packet Lost due to delay"<< delayPacketSum << "\n";
    
    outFile << "\n\n  Mean flow throughput: " << averageFlowThroughput / stats.size() << "\n";
    outFile << "  Mean flow delay: " << averageFlowDelay / stats.size () << "\n";
    outFile <<"----------------------------------------Next Simulation -------------------------------------------------------------------------------------" << "\n";
  outFile.close ();

  flowMonitor->SerializeToXmlFile("test1.xml", true, true);
  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}