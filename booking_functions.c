/**
 * booking_functions.c - Reservation management, waitlist, and reports
 *
 * Handles making and cancelling reservations, managing the waitlist,
 * and generating simple summary reports. When a reservation is cancelled,
 * the first person on the waitlist for that trip gets the seat automatically.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include "reservation.h"

/* Global booking storage */
Reservation reservations[MAX_RESERVATIONS];
int reservation_count = 0;
int next_res_id = 1;

WaitlistEntry waitlist[MAX_WAITLIST];
int waitlist_count = 0;

/**
 * Saves all reservations to file.
 * Format: res_count next_res_id on first line,
 * then one reservation per line (semicolon separated).
 */
void save_bookings() {
    char filepath[MAX_STRING];
    build_filepath(filepath, BOOKING_FILE);

    FILE *f = fopen(filepath, "w");
    if (!f) {
        printf("Error saving bookings.\n");
        return;
    }


    fprintf(f, "%d %d\n", reservation_count, next_res_id);
    for (int i = 0; i < reservation_count; i++) {
        Reservation *r = &reservations[i];
        fprintf(f, "%d;%d;%d;%s;%s\n",
                r->id, r->trip_id, r->seat_number,
                r->passenger_name, r->status);
    }

    fclose(f);
}

/**
 * Loads reservations from file.
 */
void load_bookings() {
    char filepath[MAX_STRING];
    build_filepath(filepath, BOOKING_FILE);

    FILE *f = fopen(filepath, "r");
    if (!f) return;

    fscanf(f, "%d %d\n", &reservation_count, &next_res_id);

    for (int i = 0; i < reservation_count; i++) {
        char line[MAX_STRING * 2];
        if (!fgets(line, sizeof(line), f)) break;
        line[strcspn(line, "\n")] = '\0';

        char *tok = strtok(line, ";");
        reservations[i].id = atoi(tok);

        tok = strtok(NULL, ";");
        reservations[i].trip_id = atoi(tok);

        tok = strtok(NULL, ";");
        reservations[i].seat_number = atoi(tok);

        tok = strtok(NULL, ";");
        strcpy(reservations[i].passenger_name, tok);

        tok = strtok(NULL, ";");
        strcpy(reservations[i].status, tok);
    }

    fclose(f);
}

/**
 * Saves the waitlist to file.
 */
void save_waitlist() {
    char filepath[MAX_STRING];
    build_filepath(filepath, WAITLIST_FILE);

    FILE *f = fopen(filepath, "w");
    if (!f) {
        printf("Error saving waitlist.\n");
        return;
    }

    fprintf(f, "%d\n", waitlist_count);
    for (int i = 0; i < waitlist_count; i++) {
        fprintf(f, "%d;%s\n", waitlist[i].trip_id, waitlist[i].passenger_name);
    }
    fclose(f);
}

/**
 * Loads the waitlist from file.
 */
void load_waitlist() {
    char filepath[MAX_STRING];
    build_filepath(filepath, WAITLIST_FILE);

    FILE *f = fopen(filepath, "r");
    if (!f) return;

    fscanf(f, "%d\n", &waitlist_count);
    for (int i = 0; i < waitlist_count; i++) {
        char line[MAX_STRING * 2];
        if (!fgets(line, sizeof(line), f)) break;
        line[strcspn(line, "\n")] = '\0';

        char *tok = strtok(line, ";");
        waitlist[i].trip_id = atoi(tok);

        tok = strtok(NULL, ";");
        strcpy(waitlist[i].passenger_name, tok);
    }
    fclose(f);
}

/**
 * Makes a new reservation.
 * Shows the seat map so the user can pick a seat. If the trip is
 * full, offers to add them to the waitlist instead.
 */
void make_reservation() {
    if (trip_count == 0) {
        printf("No trips available. Add a trip first.\n");
        return;
    }
    if (reservation_count >= MAX_RESERVATIONS) {
        printf("Booking system is full.\n");
        return;
    }

    // show available trips first
    view_trips();

    int trip_id;
    printf("\nEnter Trip ID to book: ");
    scanf("%d", &trip_id);
    getchar();

    int tidx = find_trip_index(trip_id);
    if (tidx == -1) {
        printf("Trip %d not found.\n", trip_id);
        return;
    }

    // check if there are any seats left
    int avail = 0;
    for (int i = 0; i < trips[tidx].total_seats; i++) {
        if (!trips[tidx].seats[i].is_booked) avail++;
    }

    char name[MAX_STRING];
    printf("Passenger name: ");
    fgets(name, MAX_STRING, stdin);
    name[strcspn(name, "\n")] = '\0';

    if (avail == 0) {
        // no seats - offer waitlist
        printf("Sorry, this trip is fully booked.\n");
        printf("Add to waitlist? (y/n): ");
        char ans;
        scanf("%c", &ans);
        getchar();

        if (ans == 'y' || ans == 'Y') {
            if (waitlist_count >= MAX_WAITLIST) {
                printf("Waitlist is full too, sorry.\n");
                return;
            }
            waitlist[waitlist_count].trip_id = trip_id;
            strcpy(waitlist[waitlist_count].passenger_name, name);
            waitlist_count++;
            save_waitlist();
            printf("Added to waitlist for Trip %d.\n", trip_id);
        }
        return;
    }

    // show seat map and let them pick
    show_seat_map(tidx);

    int seat_num;
    printf("Choose seat number: ");
    scanf("%d", &seat_num);
    getchar();

    // validate seat number
    if (seat_num < 1 || seat_num > trips[tidx].total_seats) {
        printf("Invalid seat number.\n");
        return;
    }

    int sidx = seat_num - 1;  // seats are 1-indexed for user
    if (trips[tidx].seats[sidx].is_booked) {
        printf("Seat %d is already taken.\n", seat_num);
        return;
    }

    // book the seat
    trips[tidx].seats[sidx].is_booked = 1;
    strcpy(trips[tidx].seats[sidx].passenger_name, name);

    // create reservation record
    Reservation *r = &reservations[reservation_count];
    r->id = next_res_id++;
    r->trip_id = trip_id;
    r->seat_number = seat_num;
    strcpy(r->passenger_name, name);
    strcpy(r->status, "Confirmed");

    reservation_count++;

    save_trips();
    save_bookings();
    printf("Reservation confirmed! (Booking ID: %d, Seat: %d)\n", r->id, seat_num);
}

