#include "include/ifaceinformation.h"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define maxLenMacAddr 6

///< @todo Добавить структуру для работы с сокетами в RAII
namespace iface_lib
{
    static void print_error_msg(const std::string &msg)
    {
        std::cerr << msg << strerror(errno) << std::endl;
    }

    bool is_interface_online(std::string interface)
    {
        ///< @todo Добавить нормальный возврат ошибки в случае если не удалось открыть сокет
        if (interface.length() > static_cast< size_t >(IF_NAMESIZE)) return false;
        struct ifreq interfaceRequest;
        int          sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
        std::memset(&interfaceRequest, 0, sizeof(interfaceRequest));
        std::strncpy(interfaceRequest.ifr_name, interface.c_str(), IFNAMSIZ);

        if (ioctl(sock, SIOCGIFFLAGS, &interfaceRequest) < 0)
        {
            perror("SIOCGIFFLAGS");
        }

        close(sock);
        return static_cast< bool >(interfaceRequest.ifr_flags & IFF_UP);
    }

    bool is_interface_loopBack(std::string interface)
    {
        ///< @todo Добавить нормальный возврат ошибки в случае если не удалось открыть сокет
        if (interface.length() > static_cast< size_t >(IF_NAMESIZE)) return false;
        struct ifreq ifr;
        int          sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
        std::memset(&ifr, 0, sizeof(ifr));
        std::strncpy(ifr.ifr_name, interface.c_str(), IF_NAMESIZE);

        if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
        {
            perror("SIOCGIFFLAGS");
        }

        close(sock);
        return static_cast< bool >(ifr.ifr_flags & IFF_LOOPBACK);
    }

    IfaceHelper::IfaceHelper() noexcept
    {
        update_information();
    }

    InterfacesList IfaceHelper::getInterfacesList() noexcept
    {
        if (m_information.empty())
        {
            return {};
        }

        InterfacesList lst;
        for (const auto element : m_information)
        {
            lst.push_back(element.second);
        }
        return lst;
    }

    IfaceHelper::~IfaceHelper() {}

    void IfaceHelper::update_information()
    {
        struct ifconf ifc;
        struct ifreq  ifr[10];
        int           sd { -1 };

        if (!m_information.empty())
        {
            m_information.clear();
        }

        sd = socket(PF_INET, SOCK_DGRAM, 0);
        if (sd > 0)
        {
            ifc.ifc_len           = sizeof(ifr);
            ifc.ifc_ifcu.ifcu_buf = ( caddr_t )ifr;

            if (ioctl(sd, SIOCGIFCONF, &ifc) == 0)
            {
                int ifc_num { 0 };
                ifc_num = ifc.ifc_len / sizeof(struct ifreq);

                for (size_t i = 0; i < ifc_num; ++i)
                {
                    if (ifr[i].ifr_addr.sa_family != AF_INET) continue;
                    NetInfoPtr nmInfo = std::make_shared< NetInfo >();

                    nmInfo->interface  = ifr[i].ifr_name;
                    nmInfo->isUp       = is_interface_online(ifr[i].ifr_name);
                    nmInfo->isLoopBack = is_interface_loopBack(ifr[i].ifr_name);

                    if (!update_ip(sd, &ifr[i], nmInfo)) continue;
                    if (!update_broadcast(sd, &ifr[i], nmInfo)) continue;
                    if (!update_mac(sd, &ifr[i], nmInfo)) continue;
                    if (!update_net_mask(sd, &ifr[i], nmInfo)) continue;
                    if (!update_mtu(sd, &ifr[i], nmInfo)) continue;
                    if (!update_network(sd, &ifr[i], nmInfo)) continue;
                    nmInfo->ip.shrink_to_fit();
                    m_information.insert({ nmInfo->interface, nmInfo });
                }
            }
            else
            {
                print_error_msg("ioctl error: ");
            }
        }

        else
        {
            std::cerr << "Can't open socket" << std::endl;
        }
    }

    bool IfaceHelper::update_ip(int sd, ifreq *ifr, NetInfoPtr ni)
    {
        if (ioctl(sd, SIOCGIFADDR, ifr) == 0)
        {
            int   addr     = (( struct sockaddr_in       *)&(ifr->ifr_addr))->sin_addr.s_addr;
            IpPtr ipptr    = std::make_shared< Ip_s >();
            ipptr->ip      = ipToStr(addr);
            ipptr->version = ip_version::ip_v4;  // IpV4
            ni->ip.push_back(ipptr);
            return true;
        }
        else
        {
            std::cerr << "Can't create io request to update ip address: " << strerror(errno) << std::endl;
            return false;
        }
    }

