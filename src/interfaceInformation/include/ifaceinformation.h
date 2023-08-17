#ifndef NETWORKINFORMER_H
#define NETWORKINFORMER_H
#include "struct_definition.h"
#include "version.h"
#include <bitset>
#include <map>

struct ifreq;

namespace iface_helper
{
    class UnixNetworkInformer;

    class NetworkInformer
    {
      public:
        NetworkInformer() noexcept;
        InterfacesList getInterfacesList() noexcept;                             ///< Возвращает список интерфейсов
        NetInfoPtr     getInterfaceInfo(const std::string& ifaceName) noexcept;  ///< Возвращает конкретный интерфейс
        void           update() noexcept;                                        ///< Обновляет информацию
        ~NetworkInformer();

      private:
        NetInfoPtr parseReq(const ifreq& ifr, int sd) noexcept;

        void        update_information();
        bool        update_ip(int sd, ifreq* ifr, NetInfoPtr);
        bool        update_broadcast(int sd, ifreq* ifr, NetInfoPtr ni);
        bool        update_mac(int sd, ifreq* ifr, NetInfoPtr ni);
        bool        update_net_mask(int sd, ifreq* ifr, NetInfoPtr ni);
        bool        update_mtu(int sd, ifreq* ifr, NetInfoPtr ni);
        bool        update_network(int sd, ifreq* ifr, NetInfoPtr ni);

        std::string ipToStr(int ip);

      private:
        std::map< std::string, NetInfoPtr > m_information;
    };

}  // namespace iface_helper
#endif  // NETWORKINFORMER_H
