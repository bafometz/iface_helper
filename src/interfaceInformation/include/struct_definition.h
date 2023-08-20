#ifndef STRUCT_DEFINITION_H
#define STRUCT_DEFINITION_H
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace iface_lib
{
    enum class ip_version
    {
        empty = -1,
        ip_v4 = 0,
        ip_v6
    };

    struct Ip_s
    {
        std::string ip { "" };  ///< Человекочитаемый ip
        u_int64_t   ipInt { 0 };
        ip_version  version { ip_version::empty };

        bool isV4() { return version == ip_version::ip_v4; };

        bool isV6() { return version == ip_version::ip_v6; };

        std::string versionStr()
        {
            switch (version)
            {
            case iface_lib::ip_version::empty:
                return "";
            case iface_lib::ip_version::ip_v4:
                return "IpV4";
            case iface_lib::ip_version::ip_v6:
                return "IpV6";
            default:
                return "";
            }
            return "";
        };
    };

    using IpPtr = std::shared_ptr< Ip_s >;

    struct Interface
    {
        std::string          interface = "";      ///< Имя интерфейса
        std::vector< IpPtr > ip;                  ///< Список ip адресов
        std::string          netMask    = "";     ///< Маска интерфейса
        std::string          broadcast  = "";     ///< Широковещательный адрес
        std::string          network    = "";     ///< Сеть в которой находится адрес
        std::string          macAddress = "";     ///< Мак адрес
        int                  mtu        = 0;      ///< Значение МТУ
        bool                 isUp       = false;  ///< Поднят интерфейс или нет
        bool                 isLoopBack = false;  ///< Виртуальный адрес

        friend std::ostream& operator<<(std::ostream& os, const Interface& ni)
        {
            os << "Interface: " << ni.interface << "\n"
               << "Net mask: " << ni.netMask << "\n"
               << "BroadCast address: " << ni.broadcast << "\n"
               << "Network: " << ni.network << "\n"
               << "Is interface up: " << (ni.isUp ? "true" : "false") << "\n"
               << "Is interface LoopBack: " << (ni.isLoopBack ? "true" : "false") << "\n";
            for (const auto& element : ni.ip)
            {
                os << "Ip: " << element->ip << " version " << element->versionStr() << "\n";
            }
            os << "\n";
            return os;
        }
    };

    using InterfacePtr     = std::shared_ptr< Interface >;
    using InterfacesList = std::vector< InterfacePtr >;

}  // namespace iface_helper
#endif  // STRUCT_DEFINITION_H
