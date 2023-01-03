#include "chess_state.h"
#include <sstream>

ChessState::ChessState() {
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
minfo ChessInterface::not2minfo(string str) {
    // NOT RELIABLE: because of putting king in check/moving king through check
    // convert algebraic notation to move info
    // notation valid for 8x8 board or smaller

    // We don't need to know whether the move causes check/checkmate to fill in minfo
    if((str[str.length()-1]=='+')||(str[str.length()-1]=='#')) 
        str = str.substr(0,str.length()-1);
    
    smatch sm;
    if (str=="O-O") { // castle kingside
        if (active==WT)
            return ((minfo){WKSQ,WKSQ+2,board[WKSQ/SZ][WKSQ%SZ],KCAST});
        else
            return ((minfo){BKSQ,BKSQ+2,board[BKSQ/SZ][BKSQ%SZ],KCAST});
    }
    else if (str=="O-O-O") { // castle queenside
        if (active==WT)
            return ((minfo){WKSQ,WKSQ-2,board[WKSQ/SZ][WKSQ%SZ],QCAST});
        else
            return ((minfo){BKSQ,BKSQ-2,board[BKSQ/SZ][BKSQ%SZ],QCAST});
    }
    else if (regex_match(str,regex("^[abcdefgh][0-8]$"))) { // pawn moves forward (including moving two squares)
        uint8_t sq2 = get_sq(str.substr(0,2));
        uint8_t sq1 =  sq2+SZ*((active==WT) ? 1: -1 ); // one-square move
        if (!(board[sq1/SZ][sq1%SZ]==WP || board[sq1/SZ][sq1%SZ]==BP)) // no pawn on that square
            sq1 = sq2+SZ*((active==WT) ? 2: -2 ); // two-square move
        return ((minfo){sq1,sq2,board[sq1/SZ][sq1%SZ],NCAST});
    }
    else if (regex_match(str,regex("^[abcdefgh]x[abcdefgh][0-8]$"))) { // pawn captures (including en passant)
        uint8_t sq2 = get_sq(str.substr(2,2));
        uint8_t sq1 = sq2/SZ*SZ+get_sq(str.substr(0,1)+"1")%SZ+SZ*((active==WT) ? 1: -1 );
        return ((minfo){sq1,sq2,board[sq1/SZ][sq1%SZ],NCAST});
    }
    else if (regex_match(str,regex("^[abcdefgh][0-8]=[NBRQ]$"))) { // pawn moves forward, promotes
        uint8_t sq2 = get_sq(str.substr(0,2));
        uint8_t sq1 = sq2+SZ*((active==WT)?1:-1); // FILL IN
        return ((minfo){sq1,sq2,map_piece(active,str[3]),NCAST});
    }
    else if (regex_match(str,regex("^[abcdefgh]x[abcdefgh][0-8]=[NBRQ]$"))) { // pawn captures, promotes
        uint8_t sq2 = get_sq(str.substr(2,2));
        uint8_t sq1 = get_sq(str.substr(0,1)+"1")%SZ+sq2/SZ*SZ;
        return ((minfo){sq1,sq2,map_piece(active,str[5]),NCAST});
    }
    // handle disambiguation (sometimes only need rank/file, sometimes need both)
    else if(regex_match(str,regex("^[NBKQ]x?[abcdefgh][0-8]$"))) { // piece moves, maybe captures, no ambiguity
        regex_search(str,sm,regex("[abcdefgh][0-8]"));
        uint8_t sq2 = get_sq(sm[0]);

        uint8_t piece = map_piece(active,str[0]);
        uint8_t sq1 = attack_sq(sq2,piece,str[0]);

        return ((minfo){sq1,sq2,piece,NCAST});
    }
    // rank disambiguation
        else if(regex_match(str,regex("^[NBKQ][0-8]x?[abcdefgh][0-8]$"))) { // piece moves, maybe captures, no ambiguity
        regex_search(str,sm,regex("[abcdefgh][0-8]"));
        uint8_t sq2 = get_sq(sm[0]);

        uint8_t piece = map_piece(active,str[0]);
        regex_search(str,sm,regex("[0-8]"));
        uint8_t sq1 = attack_sq(sq2,piece,str[0],SZ-(str[1]-'0'),SZ);
        
        return ((minfo){sq1,sq2,piece,NCAST});
    }
    // file disambiguation
    else if(regex_match(str,regex("^[NBKQ][abcdefgh]x?[abcdefgh][0-8]$"))) { // piece moves, maybe captures, no ambiguity
        regex_search(str,sm,regex("[abcdefgh][0-8]"));
        uint8_t sq2 = get_sq(sm[0]);

        uint8_t piece = map_piece(active,str[0]);
        regex_search(str,sm,regex("[abcdefgh]"));
        uint8_t sq1 = attack_sq(sq2,piece,str[0],SZ,get_sq(sm[0].str()+"1")%SZ);

        return ((minfo){sq1,sq2,piece,NCAST});
    }
    // square disambiguation
    else if(regex_match(str,regex("^[NBKQ][abcdefgh][0-8]x?[abcdefgh][0-8]$"))) { // piece moves, maybe captures, no ambiguity
        regex_search(str,sm,regex("[abcdefgh][0-8]"));
        uint8_t sq1 = get_sq(sm[0]);
        uint8_t piece = map_piece(active,str[0]);

        str = sm.suffix().str();
        regex_search(str,sm,regex("[abcdefgh][0-8]"));
        uint8_t sq2 = get_sq(sm[0]);

        return ((minfo){sq1,sq2,piece,NCAST});
    }
    // ERROR
    return ((minfo){SZ*SZ,SZ*SZ,INV,NCAST});
}
void ChessInterface::move(string str) {
    execute_move(not2minfo(str));
}
void ChessInterface::play_moves(vector<string> moves, bool verbose) {
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
void ChessState::print_board() {
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
int ChessInterface::get_sq(string str) {
    for(int i=0;i<SZ;i++) { // column
        if(str[0]==cols[i])
            return (SZ-(str[1]-'0'))*SZ+i;
    }
    return SZ*SZ; // mistake, throw error
}
uint8_t ChessState::map_piece(bool active,char type) { // ex: WT,'Q' -> WQ
    if(active==WT) {
        switch(type) {
            case 'P':
                return WP;
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
        switch(type) {
            case 'P':
                return BP;
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
char ChessState::map_type(uint8_t piece) {
    switch(piece) {
        case WP:
        case BP:
            return 'P';
        case WN:
        case BN:
            return 'N';
        case WB:
        case BB:
            return 'B';
        case WR:
        case BR:
            return 'R';
        case WQ:
        case BQ:
            return 'Q';
        case WK:
        case BK:
            return 'K';
        default:
            return 0; // ERROR
    }
}
void ChessState::pawn_moves(uint8_t sq, vector<minfo>& move_list) {
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
void ChessState::limited_piece_moves(uint8_t sq, vector<minfo>& move_list, const vector<pair<int8_t,int8_t> >& dirs) {
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
void ChessState::unlimited_piece_moves(uint8_t sq, vector<minfo>& move_list, const vector<pair<int8_t,int8_t> >& dirs) {
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
void ChessState::knight_moves(uint8_t sq,vector<minfo>& move_list) {
    limited_piece_moves(sq,move_list,knight_dirs);
}
void ChessState::qcast_moves(uint8_t sq, vector<minfo>& move_list) {
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
void ChessState::kcast_moves(uint8_t sq, vector<minfo>& move_list) {
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
void ChessState::king_moves(uint8_t sq,vector<minfo>& move_list) {
    limited_piece_moves(sq,move_list,king_dirs);
    qcast_moves(sq,move_list);
    kcast_moves(sq,move_list);
}
void ChessState::bishop_moves(uint8_t sq,vector<minfo>& move_list) {
    unlimited_piece_moves(sq,move_list,bishop_dirs);
}
void ChessState::rook_moves(uint8_t sq,vector<minfo>& move_list) {
    unlimited_piece_moves(sq,move_list,rook_dirs);
}
void ChessState::queen_moves(uint8_t sq,vector<minfo>& move_list) {
    unlimited_piece_moves(sq,move_list,queen_dirs);
}
void ChessState::all_moves(uint8_t sq, uint8_t piece, vector<minfo>& move_list) {
    // does not check if a move puts king in check
    char type = map_type(piece);
    switch(type) {
        case 'P':
            return pawn_moves(sq,move_list);
        case 'N':
            return knight_moves(sq,move_list);
        case 'R':
            return rook_moves(sq,move_list);
        case 'B':
            return bishop_moves(sq,move_list);
        case 'Q':
            return queen_moves(sq,move_list);
        case 'K':
            return king_moves(sq,move_list);
    }
}

void ChessState::all_moves(vector<minfo>& move_list) {
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

void ChessState::fill_psquares() {
    // fill in psquares
    for(int r=0;r<SZ;r++) {
        for(int c=0; c<SZ; c++) {
            if(board[r][c]!=EMP) {
                psquares[board[r][c]].insert(r*SZ+c);
            }
        }
    }
}
uint8_t ChessInterface::attack_sq(uint8_t sq2, uint8_t piece, char type,int drow,int dcol) {
    // THIS METHOD IS WRONG: disambiguation does not account for the fact that a piece may not be able to move if it puts the king in check

    // sq2: square attacked by the piece
    // piece, example: WN
    // type: 'N'
    // drow: disambiguation row (piece must be on this row if row<SZ)
    // dcol: disambiguation col (piece must be on this col if col<SZ)
    // (drow==SZ) || (dcol==SZ) assumed true, assume sq1 exists
    // return sq1 on which piece is located, that is attacking sq2
    for(uint8_t sq: psquares[piece]) {
        // sq must meet disambiguation condition (if exists)
        if(((drow!=SZ)&&(drow!=sq/SZ)) ||((dcol!=SZ)&&(dcol!=sq%SZ)))
            continue;
        vector<minfo> sqmoves;
        all_moves(sq,piece,sqmoves);
        for(minfo minfo: sqmoves) {
            if(minfo.sq2==sq2)
                return sq;
        }
    }
    return SZ*SZ;
}
void ChessState::execute_move(minfo minfo) {
    // move piece from square 1 to square 2 (must accomodate en passant and castle)
    // the piece becomes newp on square 2 (e.g. promotion)
    // does not check if move is legal
    uint8_t sq1 = minfo.sq1;
    uint8_t sq2 = minfo.sq2;
    uint8_t newp = minfo.newp;
    uint8_t castle = minfo.castle;

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
// static initialization
vector<pair<int8_t,int8_t> > ChessState::knight_dirs = {pair<int8_t,int8_t>(-2,-1),
                                pair<int8_t,int8_t>(-2,1),
                                pair<int8_t,int8_t>(2,-1),
                                pair<int8_t,int8_t>(2,1),
                                pair<int8_t,int8_t>(-1,-2),
                                pair<int8_t,int8_t>(-1,2),
                                pair<int8_t,int8_t>(1,-2),
                                pair<int8_t,int8_t>(1,2)};
vector<pair<int8_t,int8_t> > ChessState::king_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,0),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(0,-1),
                                pair<int8_t,int8_t>(0,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,0),
                                pair<int8_t,int8_t>(1,1)};
vector<pair<int8_t,int8_t> > ChessState::queen_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,0),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(0,-1),
                                pair<int8_t,int8_t>(0,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,0),
                                pair<int8_t,int8_t>(1,1)};
vector<pair<int8_t,int8_t> > ChessState::bishop_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,1)};
vector<pair<int8_t,int8_t> > ChessState::rook_dirs = {pair<int8_t,int8_t>(-1,0),
                                pair<int8_t,int8_t>(1,0),
                                pair<int8_t,int8_t>(0,-1),
                                pair<int8_t,int8_t>(0,1)};
string ChessState::promotions = "NBRQ";
char ChessState::pchars[INV] = {[EMP]=' ',
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
char ChessState::cols[SZ] = {'a','b','c','d','e','f','g','h'};


bool ChessInterface::one_play_input(int8_t verbose) {
    // verbose=0: no feedback
    // verbose=1: display board after each move
    // verbose=2: display possible moves for player
    if ((active==WT)&&verbose)
        cout << fmove << endl;
    if(verbose) {
        print_board();
        cout << endl;
    }
    // NEED code for notation->minfo to check whether user-input move is valid
    if(verbose==2) {
        vector<string> nlist = all_notes();
        cout << "All moves: [";
        for(int idx=0;idx<nlist.size();idx++) {
            cout << nlist[idx] << ((idx==nlist.size()-1)?']':',');
        }
        cout << endl;
    }
    string anot;
    cout << "Move: ";
    cin >> anot;
    cout << endl;
    
    if(anot=="q")
        return false;
    move(anot);
    return true;
}
void ChessInterface::play_input(int8_t verbose) {
    // verbose=0: no feedback
    // verbose=1: display board after each move
    // verbose=2: display possible moves for player
    while(one_play_input(verbose)) {}
}

string ChessInterface::minfo2not(minfo minf) {
    // INVALID: does not do + or # at end of notation.

    // castling notation is easy
    if((minf.castle)==KCAST)
        return "O-O";
    if((minf.castle)==QCAST)
        return "O-O-O";

    uint8_t r1 = minf.sq1/SZ;
    uint8_t c1 = minf.sq1%SZ;
    uint8_t r2 = minf.sq2/SZ;
    uint8_t c2 = minf.sq2%SZ;
    stringstream note;
    
    // pawn move

    if((board[r1][c1]==WP)||(board[r1][c1]==BP)) {
        if(c1!=c2) // pawn captures something
            note << cols[c1] << 'x';
        note << cols[c2] << int(SZ-r2); // destination square

        if(minf.newp!=board[r1][c1]) { // promotion
            note << "=" << map_type(minf.newp);
        }
        return note.str();
    }
    // any other piece moved
    note << map_type(minf.newp);
    // disambiguation
    bool uniq = true;
    bool runiq = true;
    bool cuniq = true;
    auto mlist = vector<minfo>();
    for(uint8_t osq: psquares[minf.newp]) {
        if(osq==minf.sq1)
            continue;
        all_moves(osq,minf.newp,mlist);
        for(minfo minf2: mlist) {
            if(minf.sq2==minf2.sq2) {
                uniq = false; // an equivalent piece on a different square can reach sq2
                if(minf2.sq1/SZ==r2) // if row isn't unique
                    runiq = false;
                if(minf2.sq2%SZ==c2) // if col isn't unique
                    cuniq = false;
            }
        }
        mlist.clear();
    }
    if(!uniq) { // need to disambiguate
        if(cuniq) // use unique column
            note << cols[c1];
        else if(runiq) // use unique row
            note << int(SZ-r1);
        else // must use full square
            note << cols[c1] << int(SZ-r1);
    }
    // capture
    if(board[r2][c2]!=EMP) {
        note << "x";
    }
    // destination square
    note << cols[c2] << int(SZ-r2);
    return note.str();
}

vector<string> ChessInterface::all_notes() {
    vector<minfo> mlist;
    vector<string> slist;
    all_moves(mlist);
    for(minfo mi: mlist) {
        slist.push_back(minfo2not(mi));
    }
    return slist;
}