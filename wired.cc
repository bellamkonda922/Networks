#include <string>
#include <vector>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("WiredConnection");

int main(int argc , char **argv){

    LogComponentEnable("WiredConnection",LOG_INFO);

    vector<int>packsizes = {40, 44, 48, 52, 60,552, 576, 628, 1420 ,1500};

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
    string tracefilename = "WiredTCP-"+agent+".txt";
    Ptr<OutputStreamWrapper> stream = helper.CreateFileStream(tracefilename);
    *stream->GetStream() << "TCP Agent used : TCP_"<<agent<<endl<<"Size of Packet \tThroughput  \t\t Jain's Fairness Index"<<endl;

int maxPacketsInQueue;
    for(int sz = 0;sz<(int)packsizes.size();sz++){
        int packetsize = packsizes[sz];
        Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetsize));
        
        NodeContainer nodes;
        nodes.Create(4);

        PointToPointHelper  hr;
        hr.SetDeviceAttribute("DataRate" , StringValue("100Mbps"));
        hr.SetChannelAttribute("Delay",StringValue("20ms"));
        maxPacketsInQueue = (100*20*1000)/(8*packetsize);
      	hr.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(to_string(maxPacketsInQueue)+"p")); 
        
        PointToPointHelper rr;
        rr.SetDeviceAttribute("DataRate" , StringValue("10Mbps"));
        rr.SetChannelAttribute("Delay",StringValue("50ms"));
        maxPacketsInQueue = (10*50*1000)/(8*packetsize);
      	rr.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(to_string(maxPacketsInQueue)+"p")); 
        
        vector<NetDeviceContainer>netdevices(3);
        for(int i=0;i<3;i++){
            if(i==1){
                netdevices[i] = rr.Install(nodes.Get(i),nodes.Get(i+1));
            }
            else{
            netdevices[i] = hr.Install(nodes.Get(i),nodes.Get(i+1));
            }
        }
        
        InternetStackHelper internet;
        internet.Install(nodes);

        vector<Ipv4Address> addr = {"10.1.1.0","10.1.2.0","10.1.3.0"};
        Ipv4AddressHelper ipv4;
        vector<Ipv4InterfaceContainer>interfaces(3);
        for(int i=0;i<3;i++){
            ipv4.SetBase(addr[i],"255.255.255.0");
            interfaces[i] = ipv4.Assign(netdevices[i]);
        }

        ApplicationContainer sourceApp,sinkApp;

        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

        uint16_t port = 10023 + sz;

        BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (interfaces[2].GetAddress (1), port));
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