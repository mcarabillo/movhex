#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#define MAX_AIR_ROUTES 5
#define MAX_NEIGHBORS 6
#define OFFSET_ODD -1
#define INVALID_COORDS -99
#define INVALID_COST -1

// Structs
typedef struct{
    int x, y;
} OffsetCoords;

typedef struct{
    int q, r, s;
} CubeCoords;

typedef struct{
    int to_x; // x cella di arrivo
    int to_y; // y cella di arrivo
    int air_route_cost; // Costo della rotta aerea
} AirRoute;

typedef struct{
    int cost; // Da 0 a 100, inizializzato a 1
    int air_routes_number; // Numero di rotte aeree (massimo 5 per cella)
    AirRoute *air_routes_arr; // Array di rotte aeree
} Cell;

typedef struct{
    OffsetCoords coords;
    int cost;
} Node;

typedef struct{
    Node *nodes;
    int size;
    int max_capacity;
} PriorityQueue;

// Variabili e costanti globali
int width, height;

Cell **map = NULL;

const CubeCoords neighbors_dir[MAX_NEIGHBORS] = {
    {1, -1, 0}, {1, 0, -1}, {0, 1, -1},
    {-1, 1, 0}, {-1, 0, 1}, {0, -1, 1}
};

// Funzioni ausiliarie
inline bool is_cell_valid(int x, int y) {
    return (map != NULL && x >= 0 && x < width && y >= 0 && y < height);
}

inline CubeCoords cube_add(CubeCoords a, CubeCoords b) {
    return (CubeCoords){a.q + b.q, a.r + b.r, a.s + b.s};
}

inline CubeCoords cube_sub(CubeCoords a, CubeCoords b) {
    return (CubeCoords){a.q - b.q, a.r - b.r, a.s - b.s};
}

inline int cube_len(CubeCoords h) {
    assert(h.q + h.r + h.s == 0);
    return ceil(((abs(h.q) + abs(h.r) + abs(h.s)) / 2.0));
}

inline int cube_distance(CubeCoords end, CubeCoords start) {
    return cube_len(cube_sub(end, start));
}

inline OffsetCoords offset_from_cube(int offset, CubeCoords hex) {
    int col = hex.q + (int)((hex.r + offset * (hex.r % 2)) / 2);
    int row = hex.r;

    return (OffsetCoords){col, row};
}

inline CubeCoords cube_from_offset(int offset, OffsetCoords hex) {
    int q = hex.x - (int)((hex.y + offset * (hex.y % 2)) / 2);
    int r = hex.y;
    int s = -q - r;

    return (CubeCoords){q, r, s};
}

inline int get_index(int x, int y) {
    return y * width + x;
}

inline bool pq_is_empty(PriorityQueue *pq) {
    return pq->size == 0;
}

PriorityQueue *pq_init(int max_capacity) {
    PriorityQueue *pq = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    pq->nodes = (Node *)malloc(max_capacity * sizeof(Node));
    pq->size = 0;
    pq->max_capacity = max_capacity;
    return pq;
}

void pq_enqueue(PriorityQueue *pq, Node node) {
    if(pq->size < pq->max_capacity) {
        pq->nodes[pq->size] = node;
        pq->size++;
    }
}

Node *pq_dequeue(PriorityQueue *pq) {
    if(pq->size == 0) return NULL;

    // Trova il nodo con la priorità più alta (costo più basso)
    int min_index = 0;
    for(int i = 1; i < pq->size; i++) {
        if(pq->nodes[i].cost < pq->nodes[min_index].cost) {
            min_index = i;
        }
    }

    // Estrai il nodo con la priorità più alta
    Node *result = &pq->nodes[min_index];

    // Sposta l'ultimo nodo nella posizione del nodo estratto
    pq->nodes[min_index] = pq->nodes[pq->size - 1];
    pq->size--;

    return result;
}

OffsetCoords find_neighbor(int dir, CubeCoords hex) {
    CubeCoords delta = neighbors_dir[dir];
    CubeCoords neighbor_coords_cube = cube_add(hex, delta);
    OffsetCoords neighbor_coords_offset = offset_from_cube(OFFSET_ODD, neighbor_coords_cube);

    if(!is_cell_valid(neighbor_coords_offset.x, neighbor_coords_offset.y)) return (OffsetCoords){INVALID_COORDS, INVALID_COORDS};

    return neighbor_coords_offset;
}

int get_neighbors_number(OffsetCoords hex, OffsetCoords *neighbors_array) {
    int count = 0;
    CubeCoords hex_coords_cube = cube_from_offset(OFFSET_ODD, hex);

    for(int i = 0; i < MAX_NEIGHBORS; i++) {
        OffsetCoords neighbor_coords = find_neighbor(i, hex_coords_cube);

        if(neighbor_coords.x == INVALID_COORDS) continue;

        neighbors_array[count] = neighbor_coords;
        count++;
    }

    return count;
}

