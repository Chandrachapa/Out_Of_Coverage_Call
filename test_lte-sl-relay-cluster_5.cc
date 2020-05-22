#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/lte-module.h>
#include <ns3/netanim-module.h>
#include "ns3/lte-module.h"
#include "ns3/lte-helper.h"
#include "ns3/lte-sidelink-helper.h"
#include "ns3/lte-sl-preconfig-pool-factory.h"
#include <ns3/mcptt-helper.h>
#include <ns3/wifi-module.h>
#include <ns3/udp-client-server-helper.h>
#include <ns3/udp-echo-helper.h>
#include <ns3/packet-sink-helper.h>
#include <ns3/psc-module.h>

using namespace ns3;
using namespace psc;

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


/*Synchronization traces*/
void
NotifyChangeOfSyncRef (Ptr<OutputStreamWrapper> stream, LteUeRrc::SlChangeOfSyncRefStatParameters param)
{
  *stream->GetStream () << Simulator::Now ().GetMilliSeconds () << "\t" << param.imsi << "\t" << param.prevSlssid << "\t" << param.prevRxOffset << "\t" << param.prevFrameNo << "\t" << param.prevSubframeNo <<
  "\t" << param.currSlssid << "\t" << param.currRxOffset << "\t" << param.currFrameNo << "\t" << param.currSubframeNo << std::endl;
}

void
NotifySendOfSlss (Ptr<OutputStreamWrapper> stream, uint64_t imsi, uint64_t slssid, uint16_t txOffset, bool inCoverage, uint16_t frame, uint16_t subframe)
{
  *stream->GetStream () << Simulator::Now ().GetMilliSeconds () << "\t" << imsi << "\t"  << slssid << "\t" << txOffset << "\t" << inCoverage << "\t" << frame <<  "\t" << subframe << std::endl;
}



int main (int argc, char *argv[])
{//the rest of the simulation program follows
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
    Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
    proseHelper->SetLteHelper (lteHelper);

    lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Hybrid3gppPropagationLossModel"));


    //Enable Sidelink
    lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));
    lteHelper->Initialize ();

    NodeContainer enbNodes;
    enbNodes.Create (1);
    NodeContainer ueNodes;
    ueNodes.Create (2);

    Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator> ();
    positionAllocEnb->Add (Vector (0.0, 0.0, 1.5));
    //Relay UE
    Ptr<ListPositionAllocator> positionAllocUe1 = CreateObject<ListPositionAllocator> ();
    positionAllocUe1->Add (Vector (20, 0.0, 1.5));
    //Remote UE
    Ptr<ListPositionAllocator> positionAllocUe2 = CreateObject<ListPositionAllocator> ();
    positionAllocUe2->Add (Vector (30, 0.0, 1.5));

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (enbNodes);
    mobility.SetPositionAllocator (positionAllocEnb);

    //Relay UE
    MobilityHelper mobilityUe1;
    mobilityUe1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityUe1.SetPositionAllocator (positionAllocUe1);
    mobilityUe1.Install (ueNodes.Get (1));

    //Remote UE
    MobilityHelper mobilityUe2;
    mobilityUe2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityUe2.SetPositionAllocator (positionAllocUe2);
    mobilityUe2.Install (ueNodes.Get (0));

    lteHelper->DisableEnbPhy (true);

    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice (enbNodes);

    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice (ueNodes);
    

    //Configure Sidelink
    Ptr<LteSlEnbRrc> enbSidelinkConfiguration = CreateObject<LteSlEnbRrc> ();
    enbSidelinkConfiguration->SetSlEnabled (true);

    //Configure communication pool
    LteRrcSap::SlCommTxResourcesSetup pool_comm;

    pool_comm.setup = LteRrcSap::SlCommTxResourcesSetup::UE_SELECTED;
    pool_comm.ueSelected.havePoolToRelease = false;
    pool_comm.ueSelected.havePoolToAdd = true;
    pool_comm.ueSelected.poolToAddModList.nbPools = 1;
    pool_comm.ueSelected.poolToAddModList.pools[0].poolIdentity = 1;

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

    pool_comm.ueSelected.poolToAddModList.pools[0].pool =  pfactory.CreatePool ();
    enbSidelinkConfiguration->SetDefaultPool (pool_comm);
    enbSidelinkConfiguration->SetDiscEnabled (true);

    LteRrcSap::SlDiscTxResourcesSetup discPool;
    discPool.setup =  LteRrcSap::SlDiscTxResourcesSetup::UE_SELECTED;
    discPool.ueSelected.havePoolToRelease = false;
    discPool.ueSelected.havePoolToAdd = true;
    discPool.ueSelected.poolToAddModList.nbPools = 1;
    discPool.ueSelected.poolToAddModList.pools[0].poolIdentity = 1;

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

    discPool.ueSelected.poolToAddModList.pools[0].pool =  pDiscFactory.CreatePool ();

    enbSidelinkConfiguration->AddDiscPool (discPool);

    //Install Sidelink configuration for eNBs
    lteHelper->InstallSidelinkConfiguration (enbDevs, enbSidelinkConfiguration);
  
    //Configure Sidelink Preconfiguration for the UEs
Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
ueSidelinkConfiguration->SetSlEnabled (true);
LteRrcSap::SlPreconfiguration preconfigurationRemote;
LteRrcSap::SlPreconfiguration preconfigurationRelay;

