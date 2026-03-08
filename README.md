# Air/Train/Bus Reservation System

INET 3101 — Capstone Project

---

## Problem Statement

Travelers often need a convenient way to search for trips, check seat availability, and book seats on buses, trains, or flights — all from one place. Existing systems are often fragmented and complicated. This project implements a simple, unified reservation system in C that lets users manage trips, make bookings, view seat maps, handle waitlists, and even check availability over a network connection. It stores everything in plain text files so data persists between sessions.

## Design and Architecture Details

The application is structured as a multi-file C project, following the same general pattern as the sample library management code provided in class.

### Data Structures

The core structs are:

- **Trip**: Holds trip info (type, origin, destination, date, time, price) and contains a built-in array of `Seat` structs — this acts as the **SeatMap ADT**. Each seat tracks whether it's booked and who booked it.
- **Reservation**: Represents a confirmed or cancelled booking, linking a passenger to a specific trip and seat number.
- **WaitlistEntry**: A simple struct for passengers waiting on a full trip. Stored as a FIFO list (first come, first served).

### File Organization

| File | Purpose |
|------|---------|
| `reservation.h` | All struct definitions, constants, and function declarations |
| `main.c` | CLI argument parsing, main menu loop, utility functions |
| `trip_functions.c` | Trip CRUD operations, file I/O for trips, seat map display |
| `booking_functions.c` | Reservation CRUD, waitlist management, summary reports |
| `network.c` | TCP socket server/client for remote seat availability checks |
| `Makefile` | Build instructions |

### Key Design Decisions

- **Flat-file persistence**: Data is stored in semicolon-delimited text files (`trips.txt`, `bookings.txt`, `waitlist.txt`). This keeps things simple and easy to debug — you can open the files and see exactly what's stored.
- **File locking**: Used `flock()` when writing to trip and booking files so that if the server mode and CLI are both running, they don't corrupt data at the same time.
- **Seat map as part of Trip struct**: Instead of a separate SeatMap structure, seats are embedded in the Trip. This makes it straightforward to save/load everything together.
- **Automatic waitlist promotion**: When a reservation is cancelled, the system automatically assigns the freed seat to the first waitlisted passenger for that trip.
- **Socket protocol**: The network layer uses a basic text-based protocol over TCP. The server supports `CHECK`, `HOLD`, and `QUIT` commands. Kept it intentionally simple.
- **CLI flags**: The `--datadir`, `--server`, `--port`, `--check`, and `--host` flags give flexibility in how you run the program (normal mode, server mode, or remote check mode).

### ADTs Used

- **SeatMap**: The `Seat` array inside each `Trip` — functions like `show_seat_map()` operate on this to display and manage seating.
- **Itinerary/Trip List**: The global `trips[]` array with helper functions for CRUD, search, and indexing by ID.
- **Waitlist (FIFO Queue)**: Implemented as a simple array that shifts elements forward when someone is promoted off the list.

## Pros and Cons of Your Solution

### Pros

- **Simple and readable**: The code follows a clear, consistent pattern. Someone familiar with the sample library code should be able to follow it easily.
- **Full CRUD**: Trips and reservations both support create, read, update, and delete.
- **Data persists**: All data is saved to files and loaded back on startup, so nothing is lost when you close the program.
- **Seat map visualization**: The visual seat map makes it easy to see which seats are taken at a glance.
- **Waitlist works automatically**: Cancelling a reservation promotes the next person in line without any extra steps.
- **Network feature**: The socket server/client adds a real networking component — you can check and hold seats remotely.
- **File locking**: Prevents data corruption when running server and CLI at the same time.

### Cons

- **No database**: Uses flat files instead of a proper database (like gdbm). This works fine for small amounts of data but wouldn't scale well.
- **Fixed array sizes**: Everything uses fixed-size arrays (`MAX_TRIPS`, `MAX_SEATS`, etc.). A real system would use dynamic memory allocation or linked lists.
- **Single-threaded server**: The server handles one client at a time. Multiple simultaneous connections would have to wait.
- **Basic search**: Search is just a substring match. No support for filtering by multiple criteria at once (like origin AND date).
- **No authentication**: Anyone can book or cancel any reservation. A real system would need user accounts and access control.
- **Limited input validation**: While there's some error checking, the system trusts user input more than it probably should (e.g., no date format validation).

### Future Improvements

- Switch to a database backend (gdbm or SQLite) for better data management
- Use dynamic memory allocation instead of fixed arrays
- Add multi-threaded server support with `pthread`
- Implement user authentication
- Add more detailed reports (revenue by date range, occupancy rates, etc.)
