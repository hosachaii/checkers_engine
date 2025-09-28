#define TRUE 1
#define FALSE 0

#define LAST_INDEX 31
#define BOARD_SIZE 32
#define INVALID 33
#define UNDO 34
#define REDO 35
#define HINT 36
#define QUIT 37

#define P1_PAWNS 4095
#define P2_PAWNS 4293918720

#define MAX_FORCED_JUMPS 10

//represents the state of a square on board
typedef enum {
	EMPTY = 0, //empty squre
	PLAYER_1, //=1
	PLAYER_2 //=2
}player_t;

//state of single square on board
typedef struct square_profile {
	player_t player; //which player has occupied the square (i.e. empty or PLAYER_1 or PLAYER_2)
	bool king; //if true, then king present
}square_profile;

//representation of board
typedef struct board_t {
	struct square_profile *squares; //pointer to 1d array of square_profile structures
}board_t;

//bitboard representation of the game
typedef struct bitboard_t {
	uint32_t player1_pawns; //32 bit unsigned int representing positions of PLAYER_1 pawns
	uint32_t player1_kings; //32 bit unsigned int representing positions of PLAYER_1 kings
	uint32_t player2_pawns; //32 bit unsigned int representing positions of PLAYER_2 pawns
	uint32_t player2_kings; //32 bit unsigned int representing positions of PLAYER_2 kings
}bitboard_t;

// stack node structure for storing board states.
typedef struct board_state {
    bitboard_t bitboard; //captures all the player positions
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

//game modes available
typedef enum {
    TWO_PLAYER, //1 vs 1
    AI_PLAYER //1 vs the computer
} game_mode_t;

void init_board(board_t *board, game_history *history);
void display_board(board_t *board);
bitboard_t arr_to_bitboard(board_t *board);
void bitboard_to_arr(bitboard_t *bitboard, board_t *board);
void init_board_state(board_state *bs);
void free_redo(game_history *history);
void save_progress(bitboard_t *bitboard, game_history *history, player_t current_player);
bool is_undo_empty(game_history *history);
bool is_redo_empty(game_history *history);
bool undo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
bool redo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player);
void funeral(board_t *board, game_history *history, fj_array *fj, unsigned short game_state);
