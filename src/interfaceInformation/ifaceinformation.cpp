#include "include/ifaceinformation.h"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <map>
#include <sstream>

#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define maxLenMacAddr 6

class UnixNetworkInformer
{
   public:
   UnixNetworkInformer ();
   ~UnixNetworkInformer ();

   std::string        getIp ( std::string interfaceName ) const;
   std::string        getMask ( std::string interfaceName ) const;
   InterfacesList     getInterfacesList () const;
   NetInfoPtr         getInterfaceInfo ( std::string interfaceName ) const;
   void               updateInfo ();
   static std::string intToAddress ( int _addr );

   private:
   bool is_interface_online ( std::string interface );
   bool is_interface_loopBack ( std::string interface );

   private:
   std::map<std::string, NetInfoPtr> m_information;
};

UnixNetworkInformer::UnixNetworkInformer ()
{
   updateInfo ();
}

UnixNetworkInformer::~UnixNetworkInformer ()
{
   if ( !m_information.empty () ) { m_information.clear (); }
}

std::string
UnixNetworkInformer::getIp ( std::string interfaceName ) const
{
   if ( m_information.empty () ) { return ""; }

   return m_information.find ( interfaceName ) != m_information.end () ? m_information.at ( interfaceName )->ip_v4 : "";
}

std::string
UnixNetworkInformer::getMask ( std::string interfaceName ) const
{
   if ( m_information.empty () ) { return ""; }
   return ( m_information.find ( interfaceName ) != m_information.end () ) ? m_information.at ( interfaceName )->netMask : "";
}

InterfacesList
UnixNetworkInformer::getInterfacesList () const
{
   InterfacesList list;

   for ( auto &element : m_information )
   {
      list.push_back ( element.second );
   }
   list.shrink_to_fit ();
   return list;
}

NetInfoPtr
UnixNetworkInformer::getInterfaceInfo ( std::string interfaceName ) const
{
   if ( m_information.empty () ) { return nullptr; }

   return ( m_information.find ( interfaceName ) != m_information.end () ) ? m_information.at ( interfaceName ) : nullptr;
}

void
UnixNetworkInformer::updateInfo ()
{
   //    Взято:
   //    https://stackoverflow.com/a/4139811
   struct ifconf ifc;
   struct ifreq  ifr[10];
   int           sd;

   if ( !m_information.empty () ) { m_information.clear (); }

   sd = socket ( PF_INET, SOCK_DGRAM, 0 );

   if ( sd > 0 )
   {
      ifc.ifc_len           = sizeof ( ifr );
      ifc.ifc_ifcu.ifcu_buf = (caddr_t)ifr;
      if ( ioctl ( sd, SIOCGIFCONF, &ifc ) == 0 )
      {
         int ifc_num { 0 }, addr { 0 }, bcast { 0 }, mask { 0 }, network { 0 }, i { 0 };
         ifc_num = ifc.ifc_len / sizeof ( struct ifreq );

         for ( i = 0; i < ifc_num; ++i )
         {
            NetInfoPtr nmInfo = std::make_shared<NetInfo> ();

            if ( ifr[i].ifr_addr.sa_family != AF_INET ) { continue; }

            /* display the interface name */
            //                printf("%d) interface: %s\n", i+1, ifr[i].ifr_name);
            nmInfo->interface  = ifr[i].ifr_name;
            nmInfo->isUp       = is_interface_online ( ifr[i].ifr_name );
            nmInfo->isLoopBack = is_interface_loopBack ( ifr[i].ifr_name );

            /* Retrieve the IP address, broadcast address, and subnet mask. */
            if ( ioctl ( sd, SIOCGIFADDR, &ifr[i] ) == 0 )
            {
               addr                = ( (struct sockaddr_in *)( &ifr[i].ifr_addr ) )->sin_addr.s_addr;
               IpPtr ipptr         = std::make_shared<Ip_s> ();
               ipptr->ip           = intToAddress ( addr );
               ipptr->ipVersion[0] = 1;// IpV4
               nmInfo->ip_v4       = ipptr->ip;
               nmInfo->ip.push_back ( ipptr );
               //                    printf("%d) address: %d.%d.%d.%d\n", i+1, intToAddress(addr));
            }

            if ( ioctl ( sd, SIOCGIFBRDADDR, &ifr[i] ) == 0 )
            {
               bcast             = ( (struct sockaddr_in *)( &ifr[i].ifr_broadaddr ) )->sin_addr.s_addr;
               nmInfo->broadcast = intToAddress ( bcast );
               //                    printf("%d) broadcast: %d.%d.%d.%d\n", i+1, intToAddress(bcast));
            }

            if ( ioctl ( sd, SIOCGIFHWADDR, &ifr[i] ) == 0 )
            {
               unsigned char mac_address[maxLenMacAddr];

               memcpy ( mac_address, ifr[i].ifr_hwaddr.sa_data, maxLenMacAddr );
               std::stringstream ss;

               for ( int p = 0; p < maxLenMacAddr; p++ )
               {
                  ss << std::hex << std::setw ( 2 ) << std::setfill ( '0' ) << std::uppercase << (uint)mac_address[p] << ":";
               }

               nmInfo->macAddress = ss.str ();
               nmInfo->macAddress.pop_back ();//Удаляем последнее :
            }

            if ( ioctl ( sd, SIOCGIFNETMASK, &ifr[i] ) == 0 )
            {
               mask            = ( (struct sockaddr_in *)( &ifr[i].ifr_netmask ) )->sin_addr.s_addr;
               nmInfo->netMask = intToAddress ( mask );
               //                    printf("%d) netmask: %d.%d.%d.%d\n", i+1, intToAddress(mask));
            }

            if ( ioctl ( sd, SIOCGIFMTU, &ifr[i] ) == 0 ) { nmInfo->mtu = ifr->ifr_mtu; }

            /* Compute the current network value from the address and netmask. */
            network         = addr & mask;
            nmInfo->network = intToAddress ( network );
            //                printf("%d) network: %d.%d.%d.%d\n", i+1, intToAddress(network));
            nmInfo->ip.shrink_to_fit ();
            m_information.insert ( { nmInfo->interface, nmInfo } );
         }
      }
      close ( sd );
   }
}

