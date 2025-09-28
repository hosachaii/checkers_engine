/*#define TRUE 1
#define FALSE 0

#define MAX_FORCED_JUMPS 10

typedef struct forced_jump {
	unsigned short from_index;
	unsigned short capture_index;
	unsigned short to_index;
}forced_jump;

typedef struct fj_array {
	forced_jump *jumps;
	short size;
	short index;
}fj_array;*/

unsigned short get_row(unsigned short index);
unsigned short get_col(unsigned short index);
bool is_valid_move(board_t *board, player_t player, unsigned short from_index, unsigned short to_index, fj_array *fj);
void move_piece(board_t *board, unsigned short from_index, unsigned short to_index, fj_array *fj);
void kinging(board_t *board, unsigned short index);
void init_fj_array(fj_array *fj);
bool is_fj_empty(fj_array *fj);
void fj_append(fj_array *fj, unsigned short from_index, unsigned short capture_index, unsigned short to_index);
void find_forward_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj);
void find_reverse_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj);
void are_forced_jumps(bitboard_t *b, fj_array *fj, player_t player);
void prune_fj(fj_array *fj, unsigned short from_index);
