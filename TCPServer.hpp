/*
* Single threaded multi-client C++ TCP server framework.
* By D. Krichevsky
*/

#define _CRT_SECURE_NO_WARNINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma warning( disable : 26495 )

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <list>

#pragma comment(lib, "ws2_32.lib")

#define SOCKET_STATUS_READ 0x1
#define SOCKET_STATUS_WRITE 0x2
#define SOCKET_STATUS_EXCEPT 0x4

#define SOCKET_TIMEOUT 7000

namespace dk
{

    class ConnectedClient;
    class TCPServer;


    class TCPServer
    {
    private:
        SOCKET serverSocket;
        unsigned short port;
        std::list<ConnectedClient*> connectedClients;
        std::vector<char> recvBuffer;
        int GetSocketStatus(SOCKET socket, int status);
        void KeepAlive(SOCKET socket);
        void HandleIncoming();
        void HandleData();
        void CleanupSockets();
    protected:
        /// <summary>
        /// Called when server starts.
        /// </summary>
        /// <param name="server">pointer to the server instance</param>
        void OnServerStartup(TCPServer* server);

        /// <summary>
        /// Called when client sends over data.
        /// </summary>
        /// <param name="client">client pointer</param>
        /// <param name="data">data pointer</param>
        void OnMessageReceive(ConnectedClient* client, char* data);

        /// <summary>
        /// Called when client request a connection.
        /// </summary>
        /// <param name="client">client pointer</param>
        /// <returns>True to accept the connection, False otherwise</returns>
        bool OnClientConnect(ConnectedClient* client);

        /// <summary>
        /// Called when client disconnects.
        /// </summary>
        /// <param name="client">client pointer</param>
        void OnClientDisconnect(ConnectedClient* client);
    public:
        TCPServer(unsigned short port);
        ~TCPServer();
        void CloseSocket(SOCKET socket);
        void Exit();
    };


    class ConnectedClient
    {
    private:
    public:
        TCPServer* serverInstance;
        SOCKET socket;
        sockaddr_in addr;
        ConnectedClient(TCPServer* instance);
        ~ConnectedClient();
        void Send(std::string data);
        void Close();
        std::string GetIP();
    };


