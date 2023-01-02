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
#define WK 6

#define BP 7 // black pieces
#define BN 8
#define BB 9
#define BR 10
#define BQ 11
#define BK 12

#define INV 13 // invalid piece (out-of-bounds access, for example), INV must be greater than the other piece values and EMP
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

char pchars[INV] = {[EMP]=' ',
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
class state{
    public:

        uint8_t board[SZ][SZ];
        bool active; // active player
        uint8_t cast; // castling availability
        uint8_t enpassant; // en-passant square, out-of-bounds when not available
        uint32_t hmove;// half-moves since last capture or pawn advance
        uint32_t fmove; // full-move clock
        set<uint8_t> psquares[INV]; // where are the pieces located?

        state() {
            uint8_t dboard[8][8] = {
                    {BR,BN,BB,BQ,BK,BB,BN,BR},
                    {BP,BP,BP,BP,BP,BP,BP,BP},
                    {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                    {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                    {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                    {EMP,EMP,EMP,EMP,EMP,EMP,EMP,EMP},
                    {WP,WP,WP,WP,WP,WP,WP,WP},
                    {WR,WN,WB,WQ,WK,WB,WN,WR}};
            memcpy(board,dboard,SZ*SZ*sizeof(uint8_t));
            active = WT;
            cast = (1<<WKCAST) | (1<<WQCAST) | (1<<BKCAST) | (1<<BQCAST);
            enpassant = SZ*SZ;
            hmove = 0;
            fmove = 1;

            for(int i=0; i<INV; i++) {
                psquares[i] = set<uint8_t>();
            }
            fill_psquares();
        }
        void move(string str) {
            // notation valid for 8x8 board or smaller
            smatch sm;
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
            // handle disambiguation (sometimes only need rank/file, sometimes need both)
            else if(regex_match(str,regex("^[NBKQ]x?[abcdefgh][0-8][\\+$]?$"))) { // piece moves, maybe captures, no ambiguity
                regex_search(str,sm,regex("[abcdefgh][0-8]"));
                uint8_t sq2 = get_sq(sm[0]);

                uint8_t piece = map_piece(active,str[0]);
                uint8_t sq1 = attack_sq(sq2,piece,str[0]);

                execute_move(sq1,sq2,piece,NCAST);
            }
            // rank disambiguation
             else if(regex_match(str,regex("^[NBKQ][0-8]x?[abcdefgh][0-8][\\+$]?$"))) { // piece moves, maybe captures, no ambiguity
                regex_search(str,sm,regex("[abcdefgh][0-8]"));
                uint8_t sq2 = get_sq(sm[0]);

                uint8_t piece = map_piece(active,str[0]);
                regex_search(str,sm,regex("[0-8]"));
                uint8_t sq1 = attack_sq(sq2,piece,str[0],SZ-(str[1]-'0'),SZ);
                
                execute_move(sq1,sq2,piece,NCAST);
            }
            // file disambiguation
            else if(regex_match(str,regex("^[NBKQ][abcdefgh]x?[abcdefgh][0-8][\\+$]?$"))) { // piece moves, maybe captures, no ambiguity
                regex_search(str,sm,regex("[abcdefgh][0-8]"));
                uint8_t sq2 = get_sq(sm[0]);

                uint8_t piece = map_piece(active,str[0]);
                regex_search(str,sm,regex("[abcdefgh]"));
                uint8_t sq1 = attack_sq(sq2,piece,str[0],SZ,get_sq(sm[0].str()+"1")%SZ);

                execute_move(sq1,sq2,piece,NCAST);
            }
            // square disambiguation
            else if(regex_match(str,regex("^[NBKQ][abcdefgh][0-8]x?[abcdefgh][0-8][\\+$]?$"))) { // piece moves, maybe captures, no ambiguity
                regex_search(str,sm,regex("[abcdefgh][0-8]"));
                uint8_t sq1 = get_sq(sm[0]);
                uint8_t piece = map_piece(active,str[0]);

                str = sm.suffix().str();
                regex_search(str,sm,regex("[abcdefgh][0-8]"));
                uint8_t sq2 = get_sq(sm[0]);

                execute_move(sq1,sq2,piece,NCAST);
            }



        }

        void play_moves(vector<string> moves, bool verbose=true) {
            for(string str: moves) {
                if ((active==WT)&&verbose)
                    cout << fmove << endl;
                move(str);
                if(verbose) {
                    print_board();
                    cout << endl;
                }
            }
        }
        void print_board() {
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

    private:
        void fill_psquares() {
            // fill in psquares
            for(int r=0;r<SZ;r++) {
                for(int c=0; c<SZ; c++) {
                    if(board[r][c]!=EMP) {
                        psquares[board[r][c]].insert(r*SZ+c);
                    }
                }
            }
        }
        uint8_t knight_sq(uint8_t sq2, uint8_t piece,int drow=SZ,int dcol = SZ) {
            for(uint8_t sq: psquares[piece]) {
                uint8_t rdiff = abs(sq2/SZ-sq/SZ);
                uint8_t cdiff = abs(sq2%SZ-sq%SZ);
                // a knight on sq must be able to attack sq2
                // must match disambiguation condition (if there is one)
                if((((rdiff==2)&&(cdiff==1))||((rdiff==1)&&(cdiff==2)))&&(((drow==SZ)||(drow==sq/SZ))&&((dcol==SZ)||(dcol==sq%SZ)))){
                    return sq;
                }
            }
            return SZ*SZ; // error
        }
        uint8_t bishop_sq(uint8_t sq2, uint8_t piece, int dr=SZ, int dc=SZ) {
            for(uint8_t sq: psquares[piece]) {
                // skip if a disambiguation condition is violated
                if(((dr!=SZ)&&(dr!=sq/SZ))||((dc!=SZ)&&(dc!=sq%SZ)))
                    continue; 
                
                int8_t rdiff = sq2/SZ-sq/SZ; // differences are signed (must use int8_t)
                int8_t cdiff = sq2%SZ-sq%SZ;

                // skip if not on the same diagonal
                if (abs(rdiff)!=abs(cdiff))
                    continue;
                
                // intermediate squares must be empty
                int8_t rdir = (rdiff<0) ? -1:1;
                int8_t cdir = (cdiff<0) ? -1:1;
                bool valid = true; // true if diagonal empty; 
                for(uint8_t inc=1;inc<abs(rdiff);inc++) { 
                    if(board[sq/SZ+inc*rdir][sq%SZ+inc*cdir]!=EMP) {
                        valid = false;
                        break;
                    }
                }
                if(valid)
                    return sq;
            }
            return SZ*SZ; // error
        }
        uint8_t rook_sq(uint8_t sq2, uint8_t piece, int dr=SZ, int dc=SZ) {
            uint8_t r;
            uint8_t c;
            uint8_t r2 = sq2/SZ;
            uint8_t c2 = sq2%SZ;
            bool valid;
            for(uint8_t sq: psquares[piece]) {
                // skip if a disambiguation condition is violated
                if(((dr!=SZ)&&(dr!=r))||((dc!=SZ)&&(dc!=c)))
                    continue;
                
                r = sq/SZ;
                c = sq%SZ;
                valid = true;
                // same row, intermediate squares must be empty
                if(r==r2) {
                    for(uint8_t c3=min(c,c2)+1;c3<max(c,c2);c3++) {
                        if(board[r][c3]!=EMP) {
                            valid = false;
                            break;
                        }
                    }
                    if(valid)
                        return sq;
                } // same column, intermediate squares must be empty
                else if (c==c2) {
                    for(uint8_t r3=min(r,r2)+1;r3<max(r,r2);r3++) {
                        if(board[r3][c]!=EMP) {
                            valid = false;
                            break;
                        }
                    }
                    if(valid)
                        return sq;
                }
            }
            return SZ*SZ;
        }
        uint8_t queen_sq(uint8_t sq2, uint8_t piece, int dr=SZ, int dc=SZ) {
            uint8_t rsq = rook_sq(sq2,piece,dr,dc);
            if(rsq!=SZ*SZ) return rsq; // queen moved like a rook
            return bishop_sq(sq2,piece,dr,dc); // otherwise queen moves like a bishop
        }
        uint8_t king_sq(uint8_t sq2, uint8_t piece, int dr=SZ, int dc=SZ) {
            for(uint8_t sq: psquares[piece]) {
                return sq; // only one king of a given color
            }
            return SZ*SZ;
        }
        uint8_t attack_sq(uint8_t sq2, uint8_t piece, char type,int drow=SZ,int dcol = SZ) {
            // sq2: square attacked by the piece
            // piece, example: WN
            // type: 'N'
            // drow: disambiguation row (piece must be on this row if row<SZ)
            // dcol: disambiguation col (piece must be on this col if col<SZ)
            // (drow==SZ) || (dcol==SZ) assumed true, assume sq1 exists
            // return sq1 on which piece is located, that is attacking sq2
            switch(type) {
                case 'N':
                    return knight_sq(sq2,piece,drow,dcol);
                case 'B':
                    return bishop_sq(sq2,piece,drow,dcol);
                case 'R':
                    return rook_sq(sq2,piece,drow,dcol);
                case 'Q':
                    return queen_sq(sq2,piece,drow,dcol);
                case 'K':
                    return king_sq(sq2,piece,drow,dcol);
                default:
                    return SZ*SZ;
            }
        }
        void execute_move(uint8_t sq1, uint8_t sq2,uint8_t newp,uint8_t castle) {
            // move piece from square 1 to square 2 (must accomodate en passant and castle)
            // the piece becomes newp on square 2 (e.g. promotion)
            // does not check if move is legal
            uint8_t& psq1 = board[sq1/SZ][sq1%SZ];
            uint8_t& psq2 = board[sq2/SZ][sq2%SZ];
            // reset hmove clock if capture or pawn move
            uint8_t next_enpassant = SZ*SZ; // enpassant available on next move?
            if (psq1==WP || psq1==BP) { // pawn move
                if (abs(sq2-sq1)==2*SZ)
                    next_enpassant = sq1+(sq2-sq1)/2; // can do enpassant at midpoint
                hmove = -1; // reset hmove clock if pawn moved (-1 b/c of increment)
            }
            if (psq2!=EMP)
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

            // update board and psquares
            psquares[psq1].erase(sq1); // piece moves away from sq1
            psquares[psq2].erase(sq2); // piece at sq2 delete
            psquares[newp].insert(sq2); // new piece at sq2

            psq1 = EMP; // updates board using reference
            psq2 = newp;
            
            if (sq2==enpassant) {
                // pawn captured  by enpassant has the same row as sq1 and same column as sq2
                uint8_t sq3 = sq1/SZ*SZ+sq2%SZ;
                uint8_t& psq3 = board[sq3/SZ][sq3%SZ];
                psquares[psq3].erase(sq3); // update psquares
                psq3 = EMP; // update board
            }
            else if (castle==QCAST){
                uint8_t sq4 = sq1-sq1%SZ; // left-most square is queenside rook
                uint8_t& psq4 = board[sq4/SZ][0];
                uint8_t& psq5 = board[sq4/SZ][3];
                psquares[psq4].erase(sq4);
                psquares[psq4].insert(sq4+3);

                psq5 = psq4;
                psq4 = EMP;
            }
            else if (castle==KCAST) {
                uint8_t sq4 = sq1-sq1%SZ+SZ-1; // right-most square is queenside rook
                uint8_t& psq4 = board[sq4/SZ][SZ-1];
                uint8_t& psq5 = board[sq4/SZ][SZ-3];
                psquares[psq4].erase(sq4);
                psquares[psq4].insert(sq4+3);

                psq5 = psq4;
                psq4 = EMP;
            }

            // update next player and enpassant
            active = NEXT(active);
            enpassant = next_enpassant;
        }
};
int main() {
    state cstate;
    vector<string> moves = {"e4","d5","exd5","c5","dxc6","a5","cxb7","a4","b8=Q+","Nf6","g4","Nxg4","Bc4"};
    cstate.play_moves(moves);
    return 0;
}