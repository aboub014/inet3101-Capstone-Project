/**
 * Air/Train/Bus Reservation System - Header File
 * INET 3101 Capstone Project
 *
 * Contains struct definitions, constants, and function declarations
 * for the reservation management system.
 */

#ifndef RESERVATION_H
#define RESERVATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Constants ---- */
#define MAX_TRIPS 50
#define MAX_SEATS 120
#define MAX_RESERVATIONS 500
#define MAX_WAITLIST 100
#define MAX_STRING 256
#define TRIP_FILE "trips.txt"
#define BOOKING_FILE "bookings.txt"
#define WAITLIST_FILE "waitlist.txt"
#define DEFAULT_PORT 8080

/**
 * Seat - represents one seat on a trip
 * is_booked: 0 = free, 1 = taken
 */
typedef struct {
    int seat_number;
    int is_booked;
    char passenger_name[MAX_STRING];
} Seat;

/**
 * Trip - a single scheduled trip (bus, train, or flight)
 * holds its own seat map as an array of Seat structs
 */
typedef struct {
    int id;
    char type[20];              // "Bus", "Train", or "Air"
    char origin[MAX_STRING];
    char destination[MAX_STRING];
    char date[MAX_STRING];      // e.g. "2026-04-15"
    char time[MAX_STRING];      // e.g. "08:30"
    float price;
    int total_seats;
    Seat seats[MAX_SEATS];
} Trip;

/**
 * Reservation - a booking made by a passenger
 * status can be "Confirmed" or "Cancelled"
 */
typedef struct {
    int id;
    int trip_id;
    int seat_number;
    char passenger_name[MAX_STRING];
    char status[20];            // "Confirmed" or "Cancelled"
} Reservation;

/**
 * WaitlistEntry - passenger waiting for a seat on a full trip
 */
typedef struct {
    int trip_id;
    char passenger_name[MAX_STRING];
} WaitlistEntry;

/* ---- Global data arrays ---- */
extern Trip trips[MAX_TRIPS];
extern int trip_count;
extern int next_trip_id;

extern Reservation reservations[MAX_RESERVATIONS];
extern int reservation_count;
extern int next_res_id;

extern WaitlistEntry waitlist[MAX_WAITLIST];
extern int waitlist_count;

extern char data_dir[MAX_STRING];   // set by --datadir flag

/* ---- Trip functions (trip_functions.c) ---- */
void save_trips();
void load_trips();
void add_trip();
void view_trips();
void update_trip();
void delete_trip();
void search_trips();
int find_trip_index(int id);
void show_seat_map(int trip_index);

/* ---- Booking functions (booking_functions.c) ---- */
void save_bookings();
void load_bookings();
void save_waitlist();
void load_waitlist();
void make_reservation();
void cancel_reservation();
void view_reservations();
void view_waitlist();
void print_report();

/* ---- Network functions (network.c) ---- */
void start_server(int port);
void check_availability_remote(const char *host, int port, int trip_id);

/* ---- Utility ---- */
void build_filepath(char *dest, const char *filename);

#endif
