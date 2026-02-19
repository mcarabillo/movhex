# Movhex: Hexagonal Grid Pathfinding

**Movhex** is a C-based pathfinding engine designed for hexagonal grids. It implements efficient path calculation using Dijkstra's algorithm, supporting dynamic terrain costs and special "air route" connections between non-adjacent cells.

This project is part of the final assessment for the BSc in Computer Science and Engineering at Politecnico di Milano, Milan, Italy.

## Features

- **Hexagonal Grid System**: Uses an odd-row offset coordinate system for map representation.
- **Pathfinding**: Implements Dijkstra's algorithm to find the minimum travel cost between cells.
- **Dynamic Terrain**: Supports changing the traversal cost of cells within a specified radius.
- **Air Routes**: Allows the creation of direct links (teleports/flights) between arbitrary cells, which are integrated into the pathfinding graph.
- **Optimization**: Includes a custom Priority Queue implementation and a simple hash-based caching system to speed up repeated queries.

## Compilation

The project includes a `Makefile` for easy compilation. It uses `gcc` with AddressSanitizer enabled for development safety.

To build the project:

```bash
make movhex
```

This will produce the `movhex` executable:

```bash
gcc -Wall -Werror -std=gnu11 -g3 -O2 -fsanitize=address movhex.c -lm -o movhex
```

## Usage

The program reads commands from standard input (`stdin`). You can run it interactively or pipe commands to it.

### Commands

- **Initialize Map**
  ```text
  init <cols> <rows>
  ```
  Initializes a grid of size `cols` x `rows`. All cells start with a default cost of 1.
  - Returns: `OK` or `KO`

- **Calculate Travel Cost**
  ```text
  travel_cost <x_start> <y_start> <x_end> <y_end>
  ```
  Calculates the minimum cost to travel from the start coordinates to the end coordinates.
  - Returns: The integer cost, or `-1` if no path exists/error.

- **Change Terrain Cost**
  ```text
  change_cost <x> <y> <value> <radius>
  ```
  Modifies the cost of the center cell `(x, y)` and its neighbors within `radius`. The `value` is added to the existing cost (clamped between 0 and 100).
  - Returns: `OK` or `KO`

- **Toggle Air Route**
  ```text
  toggle_air_route <x1> <y1> <x2> <y2>
  ```
  Creates or removes a one-way air route from `(x1, y1)` to `(x2, y2)`.
  - If a route exists, it is removed.
  - If no route exists, it is added with a travel cost equal to the destination's current cost.
  - Returns: `OK` or `KO`

### Example

```bash
./movhex
init 10 10
travel_cost 0 0 1 1
change_cost 0 0 5 1
travel_cost 0 0 1 1
```

## Technical Details

- **Coordinates**: Internally converts between Offset coordinates (for storage/API) and Cube coordinates (for geometric calculations like distance).
- **Caching**: A hash map caches query results (`xp, yp, xd, yd`) to return instant answers for repeated queries on a static map. The cache is invalidated whenever the map is modified (costs changed or routes toggled).
- **Memory Management**: The `init` command automatically cleans up previous map allocations, allowing for multiple test cases in a single run.
