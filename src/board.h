#define BOARD_SIZE 32
#define P1_PAWNS 4095
#define P2_PAWNS 4293918720

//bitboard representation of the game
typedef struct board_t {
	uint32_t player1_pawns; //32 bit unsigned int representing positions of PLAYER_1 pawns
	uint32_t player1_kings; //32 bit unsigned int representing positions of PLAYER_1 kings
	uint32_t player2_pawns; //32 bit unsigned int representing positions of PLAYER_2 pawns
	uint32_t player2_kings; //32 bit unsigned int representing positions of PLAYER_2 kings
}board_t;

void init_board(board_t *board, game_history *history);
void display_board(board_t *board);