/**
 * Cancels a reservation by booking ID.
 * Frees the seat and checks if anyone on the waitlist should get it.
 */
void cancel_reservation() {
    if (reservation_count == 0) {
        printf("No reservations to cancel.\n");
        return;
    }

    int res_id;
    printf("Enter Booking ID to cancel: ");
    scanf("%d", &res_id);
    getchar();

    // find the reservation
    int ridx = -1;
    for (int i = 0; i < reservation_count; i++) {
        if (reservations[i].id == res_id &&
            strcmp(reservations[i].status, "Confirmed") == 0) {
            ridx = i;
            break;
        }
    }

    if (ridx == -1) {
        printf("Active reservation %d not found.\n", res_id);
        return;
    }

    // free the seat on the trip
    int tidx = find_trip_index(reservations[ridx].trip_id);
    if (tidx != -1) {
        int snum = reservations[ridx].seat_number;
        trips[tidx].seats[snum - 1].is_booked = 0;
        trips[tidx].seats[snum - 1].passenger_name[0] = '\0';
    }

    // mark as cancelled
    strcpy(reservations[ridx].status, "Cancelled");

    save_trips();
    save_bookings();
    printf("Reservation %d cancelled.\n", res_id);

    // check waitlist - give the seat to first person waiting for this trip
    if (tidx != -1) {
        int trip_id = reservations[ridx].trip_id;
        int snum = reservations[ridx].seat_number;

        for (int w = 0; w < waitlist_count; w++) {
            if (waitlist[w].trip_id == trip_id) {
                // auto-assign the freed seat to this waitlisted person
                printf("Assigning freed seat %d to waitlisted passenger: %s\n",
                       snum, waitlist[w].passenger_name);

                // book the seat
                trips[tidx].seats[snum - 1].is_booked = 1;
                strcpy(trips[tidx].seats[snum - 1].passenger_name,
                       waitlist[w].passenger_name);

                // create new reservation
                Reservation *nr = &reservations[reservation_count];
                nr->id = next_res_id++;
                nr->trip_id = trip_id;
                nr->seat_number = snum;
                strcpy(nr->passenger_name, waitlist[w].passenger_name);
                strcpy(nr->status, "Confirmed");
                reservation_count++;

                // remove from waitlist by shifting
                for (int j = w; j < waitlist_count - 1; j++) {
                    waitlist[j] = waitlist[j + 1];
                }
                waitlist_count--;

                save_trips();
                save_bookings();
                save_waitlist();
                break;
            }
        }
    }
}

/**
 * Displays all reservations in a table.
 */
void view_reservations() {
    if (reservation_count == 0) {
        printf("No reservations yet.\n");
        return;
    }

    printf("\n%-6s %-8s %-6s %-25s %-12s\n",
           "BkgID", "TripID", "Seat", "Passenger", "Status");
    printf("------ -------- ------ ------------------------- ------------\n");

    for (int i = 0; i < reservation_count; i++) {
        printf("%-6d %-8d %-6d %-25.25s %-12s\n",
               reservations[i].id, reservations[i].trip_id,
               reservations[i].seat_number,
               reservations[i].passenger_name, reservations[i].status);
    }
}

/**
 * Shows everyone currently on the waitlist.
 */
void view_waitlist() {
    if (waitlist_count == 0) {
        printf("Waitlist is empty.\n");
        return;
    }

    printf("\n%-4s %-8s %-30s\n", "#", "TripID", "Passenger");
    printf("---- -------- ------------------------------\n");

    for (int i = 0; i < waitlist_count; i++) {
        printf("%-4d %-8d %-30s\n",
               i + 1, waitlist[i].trip_id, waitlist[i].passenger_name);
    }
}

/**
 * Prints a summary report showing:
 * - Total trips, total reservations
 * - For each trip: how many seats booked vs available, revenue
 */
void print_report() {
    printf("\n============= BOOKING REPORT =============\n\n");
    printf("Total trips: %d\n", trip_count);
    printf("Total reservations: %d\n", reservation_count);
    printf("Waitlist entries: %d\n\n", waitlist_count);

    if (trip_count == 0) {
        printf("No trips to report on.\n");
        return;
    }

    float total_revenue = 0;

    printf("%-4s %-6s %-15s %-15s %-8s %-8s %-10s\n",
           "ID", "Type", "Origin", "Dest", "Booked", "Avail", "Revenue");
    printf("---- ------ --------------- --------------- -------- -------- ----------\n");

    for (int i = 0; i < trip_count; i++) {
        int booked = 0;
        for (int s = 0; s < trips[i].total_seats; s++) {
            if (trips[i].seats[s].is_booked) booked++;
        }
        int avail = trips[i].total_seats - booked;
        float revenue = booked * trips[i].price;
        total_revenue += revenue;

        printf("%-4d %-6s %-15.15s %-15.15s %-8d %-8d $%-9.2f\n",
               trips[i].id, trips[i].type, trips[i].origin,
               trips[i].destination, booked, avail, revenue);
    }

    printf("\nTotal estimated revenue: $%.2f\n", total_revenue);
    printf("==========================================\n");
}
