#include "TwoDCraft.h"

#define WINDOW_X 0
#define WINDOW_Y 0
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 512

#define WORLD_SIZE 10
#define WORLD_DIM 512

SDL_Texture *block_textures[256]; 

int selected_block = 1; 

struct Player{
    int x;
    int y;
    int size;
};
typedef struct Player Player;

// Function to save the world grid to a JSON file
void save_world_to_json(int world[WORLD_SIZE][WORLD_SIZE], const char *filename) {
    json_t *root = json_array();  // Create a root JSON array

    // Iterate through the world array and add each row as a JSON array
    for (int i = 0; i < WORLD_SIZE; i++) {
        json_t *row = json_array();  // Create a row JSON array
        for (int j = 0; j < WORLD_SIZE; j++) {
            json_array_append_new(row, json_integer(world[i][j]));  // Add each block value as an integer
        }
        json_array_append_new(root, row);  // Add the row to the root array
    }

    // Open the file for writing
    FILE *file = fopen(filename, "w");
    if (file) {
        // Serialize the JSON object to the file
        if (json_dumpf(root, file, JSON_INDENT(4)) != 0) {
            fprintf(stderr, "ERROR: Failed to write JSON to file\n");
        }
        fclose(file);  // Close the file
    } else {
        fprintf(stderr, "ERROR: Unable to open file for saving\n");
    }

    json_decref(root);  // Decrement the reference count for the root object
}

// Function to load the world grid from a JSON file
void load_world_from_json(int world[WORLD_SIZE][WORLD_SIZE], const char *filename) {
    // Open the file for reading
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "ERROR: Unable to open file for loading\n");
        return;
    }

    // Parse the JSON file into a root JSON object
    json_error_t error;
    json_t *root = json_loadf(file, 0, &error);
    fclose(file);

    if (!root) {
        fprintf(stderr, "ERROR: Failed to load JSON: %s\n", error.text);
        return;
    }

    // Ensure the root is an array
    if (!json_is_array(root)) {
        fprintf(stderr, "ERROR: JSON root is not an array\n");
        json_decref(root);
        return;
    }

    // Iterate through the root array and load each row
    for (int i = 0; i < WORLD_SIZE; i++) {
        json_t *row = json_array_get(root, i);
        if (!json_is_array(row)) {
            fprintf(stderr, "ERROR: Row %d is not an array\n", i);
            continue;
        }

        for (int j = 0; j < WORLD_SIZE; j++) {
            json_t *value = json_array_get(row, j);
            if (json_is_integer(value)) {
                world[i][j] = (int)json_integer_value(value);  // Load block value
            } else {
                world[i][j] = 0;  // Default value if the cell is invalid
            }
        }
    }

    json_decref(root);  // Decrement the reference count for the root object
}


void render_selected_block(SDL_Renderer *renderer) {
    int cell_size = WORLD_DIM / WORLD_SIZE;
    SDL_Rect rect = {10, WINDOW_HEIGHT - cell_size - 10, cell_size, cell_size}; // Posizione in basso a sinistra

    if (block_textures[selected_block]) {
        SDL_RenderCopy(renderer, block_textures[selected_block], NULL, &rect);
    }
}

void load_block_textures(SDL_Renderer *renderer) {
    block_textures[0] = NULL;
    block_textures[1] = IMG_LoadTexture(renderer, "textures/grass.png");
    if (!block_textures[1]) {
        fprintf(stderr, "ERROR: Unable to load 'grass.png': %s\n", SDL_GetError());
    }

    block_textures[2] = IMG_LoadTexture(renderer, "textures/stone.png");
    if (!block_textures[2]) {
        fprintf(stderr, "ERROR: Unable to load 'stone.png': %s\n", SDL_GetError());
    }
}

