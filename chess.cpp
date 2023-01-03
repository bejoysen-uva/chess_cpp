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
class ChessState{
    public:

        uint8_t board[SZ][SZ];
        bool active; // active player
        uint8_t cast; // castling availability
        uint8_t enpassant; // en-passant square, out-of-bounds when not available
        uint32_t hmove;// half-moves since last capture or pawn advance
        uint32_t fmove; // full-move clock
        set<uint8_t> psquares[INV]; // where are the pieces located?

        ChessState() {
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
        static vector<pair<int8_t,int8_t>> knight_dirs;
        static vector<pair<int8_t,int8_t>> king_dirs;
        static vector<pair<int8_t,int8_t>> bishop_dirs;
        static vector<pair<int8_t,int8_t>> rook_dirs;
        static vector<pair<int8_t,int8_t>> queen_dirs;
        static string promotions;
        void pawn_moves(uint8_t sq, vector<minfo>& move_list) {
            uint8_t r = sq/SZ;
            uint8_t c = sq%SZ;
            minfo minfo;
            minfo.sq1 = sq;
            minfo.newp = board[r][c];
            minfo.castle = NCAST;

            int8_t fdir = (active==WT)? -1: 1; // row direction of forward movement for pawn
            if(board[r+fdir][c]==EMP) { // can move forward
                minfo.sq2 = sq+fdir*SZ;
                if(((r+fdir)==0)||((r+fdir)==SZ-1)) { // must promote on last row
                    for(char ch: promotions) {
                        minfo.newp = map_piece(active, ch);
                        move_list.push_back(minfo);
                    }
                } else {
                    move_list.push_back(minfo); // move forward 1 square
                    // move two squares if pawn is on its first row and the two squares are empty
                    if((((r-fdir)==0)||((r-fdir)==SZ-1))&&(board[r+2*fdir][c]==EMP)) {
                        minfo.sq2 = sq+2*fdir*SZ;
                        move_list.push_back(minfo);
                    }
                }
            }
            for(int8_t cdir=1;cdir>=-1;cdir-=2) { // capture left or right
                minfo.sq2 = sq+fdir*SZ+cdir;
                uint8_t oldp = ELEM(board,r+fdir,c+cdir);
                // en passant
                if(minfo.sq2==enpassant) {
                    move_list.push_back(minfo);
                }
                // can capture opposite color piece
                else if(((active==WT)&&(IS_BLACK(oldp)))||((active==BT)&&(IS_WHITE(oldp)))) {
                    if(((r+fdir)==0)||((r+fdir)==SZ-1)) {  // must promote on last row
                        for(char ch: promotions) {
                            minfo.newp = map_piece(active, ch);
                            move_list.push_back(minfo);
                        }
                    } else {
                        move_list.push_back(minfo);
                    }
                }

            }
        }
        void limited_piece_moves(uint8_t sq, vector<minfo>& move_list, const vector<pair<int8_t,int8_t>>& dirs) {
            uint8_t r = sq/SZ;
            uint8_t c = sq%SZ;
            uint8_t r2;
            uint8_t c2;
            uint8_t pdest;
            minfo minfo;
            minfo.sq1 = sq;
            minfo.newp = board[r][c];
            minfo.castle=NCAST;
            for(auto dpair: dirs) {
                r2 = r+dpair.first;
                c2 = c+dpair.second;
                pdest = ELEM(board,r2,c2);
                // invalid move iff out-of-bounds or captures your own piece
                if((pdest==INV)||((active==WT)&&IS_WHITE(pdest))||((active==BT)&&IS_BLACK(pdest)))
                    continue;
                
                minfo.sq2=r2*SZ+c2;
                move_list.push_back(minfo);  
            }
        }
        void unlimited_piece_moves(uint8_t sq, vector<minfo>& move_list, const vector<pair<int8_t,int8_t>>& dirs) {
            uint8_t r = sq/SZ;
            uint8_t c = sq%SZ;
            uint8_t r2;
            uint8_t c2;
            uint8_t pdest;

            minfo minfo;
            minfo.sq1 = sq;
            minfo.newp=board[r][c];
            minfo.castle=NCAST;

            for(auto dpair: dirs) {
                r2 = r+dpair.first; // first square on direction
                c2 = c+dpair.second;
                pdest = ELEM(board,r2,c2);
                while(pdest==EMP) {
                    minfo.sq2 = r2*SZ+c2;
                    move_list.push_back(minfo);
                    r2+=dpair.first; // continue on direction
                    c2+=dpair.second;
                    pdest = ELEM(board,r2,c2);
                }
                if (((active==WT)&&IS_BLACK(pdest))||((active==BT)&&IS_WHITE(pdest))) {
                    minfo.sq2 = r2*SZ+c2;
                    move_list.push_back(minfo);
                }
            }
        }
        void knight_moves(uint8_t sq,vector<minfo>& move_list) {
            limited_piece_moves(sq,move_list,knight_dirs);
        }
        void qcast_moves(uint8_t sq, vector<minfo>& move_list) {
            // does not check if castling puts king through/in check
            if(!(((active==WT)&&((cast>>WQCAST)%2))||((active==BT)&&((cast>>BQCAST)%2))))
                return;
            uint8_t r = sq/SZ;
            uint8_t c = sq%SZ;
            minfo minfo;
            minfo.sq1 = sq;
            minfo.sq2 = sq-2;
            minfo.newp = board[r][c];
            minfo.castle=QCAST;

            if((board[r][c-1]==EMP)&&(board[r][c-2]==EMP)&&(board[r][c-3]==EMP))
                move_list.push_back(minfo);
        }
        void kcast_moves(uint8_t sq, vector<minfo>& move_list) {
            // does not check if castling puts king through/in check
            if(!(((active==WT)&&((cast>>WQCAST)%2))||((active==BT)&&((cast>>BQCAST)%2))))
                return;
            uint8_t r = sq/SZ;
            uint8_t c = sq%SZ;
            minfo minfo;
            minfo.sq1 = sq;
            minfo.sq2 = sq+2;
            minfo.newp = board[r][c];
            minfo.castle=KCAST;

            if((board[r][c+1]==EMP)&&(board[r][c+2]==EMP))
                move_list.push_back(minfo);
        }
        void king_moves(uint8_t sq,vector<minfo>& move_list) {
            limited_piece_moves(sq,move_list,king_dirs);
            qcast_moves(sq,move_list);
            kcast_moves(sq,move_list);
        }
        void bishop_moves(uint8_t sq,vector<minfo>& move_list) {
            unlimited_piece_moves(sq,move_list,bishop_dirs);
        }
        void rook_moves(uint8_t sq,vector<minfo>& move_list) {
            unlimited_piece_moves(sq,move_list,rook_dirs);
        }
        void queen_moves(uint8_t sq,vector<minfo>& move_list) {
            unlimited_piece_moves(sq,move_list,queen_dirs);
        }
        void all_moves(uint8_t sq, uint8_t piece, vector<minfo>& move_list) {
            // does not check if a move puts king in check
            switch(piece) {
                case WP:
                case BP:
                    return pawn_moves(sq,move_list);
                case WN:
                case BN:
                    return knight_moves(sq,move_list);
                case WR:
                case BR:
                    return rook_moves(sq,move_list);
                case WB:
                case BB:
                    return bishop_moves(sq,move_list);
                case WQ:
                case BQ:
                    return queen_moves(sq,move_list);
                case WK:
                case BK:
                    return king_moves(sq,move_list);
            }
        }

        void all_moves(vector<minfo>& move_list) {
            // does not check if a move puts king in check or whether castle puts king through check
            // fills in move_list with all possible moves
            uint8_t pstart = (active==WT) ? (EMP+1) : (WK+1);
            uint8_t pend = pstart+WK; // not-inclusive
            for(uint8_t p=pstart;p<pend;p++) {
                for(uint8_t sq: psquares[p]) {
                    all_moves(sq,board[sq/SZ][sq%SZ],move_list);
                }
            }
        }

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
            // THIS METHOD IS WRONG: disambiguation does not account for the fact that a piece may not be able to move if it puts the king in check

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
// static initialization
auto ChessState::knight_dirs = {pair<int8_t,int8_t>(-2,-1),
                                pair<int8_t,int8_t>(-2,1),
                                pair<int8_t,int8_t>(2,-1),
                                pair<int8_t,int8_t>(2,1),
                                pair<int8_t,int8_t>(-1,-2),
                                pair<int8_t,int8_t>(-1,2),
                                pair<int8_t,int8_t>(1,-2),
                                pair<int8_t,int8_t>(1,2)};
auto ChessState::king_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,0),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(0,-1),
                                pair<int8_t,int8_t>(0,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,0),
                                pair<int8_t,int8_t>(1,1)};
auto ChessState::queen_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,0),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(0,-1),
                                pair<int8_t,int8_t>(0,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,0),
                                pair<int8_t,int8_t>(1,1)};
auto ChessState::bishop_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,1)};
auto ChessState::rook_dirs = {pair<int8_t,int8_t>(-1,0),
                                pair<int8_t,int8_t>(1,0),
                                pair<int8_t,int8_t>(0,-1),
                                pair<int8_t,int8_t>(0,1)};
auto ChessState::promotions = "NBRQ";
                                
int main() {
    ChessState cstate;
    vector<string> moves = {"e4","d5","exd5","c5","dxc6","a5","cxb7","a4","b8=Q+","Nf6","g4","Nxg4","Bc4"};
    cstate.play_moves(moves);
    return 0;
}