void clean_all() {
    if(map != NULL) {
        // Free array rotte aeree
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                if(map[x][y].air_routes_arr != NULL) {
                    free(map[x][y].air_routes_arr);
                    map[x][y].air_routes_arr = NULL;
                }
            }
        }

        // Free mappa
        free(map[0]);
        free(map);
        map = NULL;
    }
}

void update_ar_cost(int x, int y) {
    if(!is_cell_valid(x, y)) return;
    
    Cell *cell = &map[x][y];
    if(cell->air_routes_arr == NULL) return;
    
    for(int i = 0; i < MAX_AIR_ROUTES; i++) {
        AirRoute *route = &cell->air_routes_arr[i];
        if(route->to_x != INVALID_COORDS){
            route->air_route_cost = cell->cost;
        }
    }
}

int dijkstra(OffsetCoords start, OffsetCoords end) {
    if(start.x == end.x && start.y == end.y) return 0;
    if(!is_cell_valid(start.x, start.y) || !is_cell_valid(end.x, end.y)) return INVALID_COST;

    PriorityQueue *frontier = pq_init(width * height);
    if(frontier == NULL) {
        printf("Priority Queue allocation error in dijkstra\n");
        exit(EXIT_FAILURE);
    }

    pq_enqueue(frontier, (Node){start, 0});

    int *cost_so_far = (int *)malloc(width * height * sizeof(int));

    int idx = get_index(start.x, start.y);
    cost_so_far[idx] = 0;
    int result = INVALID_COST;
    bool found = false;

    while(!pq_is_empty(frontier) && found == false) {
        Node current = *pq_dequeue(frontier);
        Cell *current_ptr = &map[current.coords.x][current.coords.y];

        if(current.coords.x == end.x && current.coords.y == end.y) break; // Controllo se ho raggiunto la destinazione
        else if(current_ptr->air_routes_number > 0){ // Controllo se ci sono rotte aeree
            for(int i = 0; i < current_ptr->air_routes_number; i++){
                AirRoute *current_route = &current_ptr->air_routes_arr[i];
                if(current_route->to_x == end.x && current_route->to_y == end.y){
                    result = cost_so_far[get_index(current.coords.x, current.coords.y)] + current_route->air_route_cost;
                    found = true;
                    break;
                }
            }
        }
        else{ // Inizio a guardare i vicini
            
        }  
    }

    return result;
}

// Funzioni principali
void init(int cols, int rows) {
    if(rows <= 0 || cols <= 0) {
        printf("KO\n");
        return;
    }

    if(map != NULL)
        clean_all();

    width = cols;   // numero di colonne
    height = rows;  // numero di righe
    
    map = (Cell **)malloc(width * sizeof(Cell *));
    if(map == NULL) {
        printf("map allocation error in init\n");
        exit(EXIT_FAILURE);
    }

    // Alloca un singolo array contiguo per tutte le celle
    map[0] = (Cell *)malloc(width * height * sizeof(Cell));
    if(map[0] == NULL) {
        printf("map allocation error in init\n");
        exit(EXIT_FAILURE);
    }

    // Imposta i puntatori per ogni colonna
    for(int x = 1; x < width; x++) {
        map[x] = map[0] + x * height;  // Ogni colonna ha 'height' elementi
    }

    // Inizializza tutte le celle
    for(int x = 0; x < width; x++){ // x = colonna
        for(int y = 0; y < height; y++){ // y = riga
            Cell *cell = &map[x][y];
            cell->air_routes_arr = NULL;
            cell->air_routes_number = 0;
            cell->cost = 1;
        }
    }

    printf("OK\n");
}

void change_cost(int x, int y, int v, int radius) {
    if(!is_cell_valid(x, y) || abs(v) > 10 || radius <= 0) {
        printf("KO\n");
        return;
    }

    CubeCoords center_cube = cube_from_offset(OFFSET_ODD, (OffsetCoords){x, y}); // Trovo le coordinate cubiche del centro

    for(int q = center_cube.q - radius; q <= center_cube.q + radius; q++) {
        for(int r = center_cube.r - radius; r <= center_cube.r + radius; r++) {
            int s = -q - r;
            CubeCoords current_cube = {q, r, s};
            OffsetCoords current_offset = offset_from_cube(OFFSET_ODD, current_cube); // Converto in offset per controllarne la validità
            if(!is_cell_valid(current_offset.x, current_offset.y))
                continue;
            
            int distance = cube_distance(current_cube, center_cube);
            if(distance > radius)
                continue;

            Cell *cell = &map[current_offset.x][current_offset.y];
            double cost_coefficient = ((double)radius - (double)cube_distance(current_cube, center_cube)) / (double)radius;
            cell->cost += (int)floor(v * fmax(0.0, cost_coefficient));

            if(cell->cost < 0)
                cell->cost = 0;
            if(cell->cost > 100)
                cell->cost = 100;

            update_ar_cost(current_offset.x, current_offset.y);
        }
    }

    printf("OK\n");
}

