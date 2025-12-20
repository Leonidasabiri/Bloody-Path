#include "SDL.h"
#include <stdio.h>
#include "config.h"
#include "renderer.h"
#include <vector>

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    GameRenderer* gameRenderer = initRenderer("Tileset Test - Click tiles to select, ESC to exit", SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!gameRenderer) {
        SDL_Quit();
        return 1;
    }

    printf("\n=== INTERACTIVE TILESET TEST MODE ===\n");
    printf("This will display all tiles from Dungeon_Tileset.png\n");
    printf("CLICK on tiles to select them (they'll turn yellow)\n");
    printf("Selected tiles will be printed with their indices\n");
    printf("Tile index = (row * columns) + column\n");
    printf("Example: Tile at row 2, column 3 in a 10-column tileset = (2 * 10) + 3 = 23\n");
    printf("Press ESC to exit and see your final selection.\n\n");

    // Track selected tiles
    std::vector<int> selectedTiles;
    int selectionOrder = 1;
    
    const int TILE_SIZE = 16;
    const int tileDisplaySize = 40;
    const int padding = 4;
    const int tilesPerRow = 20;

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            }
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    running = false;
                }
            }
            if (ev.type == SDL_MOUSEBUTTONDOWN) {
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    int mouseX = ev.button.x;
                    int mouseY = ev.button.y;
                    
                    // Calculate which tile was clicked
                    int totalTiles = gameRenderer->tilesetColumns * gameRenderer->tilesetRows;
                    
                    for (int i = 0; i < totalTiles; i++) {
                        int displayX = (i % tilesPerRow) * (tileDisplaySize + padding) + padding;
                        int displayY = (i / tilesPerRow) * (tileDisplaySize + padding) + padding + 30;
                        
                        // Check if click is within this tile's bounds
                        if (mouseX >= displayX && mouseX < displayX + tileDisplaySize &&
                            mouseY >= displayY && mouseY < displayY + tileDisplaySize) {
                            
                            // Check if already selected
                            bool alreadySelected = false;
                            for (size_t j = 0; j < selectedTiles.size(); j++) {
                                if (selectedTiles[j] == i) {
                                    alreadySelected = true;
                                    // Remove from selection
                                    selectedTiles.erase(selectedTiles.begin() + j);
                                    printf("Deselected tile %d\n", i);
                                    break;
                                }
                            }
                            
                            if (!alreadySelected) {
                                selectedTiles.push_back(i);
                                printf("Selected #%d: tile %d (row %d, col %d)\n", 
                                       selectionOrder++, i, 
                                       i / gameRenderer->tilesetColumns, 
                                       i % gameRenderer->tilesetColumns);
                            }
                            break;
                        }
                    }
                }
            }
        }

        // Render tileset with selection highlights
        SDL_SetRenderDrawColor(gameRenderer->renderer, 40, 40, 50, 255);
        SDL_RenderClear(gameRenderer->renderer);

        int totalTiles = gameRenderer->tilesetColumns * gameRenderer->tilesetRows;
        
        for (int i = 0; i < totalTiles; i++) {
            int srcX = (i % gameRenderer->tilesetColumns) * TILE_SIZE;
            int srcY = (i / gameRenderer->tilesetColumns) * TILE_SIZE;
            
            int displayX = (i % tilesPerRow) * (tileDisplaySize + padding) + padding;
            int displayY = (i / tilesPerRow) * (tileDisplaySize + padding) + padding + 30;
            
            SDL_Rect srcRect = { srcX, srcY, TILE_SIZE, TILE_SIZE };
            SDL_Rect destRect = { displayX, displayY, tileDisplaySize, tileDisplaySize };
            
            // Check if this tile is selected
            bool isSelected = false;
            for (size_t j = 0; j < selectedTiles.size(); j++) {
                if (selectedTiles[j] == i) {
                    isSelected = true;
                    break;
                }
            }
            
            // Draw border (yellow if selected, white otherwise)
            if (isSelected) {
                SDL_SetRenderDrawColor(gameRenderer->renderer, 255, 255, 0, 255); // Yellow
                SDL_Rect highlightRect = { displayX - 2, displayY - 2, tileDisplaySize + 4, tileDisplaySize + 4 };
                SDL_RenderFillRect(gameRenderer->renderer, &highlightRect);
            }
            
            SDL_SetRenderDrawColor(gameRenderer->renderer, 255, 255, 255, 255);
            SDL_Rect borderRect = { displayX - 1, displayY - 1, tileDisplaySize + 2, tileDisplaySize + 2 };
            SDL_RenderDrawRect(gameRenderer->renderer, &borderRect);
            
            // Render the tile
            SDL_RenderCopy(gameRenderer->renderer, gameRenderer->tilesetTexture, &srcRect, &destRect);
        }
        
        SDL_RenderPresent(gameRenderer->renderer);
        SDL_Delay(16);
    }

    // Print final selection summary
    printf("\n=== FINAL SELECTION ===\n");
    printf("You selected %zu tiles:\n", selectedTiles.size());
    for (size_t i = 0; i < selectedTiles.size(); i++) {
        printf("Selection #%zu: tile index %d (row %d, col %d)\n", 
               i + 1, selectedTiles[i],
               selectedTiles[i] / gameRenderer->tilesetColumns,
               selectedTiles[i] % gameRenderer->tilesetColumns);
    }
    printf("\nNow tell me what each selected tile should be used for!\n");
    printf("Example format:\n");
    printf("tile 0 = left wall\n");
    printf("tile 5 = right wall\n");
    printf("tile 10 = platform\n");
    printf("etc...\n");

    destroyRenderer(gameRenderer);
    SDL_Quit();

    return 0;
}

tile 0 (row 0, col 0) = left wall
Selected #7: tile 20 (row 2, col 0) = left wall 
Selected #8: tile 40 (row 4, col 0) = left down corner
Selected #9: tile 1 (row 0, col 1) = platform varian 1
Selected #10: tile 2 (row 0, col 2) = platform varian 2
Selected #11: tile 3 (row 0, col 3) = platform varian 3
Selected #12: tile 4 (row 0, col 4) = platform varian 4
Selected #13: tile 5 (row 0, col 5) = right wall
Selected #16: tile 45 (row 4, col 5) = down left corenrs
Selected #17: tile 44 (row 4, col 4) = down wall varian 1
Selected #18: tile 43 (row 4, col 3) = down wall varian 2
Selected #21: tile 50 (row 5, col 0) = top left corner
Selected #22: tile 51 (row 5, col 1) = top wall varian 1
Selected #23: tile 52 (row 5, col 2) = top wall varian 2
Selected #24: tile 53 (row 5, col 3) = top right corner