
#ifndef GAME_STATE_H
#define GAME_STATE_H

enum game_state_t
{
    DEBUG,
    PLAYING,
    TRANSITION_TO_PLAYING,
    MAIN_MENU,
    PAUSED,
    QUIT
};

extern game_state_t state;

#endif