    bool IfaceHelper::update_broadcast(int sd, ifreq *ifr, NetInfoPtr ni)
    {
        if (ioctl(sd, SIOCGIFBRDADDR, ifr) == 0)
        {
            auto bcast    = (( struct sockaddr_in    *)&(ifr->ifr_broadaddr))->sin_addr.s_addr;
            ni->broadcast = ipToStr(bcast);
            return true;
        }
        else
        {
            print_error_msg("Cant't get broadcast address");
            return false;
        }
    }

    bool IfaceHelper::update_mac(int sd, ifreq *ifr, NetInfoPtr ni)
    {
        if (ioctl(sd, SIOCGIFHWADDR, ifr) == 0)
        {
            unsigned char mac_address[maxLenMacAddr];
            memcpy(mac_address, ifr->ifr_hwaddr.sa_data, maxLenMacAddr);
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3],
                     mac_address[4], mac_address[5]);
            ni->macAddress = macStr;
            return true;
        }
        else
        {
            print_error_msg("Can't update mac address");
            return false;
        }
    }

    bool IfaceHelper::update_net_mask(int sd, ifreq *ifr, NetInfoPtr ni)
    {
        if (ioctl(sd, SIOCGIFNETMASK, ifr) == 0)
        {
            auto mask   = (( struct sockaddr_in   *)&(ifr->ifr_netmask))->sin_addr.s_addr;
            ni->netMask = ipToStr(mask);
            return true;
        }
        else
        {
            print_error_msg("Can't get mask");
            return false;
        }
    }

    bool IfaceHelper::update_mtu(int sd, ifreq *ifr, NetInfoPtr ni)
    {
        if (ioctl(sd, SIOCGIFMTU, ifr) == 0)
        {
            ni->mtu = ifr->ifr_mtu;
            return true;
        }
        else
        {
            print_error_msg("Can't udpate mtu");
            return false;
        }
    }

    bool IfaceHelper::update_network(int sd, ifreq *ifr, NetInfoPtr ni)
    {
        int mask { 0 };
        int addr { 0 };

        if (ioctl(sd, SIOCGIFNETMASK, ifr) == 0)
        {
            mask = (( struct sockaddr_in * )&(ifr->ifr_netmask))->sin_addr.s_addr;
        }
        else
        {
            return false;
        }

        if (ioctl(sd, SIOCGIFADDR, ifr) == 0)
        {
            addr = (( struct sockaddr_in * )(&ifr->ifr_addr))->sin_addr.s_addr;
        }
        else
        {
            return false;
        }

        auto network = addr & mask;
        ni->network  = ipToStr(network);
        return true;
    }

    std::string IfaceHelper::ipToStr(int ip)
    {
        std::string result;

        result.append(std::to_string(ip & 0xFF));
        result.push_back('.');
        result.append(std::to_string(ip >> 8 & 0xFF));
        result.push_back('.');
        result.append(std::to_string(ip >> 16 & 0xFF));
        result.push_back('.');
        result.append(std::to_string(ip >> 24 & 0xFF));

        return result;
    }

}  // namespace iface_helper

// void UnixNetworkInformer::updateInfo()
//{
//     //    Взято:
//     //    https://stackoverflow.com/a/4139811
//     struct ifconf ifc;
//     struct ifreq  ifr[10];
//     int           sd;

//    if (!m_information.empty())
//    {
//        m_information.clear();
//    }

//    sd = socket(PF_INET, SOCK_DGRAM, 0);

//    if (sd > 0)
//    {
//        ifc.ifc_len           = sizeof(ifr);
//        ifc.ifc_ifcu.ifcu_buf = ( caddr_t )ifr;
//        if (ioctl(sd, SIOCGIFCONF, &ifc) == 0)
//        {
//            int ifc_num { 0 }, addr { 0 }, bcast { 0 }, mask { 0 }, network { 0 }, i { 0 };
//            ifc_num = ifc.ifc_len / sizeof(struct ifreq);

//            for (i = 0; i < ifc_num; ++i)
//            {
//                NetInfoPtr nmInfo = std::make_shared< NetInfo >();

//                if (ifr[i].ifr_addr.sa_family != AF_INET)
//                {
//                    continue;
//                }

//                /* display the interface name */
//                //                printf("%d) interface: %s\n", i+1, ifr[i].ifr_name);
//                nmInfo->interface  = ifr[i].ifr_name;
//                nmInfo->isUp       = is_interface_online(ifr[i].ifr_name);
//                nmInfo->isLoopBack = is_interface_loopBack(ifr[i].ifr_name);

