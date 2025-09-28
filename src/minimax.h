// Depth defined for the minimax algorithm
#define DEPTH 7

short evaluate_board(board_t *b, player_t player, unsigned short game_result);
short minimax(board_t *b, unsigned short depth, short alpha, short beta, player_t player, board_t *best_move);