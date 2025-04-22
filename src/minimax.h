// Maximum size of the move table.
#define MT_MAX_SIZE 32

// Depth defined for the minimax algorithm
#define DEPTH 7

// Scores for each piece.
#define PAWN_VALUE 5
#define KING_VALUE 8
#define CENTER_BONUS 3

// Bitmap of all the center squares of the checkers board.
#define CENTER_SQ 8289792

typedef struct move_table {
	bitboard_t *children;
	unsigned int size;
	unsigned int capacity;
}move_table;

void init_mt(move_table *mt);
void expand_mt_if_full(move_table *mt);
bitboard_t move_fj(bitboard_t b, forced_jump *jump, player_t player);
bitboard_t generate_bitboard(bitboard_t b, unsigned short from_index, unsigned short to_index, player_t player);
void append_forward_moves(bitboard_t *bitboard, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player);
void append_reverse_moves(bitboard_t *bitboard, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player);
void generate_fj_moves(bitboard_t *b, fj_array *fj, move_table *mt, player_t player);
void generate_moves(bitboard_t *b, move_table *mt, fj_array *fj, player_t player);
short minimax(bitboard_t *b, unsigned short depth, short alpha, short beta, player_t current_player, bitboard_t *best_move);
void inform_minimax_move(bitboard_t *prev, bitboard_t *next, player_t player);
void inform_ai_move(bitboard_t *bitboard, bitboard_t *ai_move);
void hint(bitboard_t *b);
void AI_player(player_t starting_player);
