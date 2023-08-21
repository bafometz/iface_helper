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
        int        ip { 0 };  ///< Человекочитаемый ip
        u_int64_t  ipInt { 0 };
        ip_version version { ip_version::empty };

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
            result.append(std::to_string(ip & 0xFF));
            result.push_back('.');
            result.append(std::to_string(ip >> 8 & 0xFF));
            result.push_back('.');
            result.append(std::to_string(ip >> 16 & 0xFF));
            result.push_back('.');
            result.append(std::to_string(ip >> 24 & 0xFF));
            return result;
        }

        std::string ipV6String()
        {
            /*
             * Note that int32_t and int16_t need only be "at least" large enough
             * to contain a value of the specified size.  On some systems, like
             * Crays, there is no such thing as an integer variable with 16 bits.
             * Keep this in mind if you think this function should have been coded
             * to use pointer overlays.  All the world's not a VAX.
             */
            char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
            auto NS_IN6ADDRSZ {16};
            auto NS_INT16SZ {2};

            struct
            {
                int base, len;
            } best, cur;

            u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
            int   i;
            /*
             * Preprocess:
             *	Copy the input (bytewise) array into a wordwise array.
             *	Find the longest run of 0x00's in src[] for :: shorthanding.
             */
            memset(words, '\0', sizeof words);
            for (i = 0; i < NS_IN6ADDRSZ; i += 2)
                words[i / 2] = (src[i] << 8) | src[i + 1];
            best.base = -1;
            cur.base  = -1;
            best.len  = 0;
            cur.len   = 0;
            for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
            {
                if (words[i] == 0)
                {
                    if (cur.base == -1)
                        cur.base = i, cur.len = 1;
                    else
                        cur.len++;
                }
                else
                {
                    if (cur.base != -1)
                    {
                        if (best.base == -1 || cur.len > best.len) best = cur;
                        cur.base = -1;
                    }
                }
            }
            if (cur.base != -1)
            {
                if (best.base == -1 || cur.len > best.len) best = cur;
            }
            if (best.base != -1 && best.len < 2) best.base = -1;
            /*
             * Format the result.
             */
            tp = tmp;
            for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
            {
                /* Are we inside the best run of 0x00's? */
                if (best.base != -1 && i >= best.base && i < (best.base + best.len))
                {
                    if (i == best.base) *tp++ = ':';
                    continue;
                }
                /* Are we following an initial run of 0x00s or any real hex? */
                if (i != 0) *tp++ = ':';
                /* Is this address an encapsulated IPv4? */
                if (i == 6 && best.base == 0 && (best.len == 6 || (best.len == 5 && words[5] == 0xffff)))
                {
                    if (!inet_ntop4(src + 12, tp, sizeof tmp - (tp - tmp))) return (NULL);
                    tp += strlen(tp);
                    break;
                }
                tp += SPRINTF((tp, "%x", words[i]));
            }
            /* Was it a trailing run of 0x00's? */
            if (best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ)) *tp++ = ':';
            *tp++ = '\0';
            /*
             * Check for overflow, copy, and we're done.
             */
            if (( socklen_t )(tp - tmp) > size)
            {
                __set_errno(ENOSPC);
                return (NULL);
            }
            return strcpy(dst, tmp);
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
