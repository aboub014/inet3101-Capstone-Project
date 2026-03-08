/**
 * main.c - Entry point for the Reservation System
 *
 * Parses command-line flags and runs the menu loop.
 * Supports:
 *   --datadir <path>   set where data files are stored
 *   --server           start in server mode (socket availability)
 *   --port <number>    set port for server mode (default 8080)
 *   --check <trip_id>  check availability on a remote server
 *   --host <addr>      host address for remote check (default 127.0.0.1)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reservation.h"

char data_dir[MAX_STRING] = ".";  

/**
 * Builds a full file path from the data directory and filename.
 * Example: if data_dir is "./data" and filename is "trips.txt",
 * result is "./data/trips.txt"
 */
void build_filepath(char *dest, const char *filename) {
    char tmp[MAX_STRING * 2];
    sprintf(tmp, "%s/%s", data_dir, filename);
    strncpy(dest, tmp, MAX_STRING - 1);
    dest[MAX_STRING - 1] = '\0';
}

/**
 * Prints the main menu options
 */
void print_menu() {
    printf("\n========================================\n");
    printf("   Air/Train/Bus Reservation System\n");
    printf("========================================\n");
    printf("  1. Add Trip\n");
    printf("  2. View All Trips\n");
    printf("  3. Update Trip\n");
    printf("  4. Delete Trip\n");
    printf("  5. Search Trips\n");
    printf("  6. View Seat Map\n");
    printf("  7. Make Reservation\n");
    printf("  8. Cancel Reservation\n");
    printf("  9. View Reservations\n");
    printf(" 10. View Waitlist\n");
    printf(" 11. Print Report\n");
    printf(" 12. Check Availability (Network)\n");
    printf("  0. Exit\n");
    printf("----------------------------------------\n");
    printf("Enter choice: ");
}

int main(int argc, char *argv[]) {
    int server_mode = 0;
    int port = DEFAULT_PORT;
    int remote_check = 0;
    int check_trip_id = -1;
    char host[MAX_STRING] = "127.0.0.1";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--datadir") == 0 && i + 1 < argc) {
            strncpy(data_dir, argv[++i], MAX_STRING - 1);
            data_dir[MAX_STRING - 1] = '\0';
        } else if (strcmp(argv[i], "--server") == 0) {
            server_mode = 1;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--check") == 0 && i + 1 < argc) {
            remote_check = 1;
            check_trip_id = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            strncpy(host, argv[++i], MAX_STRING - 1);
            host[MAX_STRING - 1] = '\0';
        } else {
            printf("Unknown flag: %s\n", argv[i]);
            printf("Usage: ./reservation [--datadir path] [--server] [--port num]\n");
            printf("       ./reservation --check <trip_id> [--host addr] [--port num]\n");
            return 1;
        }
    }

    load_trips();
    load_bookings();
    load_waitlist();

    if (server_mode) {
        printf("Starting server on port %d...\n", port);
        printf("Data directory: %s\n", data_dir);
        start_server(port);
        return 0;
    }

    if (remote_check) {
        check_availability_remote(host, port, check_trip_id);
        return 0;
    }

    printf("Data directory: %s\n", data_dir);
    printf("Loaded %d trip(s), %d reservation(s), %d waitlist entry/entries.\n",
           trip_count, reservation_count, waitlist_count);

    int choice;
    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            
            while (getchar() != '\n');
            printf("Please enter a number.\n");
            continue;
        }
        getchar(); 

        switch (choice) {
            case 1:  add_trip(); break;
            case 2:  view_trips(); break;
            case 3:  update_trip(); break;
            case 4:  delete_trip(); break;
            case 5:  search_trips(); break;
            case 6: {
                int tid;
                printf("Enter Trip ID: ");
                scanf("%d", &tid);
                getchar();
                int idx = find_trip_index(tid);
                if (idx == -1)
                    printf("Trip not found.\n");
                else
                    show_seat_map(idx);
                break;
            }
            case 7:  make_reservation(); break;
            case 8:  cancel_reservation(); break;
            case 9:  view_reservations(); break;
            case 10: view_waitlist(); break;
            case 11: print_report(); break;
            case 12: {
                int tid;
                printf("Enter Trip ID to check: ");
                scanf("%d", &tid);
                getchar();
                check_availability_remote("127.0.0.1", DEFAULT_PORT, tid);
                break;
            }
            case 0:
                printf("Goodbye!\n");
                exit(0);
            default:
                printf("Invalid choice, try again.\n");
        }
    }

    return 0;
}
