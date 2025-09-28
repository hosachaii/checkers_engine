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
}fj_array;

bool is_fj_empty(fj_array *fj);
void init_fj_array(fj_array *fj);
void fj_append(fj_array *fj, unsigned short from_index, unsigned short capture_index, unsigned short to_index);
void find_forward_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj);
void find_reverse_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj);
void are_forced_jumps(board_t *b, fj_array *fj, player_t player);
void prune_fj(fj_array *fj, unsigned short from_index);