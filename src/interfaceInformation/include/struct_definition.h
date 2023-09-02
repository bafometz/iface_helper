#ifndef STRUCT_DEFINITION_H
#define STRUCT_DEFINITION_H
#include <array>
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
        std::array< uint32_t, 4 > ip { 0, 0, 0, 0 };
        u_int64_t                 ipInt { 0 };
        ip_version                version { ip_version::empty };

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

        std::string toStr()
        {
            switch (version)
            {
            case iface_lib::ip_version::empty:
                return "";
            case iface_lib::ip_version::ip_v4:
                return ipV4String();
            case iface_lib::ip_version::ip_v6:
                return ipV6String();
            default:
                return "";
            }
            return "";
        }

      private:
        std::string ipV4String()
        {
            std::string result;
            result.append(std::to_string(ip.at(0) & 0xFF));
            result.push_back('.');
            result.append(std::to_string(ip.at(0) >> 8 & 0xFF));
            result.push_back('.');
            result.append(std::to_string(ip.at(0) >> 16 & 0xFF));
            result.push_back('.');
            result.append(std::to_string(ip.at(0) >> 24 & 0xFF));
            return result;
        }

        std::string ipV6String()
        {
            auto toHex = [] (uint16_t val) ->std::string
            {
                static const char* digits = "0123456789ABCDEF";
                std::string retStr{digits[(val >> 4) & 0x0F] , digits[val & 0x0F]};
                return retStr;
            };

            std::string  retStr;
            for(const auto val : ip)
            {
                int first = (val >> 16) & 0xffff;
                int second = val & 0xffff;
                retStr.append(toHex(first));
                retStr.append(toHex(second));
                retStr.append(":");
            }
            retStr.pop_back();
            return retStr;
        }
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
                os << "Ip: " << element->toStr() << " version " << element->versionStr() << "\n";
            }
            os << "\n";
            return os;
        }
    };

    using InterfacePtr   = std::shared_ptr< Interface >;
    using InterfacesList = std::vector< InterfacePtr >;

}  // namespace iface_lib
#endif  // STRUCT_DEFINITION_H
