# README

## 注意事项

1. 头文件包含的顺序

  正确的：

  /* Socket */
  #include <winsock2.h>
  #include <Ws2tcpip.h>

  /* Windows */
  #include <Windows.h>
  
  否则会报错：
  
  sockaddr struct type redefinition
    
2. 声明链接库文件

  #pragma comment(lib, "Ws2_32.lib")
  
  