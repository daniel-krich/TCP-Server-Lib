# C++ TCP-Server-Framework

Description:
- Single threaded.
- Multi-client.
- Non-blocking.
- Lightweight, simple, single .hpp include file.
- Tested on Windows 10.

## Usage template
Must define all the standard callbacks.
- **OnServerStartup**
- **OnClientConnect**
- **OnMessageReceive**
- **OnClientDisconnect**

```C++
#include "TCPServer.hpp"

int main()
{
    dk::TCPServer* server = new dk::TCPServer(SERVER_PORT);
    return 0;
}

void dk::TCPServer::OnServerStartup(dk::TCPServer* server)
{
}

bool dk::TCPServer::OnClientConnect(dk::ConnectedClient* client)
{
    return true; //  Accept incoming connection
}

void dk::TCPServer::OnMessageReceive(dk::ConnectedClient* client, std::vector<char>* data)
{
}

void dk::TCPServer::OnClientDisconnect(dk::ConnectedClient* client)
{
}
```
