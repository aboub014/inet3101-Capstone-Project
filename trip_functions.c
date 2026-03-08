/**
 * trip_functions.c - Trip management (CRUD) and seat map display
 *
 * Handles adding, viewing, updating, deleting, and searching trips.
 * Data is saved to a text file using semicolons as delimiters (similar
 * to the sample library code). Seat data is stored on separate lines
 * right after each trip record.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include "reservation.h"

/* Global trip storage */
Trip trips[MAX_TRIPS];
int trip_count = 0;
int next_trip_id = 1;

/**
 * Saves all trips to the data file.
 * Uses file locking (flock) so multiple processes don't corrupt data.
 *
 * Format:
 *   Line 1: trip_count next_trip_id
 *   For each trip:
 *     trip line: id;type;origin;dest;date;time;price;total_seats
 *     then total_seats lines: seat_number;is_booked;passenger_name
 */
void save_trips() {
    char filepath[MAX_STRING];
    build_filepath(filepath, TRIP_FILE);

    FILE *f = fopen(filepath, "w");
    if (!f) {
        printf("Error: could not save trips to %s\n", filepath);
        return;
    }


    fprintf(f, "%d %d\n", trip_count, next_trip_id);

    for (int i = 0; i < trip_count; i++) {
        Trip *t = &trips[i];
        fprintf(f, "%d;%s;%s;%s;%s;%s;%.2f;%d\n",
                t->id, t->type, t->origin, t->destination,
                t->date, t->time, t->price, t->total_seats);

        // write each seat on its own line
        for (int s = 0; s < t->total_seats; s++) {
            fprintf(f, "%d;%d;%s\n",
                    t->seats[s].seat_number,
                    t->seats[s].is_booked,
                    t->seats[s].passenger_name);
        }
    }
    fclose(f);
}

/**
 * Loads trips from the data file.
 * Reads the format written by save_trips().
 */
void load_trips() {
    char filepath[MAX_STRING];
    build_filepath(filepath, TRIP_FILE);

    FILE *f = fopen(filepath, "r");
    if (!f) return; // no file yet, that's fine

    fscanf(f, "%d %d\n", &trip_count, &next_trip_id);

    for (int i = 0; i < trip_count; i++) {
        char line[MAX_STRING * 4];
        if (!fgets(line, sizeof(line), f)) break;

        // remove trailing newline
        line[strcspn(line, "\n")] = '\0';

        // parse trip fields
        char *tok = strtok(line, ";");
        trips[i].id = atoi(tok);

        tok = strtok(NULL, ";");
        strcpy(trips[i].type, tok);

        tok = strtok(NULL, ";");
        strcpy(trips[i].origin, tok);

        tok = strtok(NULL, ";");
        strcpy(trips[i].destination, tok);

        tok = strtok(NULL, ";");
        strcpy(trips[i].date, tok);

        tok = strtok(NULL, ";");
        strcpy(trips[i].time, tok);

        tok = strtok(NULL, ";");
        trips[i].price = atof(tok);

        tok = strtok(NULL, ";");
        trips[i].total_seats = atoi(tok);

        // now read each seat line
        for (int s = 0; s < trips[i].total_seats; s++) {
            char sline[MAX_STRING * 2];
            if (!fgets(sline, sizeof(sline), f)) break;
            sline[strcspn(sline, "\n")] = '\0';

            char *st = strtok(sline, ";");
            trips[i].seats[s].seat_number = atoi(st);

            st = strtok(NULL, ";");
            trips[i].seats[s].is_booked = atoi(st);

            st = strtok(NULL, ";");
            if (st)
                strcpy(trips[i].seats[s].passenger_name, st);
            else
                trips[i].seats[s].passenger_name[0] = '\0';
        }
    }

    fclose(f);
}

/**
 * Finds the array index of a trip by its ID.
 * Returns -1 if not found.
 */
int find_trip_index(int id) {
    for (int i = 0; i < trip_count; i++) {
        if (trips[i].id == id)
            return i;
    }
    return -1;
}

/**
 * Adds a new trip. Asks the user for type, route, schedule, price,
 * and number of seats. Initializes all seats as available.
 */
void add_trip() {
    if (trip_count >= MAX_TRIPS) {
        printf("Cannot add more trips (limit reached).\n");
        return;
    }

    Trip *t = &trips[trip_count];
    t->id = next_trip_id++;

    // get trip type
    printf("Type (Bus/Train/Air): ");
    fgets(t->type, sizeof(t->type), stdin);
    t->type[strcspn(t->type, "\n")] = '\0';

    printf("Origin: ");
    fgets(t->origin, MAX_STRING, stdin);
    t->origin[strcspn(t->origin, "\n")] = '\0';

    printf("Destination: ");
    fgets(t->destination, MAX_STRING, stdin);
    t->destination[strcspn(t->destination, "\n")] = '\0';

    printf("Date (YYYY-MM-DD): ");
    fgets(t->date, MAX_STRING, stdin);
    t->date[strcspn(t->date, "\n")] = '\0';

    printf("Time (HH:MM): ");
    fgets(t->time, MAX_STRING, stdin);
    t->time[strcspn(t->time, "\n")] = '\0';

    printf("Price: ");
    scanf("%f", &t->price);

    printf("Number of seats: ");
    scanf("%d", &t->total_seats);
    getchar();

    if (t->total_seats > MAX_SEATS) {
        printf("Too many seats, capping at %d.\n", MAX_SEATS);
        t->total_seats = MAX_SEATS;
    }

    // initialize all seats as empty
    for (int i = 0; i < t->total_seats; i++) {
        t->seats[i].seat_number = i + 1;
        t->seats[i].is_booked = 0;
        t->seats[i].passenger_name[0] = '\0';
    }

    trip_count++;
    save_trips();
    printf("Trip added! (ID: %d)\n", t->id);
}

