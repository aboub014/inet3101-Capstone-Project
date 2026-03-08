/**
 * network.c - Socket-based availability lookup
 *
 * Provides a simple TCP server that responds to availability queries
 * and a client function that connects to it. This lets you run the
 * server in one terminal and check availability from another.
 *
 * Protocol is super simple (text-based):
 *   Client sends: "CHECK <trip_id>\n"
 *   Server responds with seat availability info, then closes connection.
 *   Client sends: "HOLD <trip_id> <seat_number> <name>\n"
 *   Server attempts to hold the seat and responds with result.
 *   Client sends: "QUIT\n" -> server closes that connection.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "reservation.h"

#define BUFFER_SIZE 2048

/**
 * Handles one client connection on the server side.
 * Reads commands and sends back availability info.
 */
static void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (bytes <= 0) break;

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strncmp(buffer, "CHECK ", 6) == 0) {
   
            int trip_id = atoi(buffer + 6);
            int idx = find_trip_index(trip_id);

            char response[BUFFER_SIZE];
            if (idx == -1) {
                snprintf(response, BUFFER_SIZE, "ERROR: Trip %d not found.\n", trip_id);
            } else {
                int avail = 0;
                for (int s = 0; s < trips[idx].total_seats; s++) {
                    if (!trips[idx].seats[s].is_booked) avail++;
                }
                snprintf(response, BUFFER_SIZE,
                         "Trip %d (%s): %s -> %s on %s at %s\n"
                         "Price: $%.2f | Seats: %d/%d available\n"
                         "Available seats: ",
                         trips[idx].id, trips[idx].type,
                         trips[idx].origin, trips[idx].destination,
                         trips[idx].date, trips[idx].time,
                         trips[idx].price, avail, trips[idx].total_seats);

            
                char seat_list[BUFFER_SIZE] = "";
                for (int s = 0; s < trips[idx].total_seats; s++) {
                    if (!trips[idx].seats[s].is_booked) {
                        char tmp[16];
                        snprintf(tmp, 16, "%d ", trips[idx].seats[s].seat_number);
                        strcat(seat_list, tmp);
                    }
                }
                strcat(response, seat_list);
                strcat(response, "\n");
            }

            write(client_fd, response, strlen(response));

        } else if (strncmp(buffer, "HOLD ", 5) == 0) {
        
            int trip_id, seat_num;
            char name[MAX_STRING];

            if (sscanf(buffer + 5, "%d %d %255[^\n]", &trip_id, &seat_num, name) < 3) {
                char *err = "ERROR: Usage: HOLD <trip_id> <seat> <name>\n";
                write(client_fd, err, strlen(err));
                continue;
            }

            int idx = find_trip_index(trip_id);
            char response[BUFFER_SIZE];

            if (idx == -1) {
                snprintf(response, BUFFER_SIZE, "ERROR: Trip %d not found.\n", trip_id);
            } else if (seat_num < 1 || seat_num > trips[idx].total_seats) {
                snprintf(response, BUFFER_SIZE, "ERROR: Invalid seat number.\n");
            } else if (trips[idx].seats[seat_num - 1].is_booked) {
                snprintf(response, BUFFER_SIZE, "ERROR: Seat %d is already taken.\n", seat_num);
            } else {
             
                trips[idx].seats[seat_num - 1].is_booked = 1;
                strcpy(trips[idx].seats[seat_num - 1].passenger_name, name);

                
                if (reservation_count < MAX_RESERVATIONS) {
                    Reservation *r = &reservations[reservation_count];
                    r->id = next_res_id++;
                    r->trip_id = trip_id;
                    r->seat_number = seat_num;
                    strcpy(r->passenger_name, name);
                    strcpy(r->status, "Confirmed");
                    reservation_count++;
                }

                save_trips();
                save_bookings();
                snprintf(response, BUFFER_SIZE,
                         "OK: Seat %d on Trip %d held for %s.\n",
                         seat_num, trip_id, name);
            }

            write(client_fd, response, strlen(response));

        } else if (strncmp(buffer, "QUIT", 4) == 0) {
            char *bye = "Goodbye.\n";
            write(client_fd, bye, strlen(bye));
            break;
        } else {
            char *help = "Commands: CHECK <trip_id> | HOLD <trip_id> <seat> <name> | QUIT\n";
            write(client_fd, help, strlen(help));
        }
    }

    close(client_fd);
}

/**
 * Starts the TCP server. Listens for connections and handles each one.
 * This runs forever (until you Ctrl+C it).
 */
void start_server(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        return;
    }

    printf("Server listening on port %d. Press Ctrl+C to stop.\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        printf("Client connected from %s\n", inet_ntoa(client_addr.sin_addr));
        handle_client(client_fd);
        printf("Client disconnected.\n");
    }

    close(server_fd);
}

/**
 * Client function: connects to a server and checks availability for a trip.
 * Used when you run: ./reservation --check <trip_id>
 */
void check_availability_remote(const char *host, int port, int trip_id) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        printf("(Make sure the server is running with --server flag)\n");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        printf("Invalid host address: %s\n", host);
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        printf("(Is the server running? Start with: ./reservation --server)\n");
        close(sock);
        return;
    }

    char cmd[BUFFER_SIZE];
    snprintf(cmd, BUFFER_SIZE, "CHECK %d\n", trip_id);
    write(sock, cmd, strlen(cmd));

    char response[BUFFER_SIZE];
    memset(response, 0, BUFFER_SIZE);
    read(sock, response, BUFFER_SIZE - 1);
    printf("\n--- Server Response ---\n%s\n", response);

    write(sock, "QUIT\n", 5);

    close(sock);
}
