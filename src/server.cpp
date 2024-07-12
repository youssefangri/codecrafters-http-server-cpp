#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <fstream>
#include <sstream>

void handle_http(int client_fd, struct sockaddr_in client_addr, std::string dir){
  std::string response;
  std::string data(2024, '\0');
  std::string content;
  
  ssize_t bytes_received = recv(client_fd, &data[0], data.length(), 0);
  if (bytes_received>0) {
      std::cout << "data: " << data << std::endl;
      if (data.starts_with("GET / HTTP/1.1")){
        response = "HTTP/1.1 200 OK\r\n\r\n";
      } else if (data.starts_with("GET /echo/")){
        content = data;
        content.erase(0,10);
        content.erase(content.find(" "), content.length());
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(content.length()) + "\r\n\r\n" + content;
      } else if (data.starts_with("GET /user-agent")){
        content = data;
        content.erase(0,content.find("User-Agent:")+12);
        content.erase(content.find("\r\n"), content.length());
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(content.length()) + "\r\n\r\n" + content;
      } else if (data.starts_with("GET /files/")){
        std::ifstream file;
        content = data;
        content.erase(0,11);
        content.erase(content.find(" "), content.length());
        file.open(dir+content);
        if (file.good()){
          std::stringstream file_content;
          file_content << file.rdbuf();
          response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + std::to_string(file_content.str().length()) + "\r\n\r\n" + file_content.str();
        } else {
          response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
        
      } else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
      }
  }
  send(client_fd, response.c_str(), response.length(),0);
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::string dir;

  if (argc == 3 && strcmp(argv[1], "--directory") == 0)
  {
  	dir = argv[2];
  }
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  //
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  while (1)
  {
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);  
    std::cout << "Client connected\n";
    // handle_http(client_fd,client_addr, dir);
    std::thread th(handle_http, client_fd, client_addr, dir);
    th.detach();
  }
  
  close(server_fd);
  return 0;
}