void toggle_air_routes(int x1, int y1, int x2, int y2) {
    if(!is_cell_valid(x1, y1) || !is_cell_valid(x2, y2) || (x1 == x2 && y1 == y2)) {
        printf("KO\n");
        return;
    }

    Cell *start = &map[x1][y1];
    if(start->air_routes_number == MAX_AIR_ROUTES) {
        printf("KO\n");
        return;
    }

    // Inizializzazione array rotte
    if(start->air_routes_arr == NULL){
        start->air_routes_arr = (AirRoute *)malloc(MAX_AIR_ROUTES * sizeof(AirRoute));

        for(int i = 0; i < MAX_AIR_ROUTES; i++){
            start->air_routes_arr[i].to_x = INVALID_COORDS;
            start->air_routes_arr[i].to_y = INVALID_COORDS;
            start->air_routes_arr[i].air_route_cost = 0; // Costo 0 = rotta non attiva
        }
    }

    int found_idx = -1;
    int first_empty_idx = -1;

    // Navigazione preliminare array rotte
    for(int i = 0; i < MAX_AIR_ROUTES; i++) {
        AirRoute *route = &start->air_routes_arr[i];
        if(route->to_x == x2 && route->to_y == y2) {
            found_idx = i; // Rotta trovata
            break;
        }
        if(route->to_x == INVALID_COORDS && first_empty_idx == -1) {
            first_empty_idx = i; // Prima posizione libera trovata
        }
    }

    if(found_idx != -1){ // Rimozione rotta
        AirRoute *route = &start->air_routes_arr[found_idx]; 
        route->to_x = INVALID_COORDS;
        route->to_y = INVALID_COORDS;
        route->air_route_cost = 0;
        start->air_routes_number--;

        printf("OK\n");
        return;
        
    }
    else if(first_empty_idx != -1){ // Aggiunta rotta
        AirRoute *route = &start->air_routes_arr[first_empty_idx];
        route->to_x = x2;
        route->to_y = y2;
        route->air_route_cost = start->cost;
        start->air_routes_number++;

        printf("OK\n");
        return;
    }
    else printf("KO\n");
}

int travel_cost(int xp, int yp, int xd, int yd) {
    if(!is_cell_valid(xp, yp) || !is_cell_valid(xd, yd))
        return INVALID_COST;

    if(xp == xd && yp == yd)
        return 0;

    return astar((OffsetCoords){xp, yp}, (OffsetCoords){xd, yd});
}

// Funzione MAIN
int main(int argc, char *argv[]) {
    // Lettura istruzione
    char *buffer;
    size_t bufsize = 100;
    size_t length;

    buffer = (char *)malloc(bufsize * sizeof(char));
    if(buffer == NULL)
        exit(EXIT_FAILURE);
    
    while((length = getline(&buffer, &bufsize, stdin)) != EOF){

        if(length > 0 && buffer[length - 1] == '\n')
            buffer[length - 1] = '\0';

        char *cmd = strtok(buffer, " ");
        if(cmd == NULL)
            continue;

        if(strcmp(cmd, "init") == 0){
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            if(param1 != NULL && param2 != NULL){
                int cols = atoi(param1);
                int rows = atoi(param2);
                init(cols, rows);
            }
            else printf("KO\n");
        }
        else if(strcmp(cmd, "change_cost") == 0){
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            char *param3 = strtok(NULL, " ");
            char *param4 = strtok(NULL, " ");
            if(param1 != NULL && param2 != NULL && param3 != NULL && param4 != NULL){
                int x = atoi(param1);
                int y = atoi(param2);
                int v = atoi(param3);
                int radius = atoi(param4);
                change_cost(x, y, v, radius);
            }
            else printf("KO\n");
        }
        else if(strcmp(cmd, "toggle_air_route") == 0){
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            char *param3 = strtok(NULL, " ");
            char *param4 = strtok(NULL, " ");
            if(param1 != NULL && param2 != NULL && param3 != NULL && param4 != NULL){
                int x1 = atoi(param1);
                int y1 = atoi(param2);
                int x2 = atoi(param3);
                int y2 = atoi(param4);
                toggle_air_routes(x1, y1, x2, y2);
            }
            else printf("KO\n");
        }
        else if(strcmp(cmd, "travel_cost") == 0){
            char *param1 = strtok(NULL, " ");
            char *param2 = strtok(NULL, " ");
            char *param3 = strtok(NULL, " ");
            char *param4 = strtok(NULL, " ");
            if(param1 != NULL && param2 != NULL && param3 != NULL && param4 != NULL){
                int xp = atoi(param1);
                int yp = atoi(param2);
                int xd = atoi(param3);
                int yd = atoi(param4);
                int cost = travel_cost(xp, yp, xd, yd);
                printf("%d\n", cost);
            }
            else printf("-1\n");
        }
        else{
            printf("No command %s\n", buffer);
        }
    }

    free(buffer);
    return 0;
}