    /// <summary>
    /// Creates the server instance and handles all the connections and data.
    /// </summary>
    /// <param name="port">server port</param>
    TCPServer::TCPServer(unsigned short port)
    {
        this->port = port;
        WSADATA wsaData;
        WORD vers = MAKEWORD(2, 2);
        if (WSAStartup(vers, &wsaData) != NO_ERROR) return;
        this->serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (this->serverSocket != INVALID_SOCKET)
        {
            sockaddr_in hint;
            hint.sin_family = AF_INET;
            hint.sin_port = htons(this->port);
            hint.sin_addr.S_un.S_addr = INADDR_ANY;
            bind(this->serverSocket, (sockaddr*)&hint, sizeof(hint));
            listen(this->serverSocket, SOMAXCONN);

            //Non blocking socket
            unsigned long argp = 1;
            setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&argp, sizeof(argp));
            ioctlsocket(this->serverSocket, FIONBIO, &argp);
            //ServerStartup callback call
            this->OnServerStartup(this);
            //
            while (int SocketStatus = this->GetSocketStatus(this->serverSocket, SOCKET_STATUS_READ) != SOCKET_ERROR)
            {
                this->HandleIncoming();
                this->HandleData();
                this->CleanupSockets();
                Sleep(1);
            }
        }
    }

    /// <summary>
    /// Destroys all the connections and clear memory.
    /// </summary>
    TCPServer::~TCPServer()
    {
        this->connectedClients.remove_if([](ConnectedClient* client) {
            delete client;
            return true;
        });
        WSACleanup();
    }

    /// <summary>
    /// Function will cause the server to fail and exit.
    /// </summary>
    void TCPServer::Exit()
    {
        this->serverSocket = INVALID_SOCKET;
    }

    /// <summary>
    /// Check if a certain socket is sending any data or packets.
    /// </summary>
    /// <param name="socket"></param>
    /// <param name="status"></param>
    /// <returns></returns>
    int TCPServer::GetSocketStatus(SOCKET socket, int status)
    {
        static timeval instantSpeedPlease = { 0,0 };
        fd_set a = { 1, {socket} };
        fd_set* read = ((status & 0x1) != 0) ? &a : NULL;
        fd_set* write = ((status & 0x2) != 0) ? &a : NULL;
        fd_set* except = ((status & 0x4) != 0) ? &a : NULL;
        int result = select(0, read, write, except, &instantSpeedPlease);
        if (result == SOCKET_ERROR)
        {
            result = WSAGetLastError();
        }
        if (result < 0 || result > 3)
        {
            return SOCKET_ERROR;
        }
        return result;
    }


    /// <summary>
    /// Closes the client socket.
    /// </summary>
    /// <param name="socket"></param>
    void TCPServer::CloseSocket(SOCKET socket)
    {
        std::list<ConnectedClient*>::iterator it;
        it = std::find_if(this->connectedClients.begin(), this->connectedClients.end(), [&](ConnectedClient* client) {
            if (client->socket == socket)
            {
                this->OnClientDisconnect(client);
                return true;
            }
            else return false;
        });
        if (it != this->connectedClients.end())
        {
            it._Ptr->_Myval->socket = INVALID_SOCKET;
        }
        shutdown(socket, SD_BOTH);
        closesocket(socket);
    }

    /// <summary>
    /// Handles the incoming connections and passes it to the OnClientConnect function.
    /// </summary>
    void TCPServer::HandleIncoming()
    {
        int client_size = sizeof(sockaddr_in);
        SOCKET tempSocket;
        sockaddr_in tempSocket_sockaddr_in;
        tempSocket = accept(this->serverSocket, (struct sockaddr*)&tempSocket_sockaddr_in, &client_size);
        if (tempSocket != INVALID_SOCKET)
        {
            ConnectedClient* clientSocket = new ConnectedClient(this);
            clientSocket->socket = tempSocket;
            clientSocket->addr = tempSocket_sockaddr_in;
            if (this->OnClientConnect(clientSocket) == true)
            {
                this->KeepAlive(clientSocket->socket);
                this->connectedClients.push_back(clientSocket);
                return;
            }
            shutdown(clientSocket->socket, SD_BOTH);
            closesocket(clientSocket->socket);
            delete clientSocket;
        }
    }

    /// <summary>
    /// Handles data from connected clients and passes it to the OnMessageReceive function.
    /// </summary>
    void TCPServer::HandleData()
    {
        for (ConnectedClient* Client : this->connectedClients)
        {
            int SocketStatus = this->GetSocketStatus(Client->socket, SOCKET_STATUS_READ);
            if (SocketStatus == 1 && Client->socket != INVALID_SOCKET)
            {
                unsigned long BufferSize = 0;
                ioctlsocket(Client->socket, FIONREAD, &BufferSize);
                if (BufferSize > 0)
                {
                    this->recvBuffer.clear();
                    unsigned long BufferFreePointer = 0;
                    int RecvStatus = 0;
                    do {
                        BufferFreePointer = (unsigned long)this->recvBuffer.size();
                        this->recvBuffer.resize((size_t)BufferFreePointer + BufferSize, 0);
                        RecvStatus = recv(Client->socket, (char*)(&this->recvBuffer[BufferFreePointer]), BufferSize, 0);
                    } while (RecvStatus > 0);
                    this->recvBuffer.resize((size_t)BufferFreePointer, 0);
                    //
                    this->OnMessageReceive(Client, this->recvBuffer.data());
                    //
                    this->recvBuffer.clear();
                    this->recvBuffer.shrink_to_fit();
                }
                else if (BufferSize <= 0) //disconnect
                {
                    this->CloseSocket(Client->socket);
                }
            }
        }
    }

    /// <summary>
    /// Clears invalid sockets from the list.
    /// </summary>
    void TCPServer::CleanupSockets()
    {
        this->connectedClients.remove_if([](ConnectedClient* client)
        {
            if (client->socket == INVALID_SOCKET)
            {
                delete client;
                return true;
            }
            else return false;
        });
    }

    /// <summary>
    /// Set socket options to KeepAlive
    /// </summary>
    /// <param name="socket"></param>
    void TCPServer::KeepAlive(SOCKET socket)
    {
        int yes = 1;
        setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&yes), sizeof(int));
        int idle = 1;
        setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, reinterpret_cast<char*>(&idle), sizeof(int));
        int interval = 5;
        setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, reinterpret_cast<char*>(&interval), sizeof(int));
        int maxpkt = 3;
        setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, reinterpret_cast<char*>(&maxpkt), sizeof(int));
    }

    //

    /// <summary>
    /// Saves the server instance in the client instance.
    /// So we could get access to server info from client callbacks.
    /// </summary>
    /// <param name="instance"></param>
    ConnectedClient::ConnectedClient(TCPServer* instance)
    {
        this->serverInstance = instance;
    }

    ConnectedClient::~ConnectedClient() { }

    /// <summary>
    /// Return data to the client.
    /// </summary>
    /// <param name="data">string</param>
    void ConnectedClient::Send(std::string data)
    {
        send(this->socket, data.c_str() + '\0', (int)data.length() + 1, 0);
    }

    /// <summary>
    /// Closes the current client connection.
    /// </summary>
    void ConnectedClient::Close()
    {
        this->serverInstance->CloseSocket(this->socket);
    }

    /// <summary>
    /// Retrieves the current client IP.
    /// </summary>
    /// <returns></returns>
    std::string ConnectedClient::GetIP()
    {
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(this->addr.sin_addr), str, INET_ADDRSTRLEN);
        return std::string(str);
    }
}