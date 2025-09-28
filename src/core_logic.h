#define TRUE 1
#define FALSE 0

#define LAST_INDEX 31
#define INVALID 33

//represents the state of a square on board
typedef enum {
	EMPTY = 0, //empty squre
	PLAYER_1, //=1
	PLAYER_2 //=2
}player_t;

unsigned short interpreter(unsigned short col, unsigned short row);
unsigned short get_row(unsigned short index);
unsigned short get_col(unsigned short index);
bool is_valid_move(board_t *b, player_t player, unsigned short from_index, unsigned short to_index, fj_array *fj);
void move_piece(board_t *b, unsigned short from_index, unsigned short to_index, fj_array *fj, player_t player);
bool forward_moving_pieces(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares);
bool reverse_moving_pieces(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares);
unsigned short is_game_over(board_t *b, player_t player);
void switch_player(player_t *player);
unsigned short move_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
void funeral(board_t *board, game_history *history, fj_array *fj, unsigned short game_state);