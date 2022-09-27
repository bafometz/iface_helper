# How to build 

## Build standalone shared lib

```bash
 git clone https://gitlab.com/Bafometz/InterfaceInformation.git && cd interface_information
 cmake && make
```

## Build standalone static lib

```bash
 git clone https://gitlab.com/Bafometz/InterfaceInformation.git && cd InterfaceInformation
 cmake -DMAKE_SHARED && make
```



## Build with your project

Include to CMakeLists.txt and link library

```cmake
add_subdirectory(<where_you_placed_sources/interface_information>)
....
target_link_libraries(${PROJECT_NAME}
    IfaceInformation
    )
```


