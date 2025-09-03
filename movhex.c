#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <time.h>

#define MAX_AIR_ROUTES 5
#define MAX_NEIGHBORS 6
#define OFFSET_ODD -1
#define INVALID_COORDS -99
#define INVALID_COST -1
#define CACHE_MISS -99
#define CACHE_SIZE 5563

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
    int cost; // Distanza
} Node;

typedef struct{
    Node *nodes;
    int size;
    int max_capacity;
} PriorityQueue;

typedef struct{
    int xp, yp, xd, yd;
    int cost;
    bool valid;
} CacheEntry;

// Variabili e costanti globali
int width, height;

Cell **map = NULL;

CacheEntry *cache = NULL;

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

inline bool pq_is_empty(PriorityQueue *pq) {
    return pq->size == 0;
}

inline unsigned int hash_function(int x1, int y1, int x2, int y2) {
    unsigned int hash = 17;
    hash = hash * 31 + (unsigned int)x1;
    hash = hash * 31 + (unsigned int)y1;
    hash = hash * 31 + (unsigned int)x2;
    hash = hash * 31 + (unsigned int)y2;
    return hash % CACHE_SIZE;
}

inline void invalidate_cache() {
    if(cache == NULL) return;

    for(int i = 0; i < CACHE_SIZE; i++) {
        cache[i].valid = false;
    }
}

inline int cache_search(int x1, int y1, int x2, int y2) {
    if(cache == NULL) return CACHE_MISS;

    unsigned int key = hash_function(x1, y1, x2, y2);
    CacheEntry *entry = &cache[key];

    if(entry->valid == true && entry->xp == x1 && entry->yp == y1 && entry->xd == x2 && entry->yd == y2) {
        return entry->cost;
    }

    return CACHE_MISS;
}

inline void cache_insert(int x1, int y1, int x2, int y2, int cost) {
    assert(cache != NULL);

    unsigned int key = hash_function(x1, y1, x2, y2);
    CacheEntry *entry = &cache[key];

    entry->xp = x1;
    entry->yp = y1;
    entry->xd = x2;
    entry->yd = y2;
    entry->cost = cost;
    entry->valid = true;
}

inline void heapify_up(PriorityQueue *pq, int idx) {
    while(idx > 0){
        int parent = (idx - 1) / 2;
        if(pq->nodes[idx].cost >= pq->nodes[parent].cost) break;

        Node temp = pq->nodes[idx];
        pq->nodes[idx] = pq->nodes[parent];
        pq->nodes[parent] = temp;

        idx = parent;
    }
}

inline void heapify_down(PriorityQueue *pq, int idx) {
    int left, right, smallest;
    while(true){
        left = 2 * idx + 1;
        right = 2 * idx + 2;
        smallest = idx;

        if(left < pq->size && pq->nodes[left].cost < pq->nodes[smallest].cost)
            smallest = left;
        if(right < pq->size && pq->nodes[right].cost < pq->nodes[smallest].cost)
            smallest = right;
        if(smallest == idx) break;

        Node temp = pq->nodes[idx];
        pq->nodes[idx] = pq->nodes[smallest];
        pq->nodes[smallest] = temp;

        idx = smallest;
    }
}

inline void pq_enqueue(PriorityQueue *pq, Node node) {
    if(pq->size < pq->max_capacity) {
        pq->nodes[pq->size] = node;
        pq->size++;
        heapify_up(pq, pq->size - 1);
    }
}

inline Node pq_dequeue(PriorityQueue *pq) {
    assert(pq->size > 0);
    
    Node minimum = pq->nodes[0];
    pq->nodes[0] = pq->nodes[pq->size - 1];
    pq->size--;
    heapify_down(pq, 0);
    return minimum;
}

PriorityQueue *pq_init(int max_capacity) {
    PriorityQueue *pq = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    pq->nodes = (Node *)malloc(max_capacity * sizeof(Node));
    pq->size = 0;
    pq->max_capacity = max_capacity;
    return pq;
}

void pq_free(PriorityQueue *pq) {
    if(pq != NULL) {
        free(pq->nodes);
        free(pq);
    }
}

