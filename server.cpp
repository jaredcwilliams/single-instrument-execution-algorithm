#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <memory>
#include <iostream>
#include <print>

int main() {
    // load up servinfo using hints
    addrinfo hints{.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
    addrinfo* servinfo{};
    if (int status{getaddrinfo("127.0.0.1", "14200", &hints, &servinfo)}; status != 0) {
        std::println(std::cerr, "gai error: {}", gai_strerror(status));
        return EXIT_FAILURE;
    }
    
    // get socket file descriptor
    int s {socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)};
    if (s == -1) {
        std::println(std::cerr, "socket error");
        return EXIT_FAILURE;
    }
    
    // bind socket
    if (bind(s, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        std::println(std::cerr, "bind error");
        return EXIT_FAILURE;
    }
    freeaddrinfo(servinfo);
    
    // listen on socket
    if (listen(s, 10) == -1) {
        std::println(std::cerr, "listen error");
        return EXIT_FAILURE;
    }

    // accept connection
    sockaddr their_addr{};
    socklen_t addr_size{sizeof(sockaddr)};
    int new_fd = accept(s, &their_addr, &addr_size);
    if (new_fd == -1) {
        std::println(std::cerr, "accept error");
        return EXIT_FAILURE;
    }
    
    // receive information
    std::byte buff[48] {};
    ssize_t buff_size {0};
    while (true) {
        if (ssize_t r = recv(new_fd, (void*)(buff + buff_size), static_cast<std::size_t>(48 - buff_size), 0); r < 0) {
            std::println(std::cerr, "receive error");
            return EXIT_FAILURE;
        } else if (r == 0) {
            std::println("closed connection");
            break;
        } else {
            std::println("received {} bytes", r);
            buff_size += r;
            if (buff_size == 48) {
                buff_size = 0;
            }
        }
    }
    
    // close sockets
    close(new_fd);
    close(s);

    return 0;
}
