#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>

#define PORT 8080

class HttpServer {
private:
    int port;

    static std::string parseHttpMethod(const std::string& request) {
        size_t space_pos = request.find(' ');
        if (space_pos != std::string::npos) {
            return request.substr(0, space_pos);
        }
        return "";
    }

    static std::string parseHttpPath(const std::string& request) {
        size_t first_space = request.find(' ');
        size_t second_space = request.find(' ', first_space + 1);
        if (first_space != std::string::npos && second_space != std::string::npos) {
            return request.substr(first_space + 1, second_space - first_space - 1);
        }
        return "/";
    }

    static std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return "";
        }

        std::ostringstream content;
        content << file.rdbuf();
        file.close();
        return content.str();
    }
    static std::string getContentType(const std::string& path) {
        if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
        if (path.ends_with(".css")) return "text/css";
        if (path.ends_with(".js")) return "application/javascript";
        if (path.ends_with(".json")) return "application/json";
        if (path.ends_with(".png")) return "image/png";
        if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
        if (path.ends_with(".gif")) return "image/gif";
        if (path.ends_with(".ico")) return "image/x-icon";
        return "text/plain";
    }

    static std::string generateHttpResponse(const std::string& method, const std::string& path) {
        std::string content;
        std::string content_type = "text/html";
        //
        // if (path == "/" || path == "/index.html") {
        //     // Try to read index.html from current directory
            content = readFile(".." + path);
        // }
        // Build HTTP response
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: " << content_type << "\r\n";
        response << "Content-Length: " << content.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << content;

        return response.str();
    }

public:
    explicit HttpServer(const int port) : port(port) {}

    [[noreturn]] int start() const  {
        struct sockaddr_in address{};
        constexpr int opt = 1;
        int addrLen = sizeof(address);
        char buffer[4096] = {0}; // Larger buffer for HTTP requests

        const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&address), static_cast<socklen_t>(addrLen)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 5) < 0) {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "HTTP Server listening on http://localhost:" << port << std::endl;

        while (true) {
            int new_sock = accept(server_fd, reinterpret_cast<struct sockaddr *>(&address), (socklen_t *)&addrLen);
            if (new_sock < 0) {
                perror("accept");
                continue;
            }

            memset(buffer, 0, sizeof(buffer));
            ssize_t bytes_read = read(new_sock, buffer, sizeof(buffer) - 1);

            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                std::string request(buffer);

                // Parse HTTP request
                std::string method = parseHttpMethod(request);
                std::string path = parseHttpPath(request);

                std::cout << "Request: " << method << " " << path << std::endl;

                // Generate and send HTTP response
                std::string response = generateHttpResponse(method, path);
                send(new_sock, response.c_str(), response.length(), 0);
            }

            close(new_sock);
        }

        close(server_fd);
        return 0;
    }
};

int main() {
    auto server = HttpServer(PORT);
    server.start();
    return 0;
}