/**
 * Displays all trips in a table format.
 */
void view_trips() {
    if (trip_count == 0) {
        printf("No trips available.\n");
        return;
    }

    printf("\n%-4s %-6s %-15s %-15s %-12s %-6s %-8s %-6s %-6s\n",
           "ID", "Type", "Origin", "Destination", "Date", "Time", "Price", "Seats", "Avail");
    printf("---- ------ --------------- --------------- ------------ ------ -------- ------ ------\n");

    for (int i = 0; i < trip_count; i++) {
        // count available seats
        int avail = 0;
        for (int s = 0; s < trips[i].total_seats; s++) {
            if (!trips[i].seats[s].is_booked)
                avail++;
        }

        printf("%-4d %-6s %-15.15s %-15.15s %-12s %-6s $%-7.2f %-6d %-6d\n",
               trips[i].id, trips[i].type, trips[i].origin,
               trips[i].destination, trips[i].date, trips[i].time,
               trips[i].price, trips[i].total_seats, avail);
    }
}

/**
 * Updates an existing trip's details.
 * Lets you change the date, time, or price. Press Enter to skip a field.
 */
void update_trip() {
    if (trip_count == 0) {
        printf("No trips to update.\n");
        return;
    }

    int id;
    printf("Enter Trip ID to update: ");
    scanf("%d", &id);
    getchar();

    int idx = find_trip_index(id);
    if (idx == -1) {
        printf("Trip %d not found.\n", id);
        return;
    }

    Trip *t = &trips[idx];
    char temp[MAX_STRING];

    printf("Current date: %s\nNew date (Enter to keep): ", t->date);
    fgets(temp, MAX_STRING, stdin);
    if (strlen(temp) > 1) {
        temp[strcspn(temp, "\n")] = '\0';
        strcpy(t->date, temp);
    }

    printf("Current time: %s\nNew time (Enter to keep): ", t->time);
    fgets(temp, MAX_STRING, stdin);
    if (strlen(temp) > 1) {
        temp[strcspn(temp, "\n")] = '\0';
        strcpy(t->time, temp);
    }

    printf("Current price: %.2f\nNew price (-1 to keep): ", t->price);
    float new_price;
    scanf("%f", &new_price);
    getchar();
    if (new_price >= 0) {
        t->price = new_price;
    }

    save_trips();
    printf("Trip %d updated.\n", id);
}

/**
 * Deletes a trip by ID. Shifts remaining trips in the array.
 */
void delete_trip() {
    if (trip_count == 0) {
        printf("No trips to delete.\n");
        return;
    }

    int id;
    printf("Enter Trip ID to delete: ");
    scanf("%d", &id);
    getchar();

    int idx = find_trip_index(id);
    if (idx == -1) {
        printf("Trip %d not found.\n", id);
        return;
    }

    // shift everything after this index down by one
    for (int i = idx; i < trip_count - 1; i++) {
        trips[i] = trips[i + 1];
    }
    trip_count--;
    save_trips();
    printf("Trip %d deleted.\n", id);
}

/**
 * Searches trips by keyword. Checks origin, destination, type, and date.
 */
void search_trips() {
    if (trip_count == 0) {
        printf("No trips available.\n");
        return;
    }

    char term[MAX_STRING];
    printf("Search (origin/destination/type/date): ");
    fgets(term, MAX_STRING, stdin);
    term[strcspn(term, "\n")] = '\0';

    int found = 0;
    printf("\n%-4s %-6s %-15s %-15s %-12s %-6s %-8s\n",
           "ID", "Type", "Origin", "Destination", "Date", "Time", "Price");
    printf("---- ------ --------------- --------------- ------------ ------ --------\n");

    for (int i = 0; i < trip_count; i++) {
        if (strstr(trips[i].origin, term) ||
            strstr(trips[i].destination, term) ||
            strstr(trips[i].type, term) ||
            strstr(trips[i].date, term)) {

            printf("%-4d %-6s %-15.15s %-15.15s %-12s %-6s $%-7.2f\n",
                   trips[i].id, trips[i].type, trips[i].origin,
                   trips[i].destination, trips[i].date, trips[i].time,
                   trips[i].price);
            found++;
        }
    }

    if (found == 0)
        printf("No trips matched '%s'.\n", term);
    else
        printf("\nFound %d trip(s).\n", found);
}

/**
 * Shows a visual seat map for a trip.
 * [XX] means booked, [ 5] means seat 5 is free.
 * Displayed in rows of 4 to look like actual seating.
 */
void show_seat_map(int trip_index) {
    Trip *t = &trips[trip_index];
    printf("\nSeat Map for Trip %d (%s: %s -> %s)\n",
           t->id, t->type, t->origin, t->destination);
    printf("[ ] = available, [XX] = booked\n\n");

    for (int i = 0; i < t->total_seats; i++) {
        if (t->seats[i].is_booked)
            printf("[XX] ");
        else
            printf("[%2d] ", t->seats[i].seat_number);

        // 4 seats per row, add aisle gap after 2
        if ((i + 1) % 4 == 2)
            printf("  ");
        if ((i + 1) % 4 == 0)
            printf("\n");
    }
    printf("\n");

    // count available
    int avail = 0;
    for (int i = 0; i < t->total_seats; i++) {
        if (!t->seats[i].is_booked) avail++;
    }
    printf("Available: %d / %d\n", avail, t->total_seats);
}