std::string
UnixNetworkInformer::intToAddress ( int _addr )
{
   std::string str;

   str.append ( std::to_string ( _addr & 0xFF ) );
   str.push_back ( '.' );
   str.append ( std::to_string ( _addr >> 8 & 0xFF ) );
   str.push_back ( '.' );
   str.append ( std::to_string ( _addr >> 16 & 0xFF ) );
   str.push_back ( '.' );
   str.append ( std::to_string ( _addr >> 24 & 0xFF ) );

   return str;
}

bool
UnixNetworkInformer::is_interface_online ( std::string interface )
{
   if ( interface.length () > static_cast<size_t> ( IF_NAMESIZE ) ) return false;

   struct ifreq interfaceRequest;
   int          sock = socket ( PF_INET6, SOCK_DGRAM, IPPROTO_IP );
   std::memset ( &interfaceRequest, 0, sizeof ( interfaceRequest ) );
   std::strncpy ( interfaceRequest.ifr_name, interface.c_str (),  IFNAMSIZ );

   if ( ioctl ( sock, SIOCGIFFLAGS, &interfaceRequest ) < 0 ) { perror ( "SIOCGIFFLAGS" ); }

   close ( sock );
   return static_cast<bool> ( interfaceRequest.ifr_flags & IFF_UP );
}

bool
UnixNetworkInformer::is_interface_loopBack ( std::string interface )
{
   if ( interface.length () > static_cast<size_t> ( IF_NAMESIZE ) ) return false;
   struct ifreq ifr;
   int          sock = socket ( PF_INET6, SOCK_DGRAM, IPPROTO_IP );
   std::memset ( &ifr, 0, sizeof ( ifr ) );
   std::strncpy ( ifr.ifr_name, interface.c_str (), IF_NAMESIZE );

   if ( ioctl ( sock, SIOCGIFFLAGS, &ifr ) < 0 ) { perror ( "SIOCGIFFLAGS" ); }

   close ( sock );
   return static_cast<bool> ( ifr.ifr_flags & IFF_LOOPBACK );
}

NetworkInformer::NetworkInformer ()
    : m_networkInformer ( std::make_unique<UnixNetworkInformer> () )
{
   update ();
}

InterfacesList
NetworkInformer::getInterfacesList ()
{
   return m_networkInformer->getInterfacesList ();
}

NetInfoPtr
NetworkInformer::getInterfaceInfo ( const std::string &interfaceName )
{
   return m_networkInformer->getInterfaceInfo ( interfaceName );
}

void
NetworkInformer::update ()
{
   m_networkInformer->updateInfo ();
}

NetworkInformer::~NetworkInformer () {}
