#include "include/ifaceinformation.h"

#include <cstring>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>

#include <ifaddrs.h>
#include <sys/types.h>

#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <sys/socket.h>

#include <limits>

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

    void IfaceHelper::getInterfacesList(InterfacesList &iface_list) noexcept
    {
        if (information_.empty())
        {
            return;
        }

        InterfacesList lst;
        for (const auto element : information_)
        {
            iface_list.push_back(element.second);
        }
        iface_list.shrink_to_fit();
    }

    void IfaceHelper::update() noexcept
    {
        update_information();
    }

    IfaceHelper::~IfaceHelper() {}

    void IfaceHelper::update_information()
    {
        struct ifconf ifc;
        struct ifreq  ifr[10];
        int           sd { -1 };

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
                    InterfacePtr nmInfo;

                    if (auto res = information_.find(ifr[i].ifr_name); res != information_.end())
                    {
                        nmInfo = res->second;
                    }
                    else
                    {
                        nmInfo = std::make_shared< Interface >();
                    }

                    nmInfo->interface  = ifr[i].ifr_name;
                    nmInfo->isUp       = is_interface_online(ifr[i].ifr_name);
                    nmInfo->isLoopBack = is_interface_loopBack(ifr[i].ifr_name);

                    if (!update_ip(sd, &ifr[i], nmInfo))
                    {
                        print_error_msg("err update ip");
                    }
                    if (!update_broadcast(sd, &ifr[i], nmInfo))
                    {
                        print_error_msg("err update ip");
                    }
                    if (!update_mac(sd, &ifr[i], nmInfo))
                    {
                        print_error_msg("err update ip");
                    }
                    if (!update_net_mask(sd, &ifr[i], nmInfo))
                    {
                        print_error_msg("err update ip");
                    }
                    if (!update_mtu(sd, &ifr[i], nmInfo))
                    {
                        print_error_msg("err update ip");
                    }
                    if (!update_network(sd, &ifr[i], nmInfo))
                    {
                        print_error_msg("err update ip");
                    }
                    updateIpV6(nmInfo);
                    nmInfo->ip.shrink_to_fit();
                    information_.insert({ nmInfo->interface, nmInfo });
                }
            }
            else
            {
                print_error_msg("ioctl error: ");
            }
        }

        else
        {
            print_error_msg("Can't open socket");
        }
    }

    bool IfaceHelper::update_ip(int sd, ifreq *ifr, InterfacePtr ni)
    {
        if (ifr != nullptr && !ni->interface.empty())
        {
            auto ip_s = std::make_shared< Ip_s >();
            switch (ifr->ifr_addr.sa_family)
            {
            case AF_INET:
                ip_s->version  = ip_version::ip_v4;
                ip_s->ip.at(0) = (( struct sockaddr_in * )&(ifr->ifr_addr))->sin_addr.s_addr;
                break;
            case AF_INET6:
                {
                    ip_s->version    = ip_version::ip_v6;
                    auto ipv6_struct = (( struct sockaddr_in6 * )&(ifr->ifr_addr))->sin6_addr;
                    for (size_t i = 0; i < 4; i++)  // 4 by uint32_t
                    {
                        ip_s->ip.at(i) = ipv6_struct.__in6_u.__u6_addr32[i];
                    }
                }
                break;
            default:
                return false;
            }

            ni->ip.push_back(ip_s);
            return true;
        }
        else
        {
            print_error_msg("Can't create io request to update ip address: ");
            return false;
        }
    }

    bool IfaceHelper::update_broadcast(int sd, ifreq *ifr, InterfacePtr ni)
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

    bool IfaceHelper::update_mac(int sd, ifreq *ifr, InterfacePtr ni)
    {
        if (ioctl(sd, SIOCGIFHWADDR, ifr) == 0)
        {
            unsigned char mac_address[maxLenMacAddr_];
            memcpy(mac_address, ifr->ifr_hwaddr.sa_data, maxLenMacAddr_);
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

    bool IfaceHelper::update_net_mask(int sd, ifreq *ifr, InterfacePtr ni)
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

    bool IfaceHelper::update_mtu(int sd, ifreq *ifr, InterfacePtr ni)
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

    bool IfaceHelper::update_network(int sd, ifreq *ifr, InterfacePtr ni)
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

    bool IfaceHelper::updateIpV6(InterfacePtr ni)
    {
        struct ifaddrs     *ifap, *ifa;
        struct sockaddr_in *sa;
        char               *addr;
        getifaddrs(&ifap);
        for (ifa = ifap; ifa; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_name == ni->interface && ifa->ifa_addr->sa_family == AF_INET6)
            {
                auto ip_s      = std::make_shared< Ip_s >();
                ip_s->version  = ip_version::ip_v6;
                auto val       = (( struct sockaddr_in6       *)(ifa->ifa_addr))->sin6_addr;
                auto val2      = val.s6_addr32;
                ip_s->ip.at(0) = val2[0];
                ip_s->ip.at(1) = val2[1];
                ip_s->ip.at(2) = val2[2];
                ip_s->ip.at(3) = val2[3];
                ni->ip.push_back(ip_s);
            }
            else
            {
                continue;
            }
        }
        freeifaddrs(ifap);
        return true;
    }

//    void IfaceHelper::update_information2()
//    {
//        struct ifaddrs     *ifap, *ifa;
//        struct sockaddr_in *sa;
//        char               *addr;
//        getifaddrs(&ifap);

//        for (ifa = ifap; ifa; ifa = ifa->ifa_next)
//        {
//            InterfacePtr nmInfo;
//            if (auto res = information_.find(ifa->ifa_name); res != information_.end())
//            {
//                nmInfo = res->second;
//            }
//            else
//            {
//                nmInfo            = std::make_shared< Interface >();
//                nmInfo->interface = ifa->ifa_name;
//            }
//            update_ip(ifa, nmInfo);
//            update_mac(ifa, nmInfo);
//            update_broadcast(ifa, nmInfo);
//        }

//        freeifaddrs(ifap);
//    }

//    bool IfaceHelper::update_ip(ifaddrs *addr_struct, InterfacePtr iface_ptr)
//    {
//        if (!addr_struct || !iface_ptr)
//        {
//            return false;
//        }

//        auto ip_s = std::make_shared< Ip_s >();
//        switch (addr_struct->ifa_addr->sa_family)
//        {
//        case AF_INET:
//            {
//                ip_s->version  = ip_version::ip_v4;
//                auto val       = ( struct sockaddr_in       *)(addr_struct->ifa_addr);
//                ip_s->ip.at(0) = val->sin_addr.s_addr;
//            }
//            break;
//        case AF_INET6:
//            {
//                ip_s->version  = ip_version::ip_v6;
//                auto val       = (( struct sockaddr_in6       *)(addr_struct->ifa_addr))->sin6_addr;
//                auto val2      = val.s6_addr32;
//                ip_s->ip.at(0) = val2[0];
//                ip_s->ip.at(1) = val2[1];
//                ip_s->ip.at(2) = val2[2];
//                ip_s->ip.at(3) = val2[3];
//            }
//            break;
//        default:
//            return true;
//        }

//        iface_ptr->ip.push_back(ip_s);
//        return true;
//    }

//    bool IfaceHelper::update_mac(ifaddrs *addr_struct, InterfacePtr iface_ptr)
//    {
//        if (!addr_struct || !iface_ptr)
//        {
//            return false;
//        }

//        if (addr_struct->ifa_addr->sa_family == AF_PACKET)
//        {
//            struct sockaddr_ll *s = ( struct sockaddr_ll * )addr_struct->ifa_addr;

//            char macStr[18];
//            snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", s->sll_addr[0], s->sll_addr[1], s->sll_addr[2], s->sll_addr[3],
//                     s->sll_addr[4], s->sll_addr[5]);
//            iface_ptr->macAddress = macStr;
//        }

//        return true;
//    }

//    bool IfaceHelper::update_broadcast(ifaddrs *addr_struct, InterfacePtr iface_ptr)
//    {
//        if (!addr_struct || !iface_ptr)
//        {
//            return false;
//        }
//        //        auto bcast    = (( struct sockaddr_in6    *)(addr_struct->ifa_addr))->sin6_addr.__in6_u.__u6_addr32;
//        auto bcast2 = addr_struct->ifa_ifu.ifu_broadaddr->sa_data;
//        //        iface_ptr->broadcast = ipToStr();
//        std::cout << "BCast: " << iface_ptr->broadcast << std::endl;
//    }

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
}  // namespace iface_lib
