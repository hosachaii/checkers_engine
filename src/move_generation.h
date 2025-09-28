// Maximum size of the move table.
#define MT_MAX_SIZE 32

// Scores for each piece.
#define PAWN_VALUE 5
#define KING_VALUE 8
#define CENTER_BONUS 3

// Bitmap of all the center squares of the checkers board.
#define CENTER_SQ 8289792

typedef struct move_table {
	board_t *children;
	unsigned int size;
	unsigned int capacity;
}move_table;

void init_mt(move_table *mt);
void expand_mt_if_full(move_table *mt);
void generate_moves(board_t *b, move_table *mt, fj_array *fj, player_t player);
void generate_fj_moves(board_t *b, fj_array *fj, move_table *mt, player_t player);
board_t move_fj(board_t b, forced_jump *jump, player_t player);
void append_forward_moves(board_t *board, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player);
void append_reverse_moves(board_t *board, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player);