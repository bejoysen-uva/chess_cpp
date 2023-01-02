#include <iostream>
#include <string>
#include <regex>

#define SZ 8 // 8x8 board
#define EMP 0 // empty square

#define WP 1 // white pieces
#define WN 2
#define WB 3
#define WR 4
#define WQ 5
#define WK 6

#define BP 7 // black pieces
#define BN 8
#define BB 9
#define BR 10
#define BQ 11
#define BK 12

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

char pchars[13] = {[EMP]=' ',
                  [WP]='P',
                  [WN]='N',
                  [WB]='B',
                  [WR]='R',
                  [WQ]='Q',
                  [WK]='K',
                  [BP]='p',
                  [BN]='n',
                  [BR]='r',
                  [BB]='b',
                  [BQ]='q',
                  [BK]='k'}; // piece characters

char cols[SZ] = {'a','b','c','d','e','f','g','h'};
int get_sq(string str) {
    for(int i=0;i<SZ;i++) { // column
        if(str[0]==cols[i])
            return (SZ-(str[1]-'0'))*SZ+i;
    }
    return SZ*SZ; // mistake, throw error
}

uint8_t map_piece(bool active,char piece) { // ex: WT,'Q' -> WQ
    if(active==WT) {
        switch(piece) {
            case 'N':
                return WN;
            case 'B':
                return WB;
            case 'R':
                return WR;
            case 'Q':
                return WQ;
            case 'K':
                return WK;
        }
    } else {
        switch(piece) {
            case 'N':
                return BN;
            case 'B':
                return BB;
            case 'R':
                return BR;
            case 'Q':
                return BQ;
            case 'K':
                return BK;
            break;
        }
    }
    return EMP;
}

