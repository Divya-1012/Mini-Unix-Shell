#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 5000
#define BUFFER_SIZE 8192
#define MAX_OUTPUT 4096
#define MAX_COMMAND_LENGTH 1024

// Function from shell.c
extern int execute_shell_command(char *input, char *output, size_t output_size);

// Escape special characters for safe JSON output
void json_escape(char *dst, const char *src, size_t dst_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dst_size - 2; i++) {
        switch (src[i]) {
            case '"':  dst[j++] = '\\'; dst[j++] = '"'; break;
            case '\\': dst[j++] = '\\'; dst[j++] = '\\'; break;
            case '\n': dst[j++] = '\\'; dst[j++] = 'n'; break;
            case '\r': dst[j++] = '\\'; dst[j++] = 'r'; break;
            case '\t': dst[j++] = '\\'; dst[j++] = 't'; break;
            default:   dst[j++] = src[i]; break;
        }
    }
    dst[j] = '\0';
}

// Decode URL-encoded form data (e.g., %20 → space)
void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            a = (a >= 'a') ? a - 'a' + 10 : (a >= 'A') ? a - 'A' + 10 : a - '0';
            b = (b >= 'a') ? b - 'a' + 10 : (b >= 'A') ? b - 'A' + 10 : b - '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Send HTTP response to client
void send_response(int client_socket, int status_code, const char *status_text, const char *content_type, const char *body) {
    char header[BUFFER_SIZE];
    int header_len = snprintf(header, BUFFER_SIZE,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n",
        status_code, status_text, content_type, strlen(body));
    
    write(client_socket, header, header_len);
    write(client_socket, body, strlen(body));
}

// Send static file like index.html, style.css, or script.js
void send_file(int client_socket, const char *filename, const char *content_type) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        send_response(client_socket, 404, "Not Found", "text/plain", "File not found");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    fread(content, 1, file_size, file);
    content[file_size] = '\0';
    fclose(file);

    send_response(client_socket, 200, "OK", content_type, content);
    free(content);
}

// Handle a single client request
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);

    if (bytes_read <= 0) return;
    buffer[bytes_read] = '\0';

    char method[16], path[256], protocol[16];
    sscanf(buffer, "%s %s %s", method, path, protocol);

    // Handle GET requests (serve frontend files)
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0)
            send_file(client_socket, "index.html", "text/html");
        else if (strcmp(path, "/style.css") == 0)
            send_file(client_socket, "style.css", "text/css");
        else if (strcmp(path, "/script.js") == 0)
            send_file(client_socket, "script.js", "application/javascript");
        else
            send_response(client_socket, 404, "Not Found", "text/plain", "Not found");
    }
    // Handle POST /execute for command execution
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/execute") == 0) {
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4;

            char command[MAX_COMMAND_LENGTH];
            char *cmd_start = strstr(body, "command=");
            if (cmd_start) {
                cmd_start += 8;
                char *cmd_end = strchr(cmd_start, '&');
                if (cmd_end) *cmd_end = '\0';
                url_decode(command, cmd_start);

                char output[MAX_OUTPUT];
                execute_shell_command(command, output, sizeof(output));

                char escaped_output[MAX_OUTPUT * 2];
                json_escape(escaped_output, output, sizeof(escaped_output));

                char response[MAX_OUTPUT * 2 + 100];
                snprintf(response, sizeof(response),
                    "{\"output\": \"%s\"}",
                    strlen(escaped_output) > 0 ? escaped_output : "Command executed successfully");

                send_response(client_socket, 200, "OK", "application/json", response);
            } else {
                send_response(client_socket, 400, "Bad Request", "text/plain", "Missing command");
            }
        }
    } else {
        send_response(client_socket, 405, "Method Not Allowed", "text/plain", "Invalid request");
    }
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Web Shell Server running on port %d\n", PORT);
    printf("Open http://localhost:%d in your browser\n", PORT);

    // Main loop: handle one request at a time
    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        handle_client(client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}
