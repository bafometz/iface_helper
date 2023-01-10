#ifndef NETWORKINFORMER_H
#define NETWORKINFORMER_H
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <bitset>
#include "../../version.h"

struct Ip_s
{
    std::string ip = ""; ///< Человекочитаемый ip
    u_int64_t   ipInt = 0;
    bool isV4() {return ipVersion[0];};
    bool isV6() {return ipVersion[1];};

    //[0,0,0,0]
    std::bitset<4> ipVersion = 0; ///< 1 - ipV4, 2 - ipV6
};
using IpPtr     = std::shared_ptr<Ip_s>;

/**
 *  @brief Структура интерфейса, включает в себя
 *  информацию об одном из Интернет интерфейсов
 */
struct NetInfo
{
   std::string interface = "";///< Имя интерфейса
   std::vector<IpPtr> ip     ;///< Список ip адресов
   std::string ip_v4        = "";///< ip интерфейса
   std::string netMask   = "";///< Маска интерфейса
   std::string broadcast = "";///< Широковещательный адрес
   std::string network   = "";///< Сеть в которой находится адрес
   std::string macAddress = ""; ///< Мак адрес
   int         mtu = 0;       ///< Значение МТУ
   bool isUp =  false;        ///< Поднят интерфейс или нет
   bool isLoopBack = false;   ///< Виртуальный адрес
   friend std::ostream&
   operator<< ( std::ostream& os, const NetInfo& ni )
   {
      os << "Interface: " << ni.interface << "\n"
         << "Ip address: " << ni.ip_v4 << "\n"
         << "Net mask: " << ni.netMask << "\n"
         << "BroadCast address: " << ni.broadcast << "\n"
         << "Network: " << ni.network << "\n"
         << "Is interface up: " << (ni.isUp ? "true" : "false") << "\n"
         << "Is interface LoopBack: " << (ni.isLoopBack ? "true" : "false") << "\n"
         << "\n";
      return os;
   }
};

using NetInfoPtr     = std::shared_ptr<NetInfo>;
using InterfacesList = std::vector<NetInfoPtr>;

class UnixNetworkInformer;

class NetworkInformer
{
public:
    NetworkInformer();
    InterfacesList getInterfacesList(); ///< Возвращает список интерфейсов
    NetInfoPtr     getInterfaceInfo(const std::string& ifaceName); ///< Возвращает конкретный интерфейс
    void           update();                                        ///< Обновляет информацию
    ~NetworkInformer();

private:
    std::unique_ptr<UnixNetworkInformer> m_networkInformer;
};

#endif// NETWORKINFORMER_H
