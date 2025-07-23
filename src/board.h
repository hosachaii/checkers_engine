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

//game modes available
typedef enum {
    TWO_PLAYER, //1 vs 1
    AI_PLAYER //1 vs the computer
} game_mode_t;

void init_board(board_t *board, game_history *history);
void display_board(board_t *board);