//                /* Retrieve the IP address, broadcast address, and subnet mask. */
//                if (ioctl(sd, SIOCGIFADDR, &ifr[i]) == 0)
//                {
//                    addr                = (( struct sockaddr_in                *)(&ifr[i].ifr_addr))->sin_addr.s_addr;
//                    IpPtr ipptr         = std::make_shared< Ip_s >();
//                    ipptr->ip           = intToAddress(addr);
//                    ipptr->ipVersion[0] = 1;  // IpV4
//                    nmInfo->ip_v4       = ipptr->ip;
//                    nmInfo->ip.push_back(ipptr);
//                    //                    printf("%d) address: %d.%d.%d.%d\n", i+1, intToAddress(addr));
//                }

//                if (ioctl(sd, SIOCGIFBRDADDR, &ifr[i]) == 0)
//                {
//                    bcast             = (( struct sockaddr_in             *)(&ifr[i].ifr_broadaddr))->sin_addr.s_addr;
//                    nmInfo->broadcast = intToAddress(bcast);
//                    //                    printf("%d) broadcast: %d.%d.%d.%d\n", i+1, intToAddress(bcast));
//                }

//                if (ioctl(sd, SIOCGIFHWADDR, &ifr[i]) == 0)
//                {
//                    unsigned char mac_address[maxLenMacAddr];

//                    memcpy(mac_address, ifr[i].ifr_hwaddr.sa_data, maxLenMacAddr);
//                    std::stringstream ss;

//                    for (int p = 0; p < maxLenMacAddr; p++)
//                    {
//                        ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << ( uint )mac_address[p] << ":";
//                    }

//                    nmInfo->macAddress = ss.str();
//                    nmInfo->macAddress.pop_back();  // Удаляем последнее :
//                }

//                if (ioctl(sd, SIOCGIFNETMASK, &ifr[i]) == 0)
//                {
//                    mask            = (( struct sockaddr_in            *)(&ifr[i].ifr_netmask))->sin_addr.s_addr;
//                    nmInfo->netMask = intToAddress(mask);
//                    //                    printf("%d) netmask: %d.%d.%d.%d\n", i+1, intToAddress(mask));
//                }

//                if (ioctl(sd, SIOCGIFMTU, &ifr[i]) == 0)
//                {
//                    nmInfo->mtu = ifr->ifr_mtu;
//                }

//                /* Compute the current network value from the address and netmask. */
//                network         = addr & mask;
//                nmInfo->network = intToAddress(network);
//                //                printf("%d) network: %d.%d.%d.%d\n", i+1, intToAddress(network));
//                nmInfo->ip.shrink_to_fit();
//                m_information.insert({ nmInfo->interface, nmInfo });
//            }
//        }
//        close(sd);
//    }
//}

// std::string UnixNetworkInformer::intToAddress(int _addr)
//{
//     std::string str;

//    str.append(std::to_string(_addr & 0xFF));
//    str.push_back('.');
//    str.append(std::to_string(_addr >> 8 & 0xFF));
//    str.push_back('.');
//    str.append(std::to_string(_addr >> 16 & 0xFF));
//    str.push_back('.');
//    str.append(std::to_string(_addr >> 24 & 0xFF));

//    return str;
//}

// bool UnixNetworkInformer::is_interface_online(std::string interface)
//{
//     if (interface.length() > static_cast< size_t >(IF_NAMESIZE)) return false;

//    struct ifreq interfaceRequest;
//    int          sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
//    std::memset(&interfaceRequest, 0, sizeof(interfaceRequest));
//    std::strncpy(interfaceRequest.ifr_name, interface.c_str(), IFNAMSIZ);

//    if (ioctl(sock, SIOCGIFFLAGS, &interfaceRequest) < 0)
//    {
//        perror("SIOCGIFFLAGS");
//    }

//    close(sock);
//    return static_cast< bool >(interfaceRequest.ifr_flags & IFF_UP);
//}

// bool UnixNetworkInformer::is_interface_loopBack(std::string interface)
//{
//     if (interface.length() > static_cast< size_t >(IF_NAMESIZE)) return false;
//     struct ifreq ifr;
//     int          sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
//     std::memset(&ifr, 0, sizeof(ifr));
//     std::strncpy(ifr.ifr_name, interface.c_str(), IF_NAMESIZE);

//    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
//    {
//        perror("SIOCGIFFLAGS");
//    }

//    close(sock);
//    return static_cast< bool >(ifr.ifr_flags & IFF_LOOPBACK);
//}

// NetworkInformer::NetworkInformer() :
//     m_networkInformer(std::make_unique< UnixNetworkInformer >())
//{
//     update();
// }

// InterfacesList NetworkInformer::getInterfacesList()
//{
//     return m_networkInformer->getInterfacesList();
// }

// NetInfoPtr NetworkInformer::getInterfaceInfo(const std::string &interfaceName)
//{
//     return m_networkInformer->getInterfaceInfo(interfaceName);
// }

// void NetworkInformer::update()
//{
//     m_networkInformer->updateInfo();
// }

// NetworkInformer::~NetworkInformer() {}
