#include <iostream>
#include <string>
#include <regex>
#include <set>

#define SZ 8 // 8x8 board
#define EMP 0 // empty square (no piece), minimum piece value must be EMP+1

#define WP 1 // white pieces
#define WN 2
#define WB 3
#define WR 4
#define WQ 5
#define WK 6 // king is max of white pieces
#define IS_WHITE(piece) ((piece>EMP)&&(piece<BP))

#define BP (WP+WK) // black pieces (greater than white pieces)
#define BN (WN+WK)
#define BB (WB+WK)
#define BR (WR+WK)
#define BQ (WQ+WK)
#define BK (WK+WK)
#define IS_BLACK(piece) ((piece>WK)&&(piece<INV))

#define INV (BK+1) // invalid piece (out-of-bounds access, for example), INV must be greater than the other piece values and EMP
#define ELEM(mat,i,j) (((i<0) || (i>=SZ) || (j<0) || (j>=SZ)) ? INV : mat[i][j])

#define WT true // white's turn
#define BT false // black's turn
#define NEXT(t) ((t==WT) ? BT:WT)

#define WKCAST 3 // castling availability for white king, binary position in cast
#define WQCAST 2
#define BKCAST 1
#define BQCAST 0

#define QCAST 1 // queenside castling
#define KCAST 2 // kingside castling
#define NCAST 0 // no castling

#define WKSQ (SZ*(SZ-1)+4) // white king starting square
#define BKSQ 4 // black king starting square

using namespace std;

struct minfo { // info for a chess move
    uint8_t sq1;
    uint8_t sq2;
    uint8_t newp;
    uint8_t castle;
};
typedef struct minfo minfo;

class ChessState{
    public:
        // fields (FEN state + piece-to-square mapping)
        uint8_t board[SZ][SZ];
        bool active; // active player
        uint8_t cast; // castling availability
        uint8_t enpassant; // en-passant square, out-of-bounds when not available
        uint32_t hmove;// half-moves since last capture or pawn advance
        uint32_t fmove; // full-move clock
        set<uint8_t> psquares[INV]; // where are the pieces located?

        ChessState(); // constructor
        void move(string str);
        void play_moves(vector<string> moves, bool verbose=true);
        void print_board();
    private:
        static vector<pair<int8_t,int8_t> > knight_dirs;
        static vector<pair<int8_t,int8_t> > king_dirs;
        static vector<pair<int8_t,int8_t> > bishop_dirs;
        static vector<pair<int8_t,int8_t> > rook_dirs;
        static vector<pair<int8_t,int8_t> > queen_dirs;
        static string promotions;
        static char pchars[INV];
        static char cols[SZ];

        static int get_sq(string str);
        static uint8_t map_piece(bool active,char piece);

        void pawn_moves(uint8_t sq, vector<minfo>& move_list);
        void limited_piece_moves(uint8_t sq, vector<minfo>& move_list, const vector<pair<int8_t,int8_t> >& dirs);
        void unlimited_piece_moves(uint8_t sq, vector<minfo>& move_list, const vector<pair<int8_t,int8_t> >& dirs);
        void knight_moves(uint8_t sq,vector<minfo>& move_list);
        void qcast_moves(uint8_t sq, vector<minfo>& move_list);
        void kcast_moves(uint8_t sq, vector<minfo>& move_list);
        void king_moves(uint8_t sq,vector<minfo>& move_list);
        void bishop_moves(uint8_t sq,vector<minfo>& move_list);
        void rook_moves(uint8_t sq,vector<minfo>& move_list);
        void queen_moves(uint8_t sq,vector<minfo>& move_list);
        void all_moves(uint8_t sq, uint8_t piece, vector<minfo>& move_list);
        void all_moves(vector<minfo>& move_list);
        void execute_move(uint8_t sq1, uint8_t sq2,uint8_t newp,uint8_t castle);

        void fill_psquares();

        uint8_t knight_sq(uint8_t sq2, uint8_t piece,int drow=SZ,int dcol = SZ);
        uint8_t bishop_sq(uint8_t sq2, uint8_t piece, int dr=SZ, int dc=SZ);
        uint8_t rook_sq(uint8_t sq2, uint8_t piece, int dr=SZ, int dc=SZ);
        uint8_t queen_sq(uint8_t sq2, uint8_t piece, int dr=SZ, int dc=SZ);
        uint8_t king_sq(uint8_t sq2, uint8_t piece, int dr=SZ, int dc=SZ);
        uint8_t attack_sq(uint8_t sq2, uint8_t piece, char type,int drow=SZ,int dcol = SZ);
};