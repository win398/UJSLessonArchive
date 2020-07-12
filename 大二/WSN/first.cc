/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

//递归加载库
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

//声明命名空间
using namespace ns3;

//日志记录模块
NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])    //main函数
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);    //最小时间间隔，默认为1ns
  //启用两个日志记录组件 构建客户机和回声回声服务器应用程序
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;    //模拟电脑对象结点，是一个拓扑辅助，因为互联网中的常常是点对点式的，所以用node
  nodes.Create (2);    //新建立2个节点指针

  /*
   *  此处的pointToPoint为点对点设备的连接，在现实世界中常为网线（双绞线等等设备），或者WiFi
   *  我们可以通过一个PointToPointHelper配置和连接 ns-3 PointToPointNetDevice 和 PointToPointChannel 
   */
  PointToPointHelper pointToPoint;
  //PointToPointHelper 对象使用值5 mbps DataRate创建一个 PointToPointNetDevice 对象
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  //设置传播延迟为2ms
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  /*
   * 使用NetDeviceContainer来保存创建的节点
   * 为每个节点 NodeContainer 创建PointToPointNetDevice 并保存在设备容器
   */
  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  /*
   * 安装拓扑辅助网络栈
   * 给上面创建的nodeContainer的每个节点安装包括TCP,UDP,IP的协议栈
   */
  InternetStackHelper stack;
  stack.Install (nodes);

  /* 声明可以分配的ipv4地址，是一个帮助器，地址由10.1.1.0开始递增（不能重复），子网掩码设置为255.255.255.0*/
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  //实际分配给设备ipv4地址，使用一个接口，同时记录的设备与ip的信息，像是路由表
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  //创建一个服务器应用程序，等待输入的UDP数据包，并将其发送回原始发件人
  UdpEchoServerHelper echoServer (9);
  
  //Install()方法执行，初始化回显服务器的应用，并将应用连接到一个节点上去;包含一个隐式转换；安装一个UdpEchoServerApplication
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0));   //应用在1s时生效
  serverApps.Stop (Seconds (10.0));   //10s时停止

  //创建一个UdpEchoClientHelper的对象，令其设置客户端的远端地址为服务器节点的IP地址。
  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  //告诉客户我们允许它发送的最大数目的包
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  //客户端数据包之间要等多长时间
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  //客户端大数据包的有效载荷
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  //如echo服务端一样，我们告诉客户端开始和停止时间
  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));   //时间为2s的时候开始
  clientApps.Stop (Seconds (10.0));   //10s时结束
  
  /*
   * 当Simulator::Run被调用时，系统会开始遍历预设事件的列表并执行。
   * 在1s时会使echo服务端应用生效
   * 在2s时让echo客户端应用开始
   */
  Simulator::Run ();
  //因为这个程序只发送一个数据包，所以执行完毕后会全局调用模拟器销毁
  Simulator::Destroy ();
  return 0;
}
