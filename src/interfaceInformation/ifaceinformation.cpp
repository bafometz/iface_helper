#include "include/ifaceinformation.h"

#include <cstring>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>

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

    IfaceHelper::~IfaceHelper() {}

    void IfaceHelper::update_information()
    {
        struct ifconf ifc;
        struct ifreq  ifr[10];
        int           sd { -1 };

        if (!information_.empty())
        {
            information_.clear();
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
//                    if (ifr[i].ifr_addr.sa_family != AF_INET) continue;
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

                    if (!update_ip(sd, &ifr[i], nmInfo)) continue;
                    if (!update_broadcast(sd, &ifr[i], nmInfo)) continue;
                    if (!update_mac(sd, &ifr[i], nmInfo)) continue;
                    if (!update_net_mask(sd, &ifr[i], nmInfo)) continue;
                    if (!update_mtu(sd, &ifr[i], nmInfo)) continue;
                    if (!update_network(sd, &ifr[i], nmInfo)) continue;
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
            std::cerr << "Can't open socket" << std::endl;
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
                ip_s->version = ip_version::ip_v4;
                ip_s->ip  = (( struct sockaddr_in  *)&(ifr->ifr_addr))->sin_addr.s_addr;
                break;
            case AF_INET6:
//                ip_s->version = ip_version::ip_v6;
//                auto ipv6_struct = (( struct sockaddr_in6  *)&(ifr->ifr_addr))->sin6_addr;
//                ip_s->ip = ipv6_struct.__in6_u.__u6_addr32;
                return false;
                break;
            }

            ni->ip.push_back(ip_s);
            return true;
        }
        else
        {
            std::cerr << "Can't create io request to update ip address: " << strerror(errno) << std::endl;
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
}  // namespace iface_lib

// void UnixNetworkInformer::updateInfo()
//{
//     //    Взято:
//     //