std::cout << "ok 15" << std::endl;

bool useRelay = true;

 if (useRelay)
    {
      //General
      preconfigurationRemote.preconfigGeneral.carrierFreq = 23330;
      preconfigurationRemote.preconfigGeneral.slBandwidth = 50;

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

   
      //synchronization
      preconfigurationRemote.preconfigSync.syncOffsetIndicator1 = 18;
      preconfigurationRemote.preconfigSync.syncOffsetIndicator2 = 29;
      preconfigurationRemote.preconfigSync.syncTxThreshOoC = -60; //dBm;
      preconfigurationRemote.preconfigSync.syncRefDiffHyst = 0; //dB;
      preconfigurationRemote.preconfigSync.syncRefMinHyst = 0; //dB;
      preconfigurationRemote.preconfigSync.filterCoefficient = 0;  //k = 4 ==> a = 0.5, k = 0 ==> a = 1 No filter;
         //-Relay UE (re)selection
      preconfigurationRemote.preconfigRelay.haveReselectionInfoOoc = true;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.filterCoefficient = 0;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.minHyst = 0;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.qRxLevMin = -125;
      

      ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRemote);
      lteHelper->InstallSidelinkConfiguration (ueDevs.Get (0), ueSidelinkConfiguration);
      
      std::cout << "downlink frequency remote: " << ueDevs.Get (0)->GetObject<LteUeNetDevice> ()-> GetDlEarfcn() << std::endl;
      std::cout << "downlink frequency relay: "  << ueDevs.Get (1)->GetObject<LteUeNetDevice> ()-> GetDlEarfcn() << std::endl;
      
    }

    //general configs for all UEs
    ueSidelinkConfiguration->SetDiscEnabled (true);
    
    uint8_t nb = 3;
    ueSidelinkConfiguration->SetDiscTxResources (nb);
    ueSidelinkConfiguration->SetDiscInterFreq (enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlEarfcn ());
    
    ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRelay);
    lteHelper->InstallSidelinkConfiguration (ueDevs.Get (1), ueSidelinkConfiguration);

    ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRemote);
    lteHelper->InstallSidelinkConfiguration (ueDevs.Get (0), ueSidelinkConfiguration);
    

    uint16_t base_t = 2000; //ms

    uint16_t devIt = 1;
    ueDevs.Get (devIt)->GetObject<LteUeNetDevice> ()->GetRrc ()->SetSlssid (devIt + 10);
    ueDevs.Get (devIt)->GetObject<LteUeNetDevice> ()->GetPhy ()->SetFirstScanningTime (MilliSeconds (base_t + (devIt * base_t)));
    std::cout << "IMSI " << ueDevs.Get (devIt)->GetObject<LteUeNetDevice> ()->GetRrc ()->GetImsi ()
            << " SLSSID " << devIt + 10
            << " First Scan "  << base_t + (devIt * base_t) << " ms" << std::endl;

    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> streamSyncRef = ascii.CreateFileStream ("SyncRef.txt");
    *streamSyncRef->GetStream () << "Time\tIMSI\tprevSLSSID\tprevRxOffset\tprevFrameNo\tprevSframeNo\tcurrSLSSID\tcurrRxOffset\tcurrFrameNo\tcurrSframeNo" << std::endl;

    Ptr<OutputStreamWrapper> streamSendOfSlss = ascii.CreateFileStream ("TxSlss.txt");
    *streamSendOfSlss->GetStream () << "Time\tIMSI\tSLSSID\ttxOffset\tinCoverage\tFrameNo\tSframeNo" << std::endl;

    for (uint32_t i = 0; i < ueDevs.GetN (); ++i)
    {
        Ptr<LteUeRrc> ueRrc =  ueDevs.Get (i)->GetObject<LteUeNetDevice> ()->GetRrc ();
        ueRrc->TraceConnectWithoutContext ("ChangeOfSyncRef", MakeBoundCallback (&NotifyChangeOfSyncRef, streamSyncRef));
        ueRrc->TraceConnectWithoutContext ("SendSLSS", MakeBoundCallback (&NotifySendOfSlss, streamSendOfSlss));
    }
    

    InternetStackHelper internet;
    internet.Install (ueNodes);

    Ipv4Address groupAddress4 = Ipv4Address ("255.255.255.255");
    Time startTime = Seconds (1);
    Time stopTime =  Seconds (20);
    McpttHelper mcpttHelper;
    ApplicationContainer mcpttclientApps;
    McpttTimer mcpttTimer;

    double pushTimeMean = 5.0; // seconds
    double pushTimeVariance = 2.0; // seconds
    double releaseTimeMean = 5.0; // seconds
    double releaseTimeVariance = 2.0; // seconds
    TypeId socketFacTid = UdpSocketFactory::GetTypeId ();
    DataRate dataRate = DataRate ("24kb/s");
    Ipv6Address PeerAddress = Ipv6Address ("6001:db80::");

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

    mcpttclientApps.Add (mcpttHelper.Install (ueNodes));
    mcpttclientApps.Start (startTime);
    mcpttclientApps.Stop (stopTime);


    ObjectFactory callFac;
    callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");

    ObjectFactory floorFac;
    floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");
    
    Ipv4AddressValue grpAddr;

    Ptr<McpttPttApp> ueAPttApp = DynamicCast<McpttPttApp, Application> (mcpttclientApps.Get (0));

    //floor and call machines generate*******************************
    ueAPttApp->CreateCall (callFac, floorFac);
    ueAPttApp->SelectLastCall ();

    Simulator::Schedule (Seconds (2.2), &McpttPttApp::TakePushNotification, ueAPttApp);

    std::ostringstream txWithAddresses;
    txWithAddresses << "/NodeList/" << ueDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/TxWithAddresses";


    std::ostringstream rxWithAddresses;
    rxWithAddresses << "/NodeList/" << ueDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/RxWithAddresses";


 
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("UePacketTrace.tr");

    //Trace file table header
    *stream->GetStream () << "time(sec)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;
    std::ostringstream oss;
    // Set Tx traces
    // for (uint16_t ac = 0; ac < mcpttclientApps.GetN (); ac++)
    // {
    //     Ipv4Address localAddrs =  mcpttclientApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
    //     std::cout << "Tx address: " << localAddrs << " Node Id " << mcpttclientApps.Get (ac)->GetNode ()->GetId () << std::endl;
    //     oss << "tx\t" << mcpttclientApps.Get (ac)->GetNode ()->GetId () << "\t" << mcpttclientApps.Get (ac)->GetNode ()->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
    //     mcpttclientApps.Get (ac)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
    //     oss.str ("");
    // }




    // configure all the simulation scenario here...
    lteHelper->EnablePhyTraces ();
    lteHelper->EnableMacTraces ();
    lteHelper->EnableRlcTraces ();
    lteHelper->EnablePdcpTraces ();

    lteHelper->EnableSlPscchMacTraces ();
    lteHelper->EnableSlPsschMacTraces ();

    lteHelper->EnableSlRxPhyTraces ();
    lteHelper->EnableSlPscchRxPhyTraces ();

    mcpttHelper.EnableMsgTraces ();
    mcpttHelper.EnableStateMachineTraces ();

    Simulator::Stop (stopTime);
    
    AnimationInterface anim("b_sl_4.xml");
    anim.SetMaxPktsPerTraceFile(500000);

    Simulator::Run ();
  
    Simulator::Destroy ();
    std::cout << "done" << std::endl;
    return 0;


}