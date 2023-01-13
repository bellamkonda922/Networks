#include <string>
#include<vector>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"


using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("WirelessConnection");

int main(int argc , char **argv){

    LogComponentEnable("WirelessConnection",LOG_INFO);

    vector<int>packsizes = {40, 44, 48, 52, 60, 552, 576, 628, 1420 ,1500};

    string agent;

    CommandLine terminal;
    terminal.AddValue("agent","please mention the tcp agent type u want to use, available agents are Westwood,Veno,Vegas",agent);
    terminal.Parse(argc,argv);

    if(agent == "Vegas"){
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpVegas"));
    }
    else if(agent == "Westwood"){
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpWestwood"));
    }
    else if(agent == "Veno"){
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpVeno"));
    }
    else{
        NS_LOG_INFO("Kindly enter a correct agent as specified");
        exit(0);
    }
    NS_LOG_INFO("Using the TCP AGENT : "+agent);


    AsciiTraceHelper helper;
    string tracefilename = "WirelessTCP-"+agent+".txt";
    Ptr<OutputStreamWrapper> stream = helper.CreateFileStream(tracefilename);
    *stream->GetStream() << "TCP Agent used : TCP_"<<agent<<endl<<"Size of Packet \tThroughput  \t\t Jain's Fairness Index"<<endl;

    Config::SetDefault ("ns3::WifiMacQueue::DropPolicy", EnumValue(WifiMacQueue::DROP_NEWEST));

    for(int sz = 0;sz<(int)packsizes.size();sz++){
        int packetsize = packsizes[sz];
        Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetsize));
        
        NodeContainer nodes;
        nodes.Create(4);
        vector<NodeContainer>apnodes(2),stanodes(2);
        for(int i=0;i<2;i++){
            apnodes[i] = nodes.Get(i+1);
            stanodes[i] = nodes.Get(i*3);
        }
       
        vector<YansWifiChannelHelper> channels(2);
        vector<YansWifiPhyHelper>phys(2);
       
        for(int i=0;i<2;i++){
            channels[i]=YansWifiChannelHelper::Default();
            phys[i].SetChannel(channels[i].Create());
        }
        
        WifiHelper wifi;
        wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
        
        WifiMacHelper mac;
        Ssid ssid = Ssid ("ns-3-ssid");
  		mac.SetType ( "ns3::StaWifiMac" , "Ssid" , SsidValue (ssid), "ActiveProbing" , BooleanValue (false) ) ;
      
        vector<NetDeviceContainer>stadevices(2);

        for(int i=0;i<2;i++){
            stadevices[i] = wifi.Install(phys[i],mac,stanodes[i]);
        }
        
        mac.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssid) );

        vector<NetDeviceContainer>apdevices(2);
        for(int i=0;i<2;i++){
            apdevices[i] = wifi.Install(phys[i],mac,apnodes[i]);
        }
       
        MobilityHelper mobile;
        
        Ptr<ListPositionAllocator> locationvec = CreateObject<ListPositionAllocator>();
        
	    locationvec-> Add(Vector(000.0, 0.0, 0.0));
	    locationvec-> Add(Vector(050.0, 0.0, 0.0));
	    locationvec-> Add(Vector(100.0, 0.0, 0.0));
	    locationvec-> Add(Vector(150.0, 0.0, 0.0));
        

        mobile.SetPositionAllocator(locationvec);
	    mobile.SetMobilityModel("ns3::ConstantPositionMobilityModel");

        
        for(int i=0;i<4;i++){
            mobile.Install(nodes.Get(i));
        }
        
        PointToPointHelper btb;
        btb.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
        btb.SetChannelAttribute("Delay",StringValue("100ms"));
        int maxPacketsInQueue = (10*100*1000)/(8*packetsize);
        btb.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(to_string(maxPacketsInQueue) + "p"));
        
        NetDeviceContainer basestation;
        basestation = btb.Install(nodes.Get(1),nodes.Get(2));
        
        InternetStackHelper internet;
        internet.Install(nodes);
        
        Ipv4AddressHelper ipv4;
        vector<Ipv4InterfaceContainer>interfaces(5);
        ipv4.SetBase( "10.1.1.0" , "255.255.255.0" );
	  	interfaces[0] = ipv4.Assign ( apdevices[0] );
	  	interfaces[2] = ipv4.Assign ( stadevices[0] );
	  	
  		ipv4.SetBase( "10.1.2.0" , "255.255.255.0" );
	  	 interfaces[1] = ipv4.Assign ( apdevices[1] );
	  	 interfaces[3] = ipv4.Assign ( stadevices[1] );
        
  		ipv4.SetBase( "10.1.3.0" , "255.255.255.0" );
	  	 interfaces[4] = ipv4.Assign ( basestation );
            
        ApplicationContainer sourceApp,sinkApp;

        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

        uint16_t port = 10023 + sz;

        BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (interfaces[3].GetAddress(0), port));
      	source.SetAttribute("SendSize" , UintegerValue(packetsize));
      	source.SetAttribute("MaxBytes" , UintegerValue(0));
      	sourceApp= (source.Install( nodes.Get(0) ) );
        
        PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
		sinkApp= (sink.Install(nodes.Get(3)));

        sinkApp.Start(Seconds(0));
		sinkApp.Stop(Seconds(5));
		sourceApp.Start (Seconds (0));
	  	sourceApp.Stop (Seconds (5));
        
        FlowMonitorHelper moni;
        Ptr<FlowMonitor> monitor = moni.InstallAll();
        
        Simulator::Stop (Seconds (5));
  		Simulator::Run ();

        Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (moni.GetClassifier());
  		FlowMonitor::FlowStatsContainer statistics = monitor->GetFlowStats ();
  		std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = statistics.begin();
        
        double timetaken = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
        double throughput = (((i->second.rxBytes*8)/timetaken)/1000); // throughput in kbps
        double fairnessindex = (throughput*throughput)/(throughput*throughput);
       
        *stream->GetStream() << to_string(packetsize) <<"\t\t"<<to_string(throughput)<<"\t\t"<<fairnessindex<<endl;
        cout << to_string(packetsize)<<"\t\t"<<throughput<<"\t\t"<<fairnessindex<<endl;


    Simulator::Destroy();

    }
    return 0;
    
}