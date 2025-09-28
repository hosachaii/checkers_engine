// stack node structure for storing board states.
typedef struct board_state {
    board_t board; //captures all the player positions
    player_t player_turn; //which player's turn
    struct board_state *next; //pointer to next board_state in a linked list
}board_state;

// two stacks using singly linked list to store board states
// while enabling undo-redo functionality
typedef struct game_history {
	board_state *undo_top; //pointer to the top of undo stack
	board_state *redo_top; //pointer to top of redo stack
}game_history;

void init_board_state(board_state *bs);
bool is_undo_empty(game_history *history);
bool is_redo_empty(game_history *history);
void save_progress(board_t *board, game_history *history, player_t current_player);
bool undo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
bool redo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
void free_redo(game_history *history);
void free_undo(game_history *history);