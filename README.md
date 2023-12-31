# How to build 

## Build standalone static lib

```bash
 git clone https://gitlab.com/Bafometz/InterfaceInformation.git && cd interface_information
 cmake && make
```

## Build standalone shared lib

```bash
 git clone https://gitlab.com/Bafometz/InterfaceInformation.git && cd iface_information
 cmake -DMAKE_SHARED && make
```



## Build with your project

Include to CMakeLists.txt and link library

```cmake
add_subdirectory(<where_you_placed_sources/interface_information>)
....
target_link_libraries(${PROJECT_NAME}
    iface_information
    )
```



## Usage



Information about interfaces is updated when the constructor is called, if there is a suspicion that information about interfaces has changed, you need to call the **update ()** method.



```c++
#include <iostream>
#include "InterfacesLib/src/interfaceInformation/ifaceinformation.h"

int main()
{

    iface_lib::IfaceHelper inf;
    iface_lib::InterfacesList list;
    inf.getInterfacesList(list);

    for(const auto element : list)
    {
        std::cout << *element << std::endl;
    }


    /*
     * Many hours(?) after, when you will get information again,
     * call update() function, mb something change 
    */

    inf.update();

    for(const auto element : list)
    {
        std::cout << *element << std::endl;
    }

    return 0;
}
```
