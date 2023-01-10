#include "../ifaceinformation.h"

int main()
{
  NetworkInformer inf;
  
  for(const auto& iface : inf.getInterfacesList())
  {
      if(iface) std::cout << iface << std::endl;
  }
  return 0;
}