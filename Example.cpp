#include <iostream>
#include "TCPServer.hpp"

int main()
{
	dk::TCPServer* server = new dk::TCPServer(2227);
	return 0;
}

void dk::TCPServer::OnServerStartup(dk::TCPServer* server)
{
	SetConsoleTitleA(std::string("Running on port: " + std::to_string(server->port)).c_str());
	std::cout << "Server is running..." << std::endl;
}

bool dk::TCPServer::OnClientConnect(dk::ConnectedClient* client)
{
	if (client->serverInstance->connectedClients.size() < 3)
	{
		std::cout << "client connected, IP: " << client->GetIP() << std::endl;
		client->Send("[SERVER] Welcome, " + client->GetIP());
		return true;
	}
	else
	{
		std::cout << client->GetIP() << " tried to connect, but server reached max online" << std::endl;
		client->Send("[SERVER] Reached maximum online.");
		return false;
	}
}

void dk::TCPServer::OnMessageReceive(dk::ConnectedClient* client, char* data)
{
	std::cout << "IP: " << client->GetIP() << ", Data: " << data << std::endl;
	client->Send("[SERVER] You've sent: " + std::string(data));
}

void dk::TCPServer::OnClientDisconnect(dk::ConnectedClient* client)
{
	std::cout << "IP: " << client->GetIP() << ", disconnected" << std::endl;
}

