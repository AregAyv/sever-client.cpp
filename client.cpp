#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

void communicate_with_server(const std::string& server_ip, int port) {
    while (true) {
        int m; 
        std::cin >> m;
        // Create server socket ,AF_INET indicates the use of IPv4, SOCK_STREAN indicates the use of TCP
        //returns  FD for socket server or -1  
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            std::cerr << "Failed creating socket" << std::endl;
            exit(-1);
        }

        sockaddr_in server_address;
        // Clears the memory of the server_address
        memset(&server_address, 0, sizeof(server_address));
         //Sets the address family of the server, indicating IPv4.
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        // Converts the server IP address from string format to a binary form and stores it in the sin_addr
        if (inet_pton(AF_INET, server_ip.c_str(), &(server_address.sin_addr)) <= 0) {
            std::cerr << "Failed  converting server IP address" << std::endl;
            exit(-1);
        }
        //connects with the server using the client socket and the server address structure if failed returnes -1
        if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
            std::cerr << "Couldnt connect to the server" << std::endl;
            close(client_socket);
            continue;
        }

        m = htonl(m);
       //Sends the value of m to the server
        if (send(client_socket, &m, sizeof(m), 0) == -1) {
            std::cerr << "Error sending data to server" << std::endl;
            close(client_socket);
            continue;
        }

        int result;
        //Receives the result from the server
        if (recv(client_socket, &result, sizeof(result), 0) == -1) {
            std::cerr << "Error receiving data from server" << std::endl;
            close(client_socket);
            continue;
        }
        result = ntohl(result);

        std::cout << "Result: " << result << std::endl;
      //Close socket
        close(client_socket);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
      exit(-1);
    }

    std::string server_ip = argv[1];
    int port = std::stoi(argv[2]);

    communicate_with_server(server_ip, port);

    return 0;
}
