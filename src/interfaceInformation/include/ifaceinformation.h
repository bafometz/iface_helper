#ifndef NETWORKINFORMER_H
#define NETWORKINFORMER_H
#include "struct_definition.h"
#include "version.h"
#include <map>

struct ifreq;

// https://stackoverflow.com/a/4139811
namespace iface_lib
{

    class IfaceHelper
    {
      public:
        IfaceHelper() noexcept;
        void       getInterfacesList(InterfacesList& iface_list) noexcept;    ///< Возвращает список интерфейсов
        InterfacePtr getInterfaceInfo(const std::string& iface_name) noexcept;  ///< Возвращает конкретный интерфейс
        void       update() noexcept;                                         ///< Обновляет информацию
        ~IfaceHelper();

      private:
        InterfacePtr parseReq(const ifreq& ifr, int sd) noexcept;

        void update_information();
        bool update_ip(int sd, ifreq* ifr, InterfacePtr);
        bool update_broadcast(int sd, ifreq* ifr, InterfacePtr ni);
        bool update_mac(int sd, ifreq* ifr, InterfacePtr ni);
        bool update_net_mask(int sd, ifreq* ifr, InterfacePtr ni);
        bool update_mtu(int sd, ifreq* ifr, InterfacePtr ni);
        bool update_network(int sd, ifreq* ifr, InterfacePtr ni);

        std::string ipToStr(int ip){return  "";};

      private:
        std::map< std::string, InterfacePtr > information_;
        const uint8_t                       maxLenMacAddr_ = 6;
    };

}  // namespace iface_lib
#endif  // NETWORKINFORMER_H
