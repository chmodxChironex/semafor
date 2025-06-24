# proj2 – Skibus Synchronization Simulation (IOS Project 2024)

This C project simulates a skibus system using POSIX semaphores and shared memory. It's inspired by the Senate Bus Problem from *The Little Book of Semaphores*.

## How to Build

```bash
make
```

## How to Run

Run the simulation with 5 parameters:

```bash
./proj2 <L> <Z> <K> <TL> <TB>
```

Where:

- `L` – number of skiers (1–20000)
- `Z` – number of stops (1–10)
- `K` – bus capacity (10–100)
- `TL` – max time (µs) a skier waits before arriving (0–10000)
- `TB` – max bus travel time between stops (µs) (0–1000)

Example:

```bash
./proj2 8 4 10 4 5
```

## Output Format

The program prints actions (with unique numbering) to `proj2.out`. Actions include skiers starting, arriving, boarding, skiing, and bus events like arriving/leaving/final.

**Note:** The output **will not** always match the exact order in the assignment example. This is expected. The simulation focuses on correct synchronization and logic, not strict output order (except for specific events like `BUS: leaving final` and `BUS: finish`, which must happen last).

## Running the Test Script

To validate the output format, run:

```bash
./proj2 8 4 10 4 5 | bash test.sh
```

This script checks consistency of printed events (e.g., all skiers board and finish, exactly one BUS starts and finishes, etc.).

## Project Purpose

This project demonstrates:
- Safe process synchronization without deadlocks or race conditions.
- Correct use of semaphores and shared memory.
- Inter-process communication and resource cleanup.

## Notes

- Designed for Linux/WSL environments.
- Project received **15/15 points**.
- Author: Martin Klíma, FIT VUT, 2024.