void print(uint8_t (&board)[SZ][SZ]) {
    for(uint8_t j=0;j<2*SZ+1;j++) {
        cout << '-';
    }
    cout << endl;
    for(uint8_t i=0;i<SZ;i++) {
        cout << '|';
        for(uint8_t j=0;j<SZ;j++) {
            cout << pchars[board[i][j]] << '|';
        }
        cout << endl;
        for(uint8_t j=0;j<2*SZ+1;j++) {
            cout << '-';
        }
        cout << endl;
    }
}
class state{
    public:
        // uint8_t board[SZ][SZ] = {
        //                     {BR,BN,BB,BQ,BK,BB,BN,BR},
        //                     {BP,BP,BP,BP,BP,BP,BP,BP},
        //                     {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
        //                     {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
        //                     {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
        //                     {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
        //                     {WP,WP,WP,WP,WP,WP,WP,WP},
        //                     {WR,WN,WB,WQ,WK,WB,WN,WR}};
        uint8_t board[SZ][SZ] = {
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {BP,BP,BP,BP,BP,BP,BP,BP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                            {WP,WP,WP,WP,WP,WP,WP,WP},
                            {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP}};
        bool active = WT; // active player
        uint8_t cast = (1<<WKCAST) | (1<<WQCAST) | (1<<BKCAST) | (1<<BQCAST); // castling availability
        uint8_t enpassant = SZ*SZ; // en-passant square, out-of-bounds when not available
        uint32_t hmove = 0; // half-moves since last capture or pawn advance
        uint32_t fmove = 1; // full-move clock

        void move(string str) {
            // notation valid for 8x8 board or smaller
            if (str=="O-O") { // castle kingside
                if (active==WT)
                    execute_move(WKSQ,WKSQ+2,board[WKSQ/SZ][WKSQ%SZ],KCAST);
                else
                    execute_move(BKSQ,BKSQ+2,board[BKSQ/SZ][BKSQ%SZ],KCAST);
            }
            else if (str=="O-O-O") { // castle queenside
                if (active==WT)
                    execute_move(WKSQ,WKSQ-2,board[WKSQ/SZ][WKSQ%SZ],QCAST);
                else
                    execute_move(BKSQ,BKSQ-2,board[BKSQ/SZ][BKSQ%SZ],QCAST);
            }
            else if (regex_match(str,regex("^[abcdefgh][0-8][\\+#]?$"))) { // pawn moves forward (including moving two squares)
                uint8_t sq2 = get_sq(str.substr(0,2));
                uint8_t sq1 =  sq2+SZ*((active==WT) ? 1: -1 ); // one-square move
                if (!(board[sq1/SZ][sq1%SZ]==WP || board[sq1/SZ][sq1%SZ]==BP)) // no pawn on that square
                    sq1 = sq2+SZ*((active==WT) ? 2: -2 ); // two-square move
                execute_move(sq1,sq2,board[sq1/SZ][sq1%SZ],NCAST);
            }
            else if (regex_match(str,regex("^[abcdefgh]x[abcdefgh][0-8][\\+#]?$"))) { // pawn captures (including en passant)
                uint8_t sq2 = get_sq(str.substr(2,2));
                uint8_t sq1 = sq2/SZ*SZ+get_sq(str.substr(0,1)+"1")%SZ+SZ*((active==WT) ? 1: -1 );
                execute_move(sq1,sq2,board[sq1/SZ][sq1%SZ],NCAST);
            }
            else if (regex_match(str,regex("^[abcdefgh][0-8]=[NBRQ][+#]?[\\+#]?$"))) { // pawn moves forward, promotes
                uint8_t sq2 = get_sq(str.substr(0,2));
                uint8_t sq1 = sq2+SZ*((active==WT)?1:-1); // FILL IN
                execute_move(sq1,sq2,map_piece(active,str[3]),NCAST);
            }
            else if (regex_match(str,regex("^[abcdefgh]x[abcdefgh][0-8]=[NBRQ][\\+#]?$"))) { // pawn captures, promotes
                uint8_t sq2 = get_sq(str.substr(2,2));
                uint8_t sq1 = get_sq(str.substr(0,1)+"1")%SZ+sq2/SZ*SZ;
                execute_move(sq1,sq2,map_piece(active,str[5]),NCAST);
            }
            else if (regex_match(str,regex("^[NBKQ](?:[abcdefgh][0-8]){1,2}[\\+#]?$"))) { // piece moves (no capture)
                //
            }
            else if (regex_match(str,regex("^[NBKQ](?:[abcdefgh][0-8])?x[abcdefgh][0-8][\\+#]?$"))) { // piece captures
                //
            }



        }
    private:
        void execute_move(uint8_t sq1, uint8_t sq2,uint8_t newp,uint8_t castle) {
            // move piece from square 1 to square 2 (must accomodate en passant and castle)
            // the piece becomes newp on square 2 (e.g. promotion)
            // does not check if move is legal

            // reset hmove clock if capture or pawn move
            uint8_t next_enpassant = SZ*SZ; // enpassant available on next move?
            if (board[sq1/SZ][sq1%SZ]==WP || board[sq1/SZ][sq1%SZ]==BP) { // pawn move
                if (abs(sq2-sq1)==2*SZ)
                    next_enpassant = sq1+(sq2-sq1)/2; // can do enpassant at midpoint
                hmove = -1; // reset hmove clock if pawn moved (-1 b/c of increment)
            }
            if ((board[sq2/SZ][sq2%SZ]!=EMP))
                hmove = -1; // reset hmove clock if piece is captured
            hmove+=1; // increment hmove
            fmove = (active==BT) ? (fmove+1) : fmove;

            // update castling availability
            if ((cast>>WQCAST)%2 && ((sq1==WKSQ)||(sq1==WKSQ-4)||(sq2==WKSQ-4)))
                cast -= (1<<WQCAST);
            if ((cast>>WKCAST)%2 && ((sq1==WKSQ)||(sq1==WKSQ+SZ-5)||(sq2==WKSQ+SZ-5)))
                cast -= (1<<WKCAST);
            if ((cast>>BQCAST)%2 && ((sq1==BKSQ)||(sq1==BKSQ-4)||(sq2==BKSQ-4)))
                cast -= (1<<BQCAST);
            if ((cast>>BKCAST)%2 && ((sq1==BKSQ)||(sq1==BKSQ+SZ-5)||(sq2==BKSQ+SZ-5)))
                cast -= (1<<BKCAST);

            // update board
            board[sq2/SZ][sq2%SZ] = newp;
            board[sq1/SZ][sq1%SZ] = EMP; 
            if (sq2==enpassant) {
                board[sq2/SZ+((active==WT)?1:-1)][sq2%SZ] = EMP; // capture pawn enpassant
            }
            else if (castle==QCAST){
                board[sq2/SZ][sq2%SZ+1] = (active==WT) ? WR : BR;
                board[sq1/SZ][0] = EMP;
            }
            else if (castle==KCAST) {
                board[sq2/SZ][sq2%SZ-1] = (active==WT) ? WR : BR;
                board[sq1/SZ][SZ-1] = EMP;
            }

            // update next player and enpassant
            active = NEXT(active);
            enpassant = next_enpassant;
        }
};
int main() {
    state cstate;
    cstate.move("e4");
    print(cstate.board);
    cout << endl;

    cstate.move("d5");
    print(cstate.board);
    cout << endl;

    cstate.move("exd5");
    print(cstate.board);
    cout << endl;

    cstate.move("c5");
    print(cstate.board);
    cout << endl;

    cstate.move("dxc6");
    print(cstate.board);
    cout << endl;

    cstate.move("a5");
    print(cstate.board);
    cout << endl;

    cstate.move("cxb7");
    print(cstate.board);
    cout<<endl;

    cstate.move("a4");
    print(cstate.board);
    cout<<endl;

    cstate.move("b8=Q+");
    print(cstate.board);
    cout<<endl;
    return 0;
};