CacheEntry *cache_init() {
    CacheEntry *cache = (CacheEntry *)malloc(CACHE_SIZE * sizeof(CacheEntry));
    if(cache == NULL) {
        printf("cache allocation error in cache_init\n");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < CACHE_SIZE; i++) {
        cache[i].valid = false;
    }

    return cache;
}

OffsetCoords find_ground_neighbor(int dir, CubeCoords hex) {
    CubeCoords delta = neighbors_dir[dir];
    CubeCoords ground_neighbor_coords_cube = cube_add(hex, delta);
    OffsetCoords ground_neighbor_coords_offset = offset_from_cube(OFFSET_ODD, ground_neighbor_coords_cube);

    if(!is_cell_valid(ground_neighbor_coords_offset.x, ground_neighbor_coords_offset.y)) return (OffsetCoords){INVALID_COORDS, INVALID_COORDS};

    return ground_neighbor_coords_offset;
}

int get_all_neighbors(OffsetCoords hex, OffsetCoords *neighbors_array, int *costs_array) { // Considero come vicini anche gli esagoni raggiungibili tramite rotta aerea
    int count = 0;
    CubeCoords hex_coords_cube = cube_from_offset(OFFSET_ODD, hex);

    // Vicini terrestri
    for(int i = 0; i < MAX_NEIGHBORS; i++) {
        OffsetCoords ground_neighbor_coords = find_ground_neighbor(i, hex_coords_cube);

        if(ground_neighbor_coords.x == INVALID_COORDS) continue;

        neighbors_array[count] = ground_neighbor_coords;
        costs_array[count] = map[hex.x][hex.y].cost;
        count++;
    }

    // Vicini aerei
    Cell *current_cell = &map[hex.x][hex.y];

    if(current_cell->air_routes_arr == NULL) return count;

    for(int i = 0; i < MAX_AIR_ROUTES; i++){
        AirRoute *route = &current_cell->air_routes_arr[i];
        if(route->to_x != INVALID_COORDS){
            neighbors_array[count] = (OffsetCoords){route->to_x, route->to_y};
            costs_array[count] = route->air_route_cost;
            count++;
        }
    }

    return count; // Ritorno il numero di vicini, non l'array
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
            int new_cost = cell->cost;
            route->air_route_cost = new_cost;
        }
    }
}

int dijkstra(OffsetCoords start, OffsetCoords end) {
    PriorityQueue *frontier = pq_init(width * height);

    bool **visited = (bool **)malloc(width * sizeof(bool *));
    int **distances = (int **)malloc(width * sizeof(int *));

    for(int x = 0; x < width; x++){
        visited[x] = (bool *)malloc(height * sizeof(bool));
        distances[x] = (int *)malloc(height * sizeof(int));
        for(int y = 0; y < height; y++){
            visited[x][y] = false;
            distances[x][y] = INT32_MAX;
        }
    }

    Node start_node = {start, 0};
    pq_enqueue(frontier, start_node);
    distances[start.x][start.y] = 0;

    int result = INVALID_COST;

    while(!pq_is_empty(frontier)){
        Node current = pq_dequeue(frontier);
        OffsetCoords current_position = current.coords;
        int current_cost = current.cost;

        // Controllo se ho già visitato la cella quando il costo era migliore
        if(visited[current_position.x][current_position.y] == true) continue;
        // Se no, la visito
        visited[current_position.x][current_position.y] = true;
        // Controllo se ho raggiunto la destinazione
        if(current_position.x == end.x && current_position.y == end.y){
            result = current_cost;
            break;
        }
        // Controllo se posso uscire dalla cella
        if(map[current_position.x][current_position.y].cost == 0) continue;

        // Se non è la destinazione, esploro i vicini. Considero "vicino" anche una cella raggiungibile tramite rotta aerea 
        OffsetCoords neighbors[MAX_NEIGHBORS + MAX_AIR_ROUTES];
        int neighbor_cost[MAX_NEIGHBORS + MAX_AIR_ROUTES];
        int neighbor_count = get_all_neighbors(current_position, neighbors, neighbor_cost);

        for(int i = 0; i < neighbor_count; i++){
            OffsetCoords neighbor = neighbors[i];

            if(visited[neighbor.x][neighbor.y] == true) continue;

            int new_distance = current_cost + neighbor_cost[i];

            if(new_distance < distances[neighbor.x][neighbor.y]){
                distances[neighbor.x][neighbor.y] = new_distance;
                Node neighbor_node = {neighbor, new_distance};
                pq_enqueue(frontier, neighbor_node);
            }
        }
    }

    // Pulizia
    for(int x = 0; x < width; x++){
        free(visited[x]);
        free(distances[x]);
    }
    free(visited);
    free(distances);
    pq_free(frontier);

    return result;
}

