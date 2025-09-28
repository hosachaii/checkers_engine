#define BOARD_SIZE 32
#define P1_PAWNS 4095
#define P2_PAWNS 4293918720

#define TRUE 1
#define FALSE 0

#define LAST_INDEX 31
#define INVALID 33
#define UNDO 34
#define REDO 35
#define HINT 36
#define QUIT 37

// Maximum size of the move table.
#define MT_MAX_SIZE 32

// Scores for each piece.
#define PAWN_VALUE 5
#define KING_VALUE 8
#define CENTER_BONUS 3

// Bitmap of all the center squares of the checkers board.
#define CENTER_SQ 8289792

#define MAX_FORCED_JUMPS 10

// Depth defined for the minimax algorithm
#define DEPTH 7

//represents the state of a square on board
typedef enum {
	EMPTY = 0, //empty squre
	PLAYER_1, //=1
	PLAYER_2 //=2
}player_t;

//game modes available
typedef enum {
    TWO_PLAYER, //1 vs 1
    AI_PLAYER //1 vs the computer
} game_mode_t;

//bitboard representation of the game
typedef struct board_t {
	uint32_t player1_pawns; //32 bit unsigned int representing positions of PLAYER_1 pawns
	uint32_t player1_kings; //32 bit unsigned int representing positions of PLAYER_1 kings
	uint32_t player2_pawns; //32 bit unsigned int representing positions of PLAYER_2 pawns
	uint32_t player2_kings; //32 bit unsigned int representing positions of PLAYER_2 kings
}board_t;

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

typedef struct move_table {
	board_t *children;
	unsigned int size;
	unsigned int capacity;
}move_table;

void init_board(board_t *board, game_history *history);
void display_board(board_t *board);
void init_board_state(board_state *bs);
bool is_undo_empty(game_history *history);
bool is_redo_empty(game_history *history);
void save_progress(board_t *board, game_history *history, player_t current_player);
bool undo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
bool redo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
void free_redo(game_history *history);
void free_undo(game_history *history);
unsigned short get_row(unsigned short index);
unsigned short get_col(unsigned short index);
unsigned short interpreter(unsigned short col, unsigned short row);
bool is_valid_move(board_t *b, player_t player, unsigned short from_index, unsigned short to_index, fj_array *fj);
void move_piece(board_t *b, unsigned short from_index, unsigned short to_index, fj_array *fj, player_t player);
bool forward_moving_pieces(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares);
bool reverse_moving_pieces(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares);
unsigned short is_game_over(board_t *b, player_t player);
bool is_fj_empty(fj_array *fj);
void init_fj_array(fj_array *fj);
void fj_append(fj_array *fj, unsigned short from_index, unsigned short capture_index, unsigned short to_index);
void find_forward_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj);
void find_reverse_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj);
void are_forced_jumps(board_t *b, fj_array *fj, player_t player);
void prune_fj(fj_array *fj, unsigned short from_index);
void switch_player(player_t *player);
unsigned short move_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
void funeral(board_t *board, game_history *history, fj_array *fj, unsigned short game_state);
void clear_screen();
void print_player(player_t player);
void get_move(board_t *board, unsigned short *from_index, unsigned short *to_index, fj_array *fj, player_t player);
unsigned short get_index(void);
void inform_fj(fj_array *fj);
void init_mt(move_table *mt);
void expand_mt_if_full(move_table *mt);
board_t generate_bitboard(board_t b, unsigned short from_index, unsigned short to_index, player_t player);
void generate_moves(board_t *b, move_table *mt, fj_array *fj, player_t player);
void generate_fj_moves(board_t *b, fj_array *fj, move_table *mt, player_t player);
board_t move_fj(board_t b, forced_jump *jump, player_t player);
void append_forward_moves(board_t *board, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player);
void append_reverse_moves(board_t *board, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player);
short evaluate_board(board_t *b, player_t player, unsigned short game_result);
short minimax(board_t *b, unsigned short depth, short alpha, short beta, player_t player, board_t *best_move);
void inform_minimax_move(board_t *prev, board_t *next, player_t player);
void inform_ai_move(board_t *board, board_t *ai_move);
void two_player(void);
unsigned short human_move(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
void AI_player(player_t starting_player);