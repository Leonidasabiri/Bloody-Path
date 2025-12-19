#include "renderer.h"
#include "config.h"
#include "tinyutils.h"






static void renderPlayer(SDL_Renderer* renderer, Player* player) {
    if (!player) return;
    SDL_Rect playerRect = { (int)player->x, (int)player->y, (int)player->width, (int)player->height };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red placeholder
    SDL_RenderFillRect(renderer, &playerRect);
}


// void renderFrame(GameRenderer gameRenderer, Map* map, Player* player) {

//     // Render game objects
//     renderMap(gameRenderer, map);
//     // renderPlayer(gameRenderer.renderer, player);

//     // Present the frame
//     SDL_RenderPresent(gameRenderer.renderer);
// }

// void destroyRenderer(GameRenderer* gameRenderer) {
//     if (!gameRenderer) return;
//     if (gameRenderer->renderer) {
//         SDL_DestroyRenderer(gameRenderer->renderer);
//     }
//     if (gameRenderer->window) {
//         SDL_DestroyWindow(gameRenderer->window);
//     }
//     free(gameRenderer);
// }