void place_block(Player *player, SDL_MouseButtonEvent mouse, int world[WORLD_SIZE][WORLD_SIZE]) {
    int cell_size = WORLD_DIM / WORLD_SIZE;

    // Convert mouse position to grid coordinates
    int grid_x = mouse.x / cell_size;
    int grid_y = mouse.y / cell_size;

    // Check that the player is within a certain radius (2 blocks away in this case)
    int player_grid_x = player->x / cell_size;
    int player_grid_y = player->y / cell_size;

    int distance_x = abs(grid_x - player_grid_x);
    int distance_y = abs(grid_y - player_grid_y);

    if (distance_x <= 2 && distance_y <= 2) {
        // Place or replace the block at the clicked position
        if (grid_x >= 0 && grid_x < WORLD_SIZE && grid_y >= 0 && grid_y < WORLD_SIZE) {
            world[grid_y][grid_x] = selected_block; // Replace the block (or place if empty)
        }
    }
}


void init_player(Player *player) {
    player->x = WORLD_DIM / 2; // Start near the center of the world
    player->y = WORLD_DIM / 2;
    player->size = WORLD_DIM / WORLD_SIZE; // Size matches one grid cell
}

void handle_player_input(Player *player, SDL_Event event) {
    switch (event.key.keysym.sym) {
        case SDLK_w: 
            if (player->y > 0) player->y -= player->size; 
            break; // Move up
        case SDLK_s: 
            if (player->y + player->size < WORLD_DIM - 1) player->y += player->size; 
            break; // Move down
        case SDLK_a: 
            if (player->x > 0) player->x -= player->size; 
            break; // Move left
        case SDLK_d: 
            if (player->x + player->size < WORLD_DIM - 1) player->x += player->size; 
            break; // Move right
    }
}

void render_player(SDL_Renderer *renderer, Player *player) {
    SDL_Rect rect = {player->x, player->y, player->size, player->size};
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 255); // Red color for the player
    SDL_RenderFillRect(renderer, &rect);
}

void render_world(SDL_Renderer *renderer, int world[WORLD_SIZE][WORLD_SIZE]) {
    SDL_SetRenderDrawColor(renderer, 0x55, 0x55, 0x55, 255); // Colore delle celle
    int cell_size = WORLD_DIM / WORLD_SIZE;
    SDL_Rect cell;
    cell.w = cell_size;
    cell.h = cell_size;

    for (int i = 0; i < WORLD_SIZE; i++) {
        for (int j = 0; j < WORLD_SIZE; j++) {
            cell.x = j * cell_size;
            cell.y = i * cell_size;

            if (world[i][j] == 0) {
                SDL_RenderDrawRect(renderer, &cell); // Disegna griglia vuota
            } else if (block_textures[world[i][j]]) {
                SDL_RenderCopy(renderer, block_textures[world[i][j]], NULL, &cell); // Disegna il blocco
            }

        }
    }
}

int main() {
    SDL_Window *window;
    SDL_Renderer *renderer;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: SDL_Init could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "ERROR: SDL_image could not initialize! IMG_Init: %s\n", IMG_GetError());
        return -1;
    }

    window = SDL_CreateWindow(
        "TwoDCraft",
        WINDOW_X,
        WINDOW_Y,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );

    if (!window) {
        fprintf(stderr, "ERROR: SDL_CreateWindow failed! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        fprintf(stderr, "ERROR: SDL_CreateRenderer failed! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    Player player;
    init_player(&player);

    bool quit = false;
    SDL_Event event;
    int world[WORLD_SIZE][WORLD_SIZE] = {0};

    // Load world from JSON file
    load_world_from_json(world, "world.json");

    load_block_textures(renderer);

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        place_block(&player, event.button, world);
                    }
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        quit = true;
                    } else if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9) {
                        selected_block = event.key.keysym.sym - SDLK_1 + 1; // Change selected block
                    } else {
                        handle_player_input(&player, event);
                    }
                    break;
            }
        }
        SDL_RenderClear(renderer);
        render_world(renderer, world);
        render_selected_block(renderer);
        render_player(renderer, &player);
        SDL_RenderPresent(renderer);
    }

    // Save the world to JSON when quitting
    save_world_to_json(world, "world.json");

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    IMG_Quit();

    return 0;
}

