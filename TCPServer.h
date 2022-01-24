/*
* Single threaded multi-client C++ TCP server framework.
* By D. Krichevsky
*/

#define _CRT_SECURE_NO_WARNINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma warning( disable : 26495 )

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
        void OnMessageReceive(ConnectedClient* client, std::vector<char>* data);

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
}