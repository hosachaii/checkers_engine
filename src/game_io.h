#define UNDO 34
#define REDO 35
#define HINT 36
#define QUIT 37

//game modes available
typedef enum {
    TWO_PLAYER, //1 vs 1
    AI_PLAYER //1 vs the computer
} game_mode_t;

void clear_screen();
void print_player(player_t player);
void get_move(board_t *board, unsigned short *from_index, unsigned short *to_index, fj_array *fj, player_t player);
unsigned short get_index(void);
void inform_fj(fj_array *fj);
void inform_minimax_move(board_t *prev, board_t *next, player_t player);
void inform_ai_move(board_t *board, board_t *ai_move);