// Funzioni principali
void init(int cols, int rows) {
    if(rows <= 0 || cols <= 0) {
        printf("KO\n");
        return;
    }

    if(map != NULL){
        clean_all();
    }

    if(cache == NULL){
        cache = cache_init();
    }
    else{
        invalidate_cache();
    }

    width = cols;   
    height = rows;  
    
    map = (Cell **)malloc(width * sizeof(Cell *));
    if(map == NULL) {
        printf("map allocation error in init\n");
        exit(EXIT_FAILURE);
    }

    map[0] = (Cell *)malloc(width * height * sizeof(Cell));
    if(map[0] == NULL) {
        printf("map allocation error in init\n");
        exit(EXIT_FAILURE);
    }

    for(int x = 1; x < width; x++) {
        map[x] = map[0] + x * height;
    }

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

    invalidate_cache();

    CubeCoords center_cube = cube_from_offset(OFFSET_ODD, (OffsetCoords){x, y}); // Trovo le coordinate cubiche del centro

    for(int q = center_cube.q - radius; q <= center_cube.q + radius; q++) {
        for(int r = center_cube.r - radius; r <= center_cube.r + radius; r++) {
            int s = -q - r;
            CubeCoords current_node_cube = {q, r, s};
            OffsetCoords current_node_offset = offset_from_cube(OFFSET_ODD, current_node_cube); // Converto in offset per controllarne la validità        
            if(!is_cell_valid(current_node_offset.x, current_node_offset.y)) continue;
            
            int distance = cube_distance(current_node_cube, center_cube);
            if(distance >= radius) continue;

            Cell *cell = &map[current_node_offset.x][current_node_offset.y];
            double cost_coefficient = ((double)radius - (double)cube_distance(current_node_cube, center_cube)) / (double)radius;
            cell->cost += (int)floor(v * fmax(0.0, cost_coefficient));

            if(cell->cost < 0){
                cell->cost = 0;
            }
            if(cell->cost > 100){
                cell->cost = 100;
            }

            update_ar_cost(current_node_offset.x, current_node_offset.y);
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

        invalidate_cache();

        printf("OK\n");
        return;
        
    }
    else if(first_empty_idx != -1){ // Aggiunta rotta
        AirRoute *route = &start->air_routes_arr[first_empty_idx];
        Cell *cell = &map[x1][y1];
        route->to_x = x2;
        route->to_y = y2;
        route->air_route_cost = cell->cost;
        start->air_routes_number++;

        invalidate_cache();

        printf("OK\n");
        return;
    }
    else printf("KO\n");
}

int travel_cost(int xp, int yp, int xd, int yd) {
    if(!is_cell_valid(xp, yp) || !is_cell_valid(xd, yd)) return INVALID_COST;
    if(xp == xd && yp == yd) return 0;
    if(map[xp][yp].cost == 0) return INVALID_COST;

    if(cache != NULL){
        int cached_cost = cache_search(xp, yp, xd, yd);
        if(cached_cost != CACHE_MISS){
            // printf("Cache hit\n");
            return cached_cost;
        }
    }

    int cost = dijkstra((OffsetCoords){xp, yp}, (OffsetCoords){xd, yd});
    if(cache != NULL){
        cache_insert(xp, yp, xd, yd, cost);
    }

    return cost;
}

// Funzione MAIN
int main(int argc, char *argv[]) {
    // Lettura istruzione
    char *buffer;
    size_t bufsize = 100;
    size_t length;

    // time_t start_time, end_time;
    // start_time = time(NULL);

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

    if(cache != NULL){
        free(cache);
        cache = NULL;
    }

    clean_all();

    // end_time = time(NULL);
    // printf("Execution time: %ld seconds\n", end_time - start_time);

    return 0;
}
