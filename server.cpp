#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <functional>



// function to check if a number is prime
bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

// return m-th prime number
int calculate_prime(int m) {
    int count = 0;
    int i = 2;
    while (count < m) {
        if (is_prime(i)) {
            count++;
        }
        i++;
    }
    return i - 1;
}

// function to handle a single client connection
void handle_client(int client_socket) {
    int m;
    //read from socket and write it in m, return num or readen bytes or -1 id couldnt read
    if (recv(client_socket, &m, sizeof(m), 0) == -1) {
        std::cerr << "could not read from socket" << std::endl;
        //if could not read then closes socket
        close(client_socket);
        return;
    }
    //convertes m to host byte order, it is needed if information was send by different order than host
    m = ntohl(m);
    //fined m prime number
    int result = calculate_prime(m);
    result = htonl(result);
    // returns result to client, returns same as recv()
    if (send(client_socket, &result, sizeof(result), 0) == -1) {
        std::cerr << "Could not send data back to client" << std::endl;
    }
    //close socket
    close(client_socket);
}


void serve_start(int port) {
    // Create server socket ,AF_INET indicates the use of IPv4, SOCK_STREAN indicates the use of TCP
    //returns  FD for socket server or -1  
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error creating server socket" << std::endl;
        exit(-1);
    }

    int yes = 1;
    //installs REUSEADDR operatinon for server whick allows to reuse adress without waiting
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        std::cerr << "Error setting socket options" << std::endl;
        exit(-1);
    }

    // Bind to address and port
    sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    //inst adr_family whick corresponds to IPv4
    server_address.sin_family = AF_INET;
    //inst IP adress of server to ...
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    //connsect servers socket to port
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        std::cerr << "Error failed to connect" << std::endl;
        exit(-1);
    }

    // Listen for incoming connections
    //Changes socket into listening mode
    if (listen(server_socket, SOMAXCONN) == -1) {
        std::cerr << "Error listening for connections" << std::endl;
        exit(-1);
    }

    // Create a queue to hold client sockets
   std:: queue<int> client_queue;

    // Create a mutex and condition variable for thread synchronization
    std::mutex client_queue_mutex;
    std:: condition_variable client_queue_cv;

    std::vector<std::thread> workers;

    for (int i = 0; i < 10; ++i) {
        workers.push_back(std::thread([&]() {
            while (true) {
                int client_socket;
                {
                    std::unique_lock<std::mutex> lock(client_queue_mutex);
                    client_queue_cv.wait(lock, [&]() { return !client_queue.empty(); });
                    client_socket = client_queue.front();
                    client_queue.pop();
                }
                handle_client(client_socket);
            }
        }));
    }


while (true) {
        // Accept a client connection and obtain the client socket
        sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) {
            std::cerr << "Denied" << std::endl;
            continue;
        }

        // Enqueue the client socket for processing
        {
            // Acquires a lock
            std::lock_guard<std::mutex> lock(client_queue_mutex);
            //Adds the client_socket to the end of the client queue
            client_queue.push(client_socket);
        }
        // Notify a worker thread to process the enqueued client socket
        client_queue_cv.notify_one();  
    }

     for (auto& worker : workers) {
        worker.join();
    }
    close(server_socket);
}


int main(int argc, char* argv[]) {
     if (argc != 2) {
        std::cerr << "too few arguments" << std::endl;
        exit(-1);
    }

    int port = atoi(argv[1]);

    // Start serving incoming client connections
    serve_start(port);

    return 0;
}
