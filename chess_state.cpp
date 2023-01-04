#include "chess_state.h"

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
        psquares[psq4].insert(sq4-2);

        psq5 = psq4;
        psq4 = EMP;
    }

    // update next player and enpassant
    active = NEXT(active);
    enpassant = next_enpassant;
}
// static initialization
vector<pair<int8_t,int8_t> > LazyChessState::knight_dirs = {pair<int8_t,int8_t>(-2,-1),
                                pair<int8_t,int8_t>(-2,1),
                                pair<int8_t,int8_t>(2,-1),
                                pair<int8_t,int8_t>(2,1),
                                pair<int8_t,int8_t>(-1,-2),
                                pair<int8_t,int8_t>(-1,2),
                                pair<int8_t,int8_t>(1,-2),
                                pair<int8_t,int8_t>(1,2)};
vector<pair<int8_t,int8_t> > LazyChessState::king_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,0),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(0,-1),
                                pair<int8_t,int8_t>(0,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,0),
                                pair<int8_t,int8_t>(1,1)};
vector<pair<int8_t,int8_t> > LazyChessState::queen_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,0),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(0,-1),
                                pair<int8_t,int8_t>(0,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,0),
                                pair<int8_t,int8_t>(1,1)};
vector<pair<int8_t,int8_t> > LazyChessState::bishop_dirs = {pair<int8_t,int8_t>(-1,-1),
                                pair<int8_t,int8_t>(-1,1),
                                pair<int8_t,int8_t>(1,-1),
                                pair<int8_t,int8_t>(1,1)};
vector<pair<int8_t,int8_t> > LazyChessState::rook_dirs = {pair<int8_t,int8_t>(-1,0),
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

pair<bool,bool> LazyChessState::execute_lazy_move(minfo mv) {
    uint8_t sq1 = mv.sq1;
    uint8_t sq2 = mv.sq2;
    uint8_t newp = mv.newp;
    uint8_t castle = mv.castle;
    uint8_t& psq1 = board[sq1/SZ][sq1%SZ];
    uint8_t& psq2 = board[sq2/SZ][sq2%SZ];
    // castling: move king and rook
    if(castle!=NCAST) {
        uint8_t sq3 = (castle==KCAST)? (sq1+2):(sq1-2);
        uint8_t sq4 = (castle==KCAST) ? (sq1+3):(sq1-4);
        uint8_t& psq3 = board[sq3/SZ][sq3%SZ];
        uint8_t& psq4 = board[sq4/SZ][sq4%SZ];

        psquares[psq1].erase(sq1); // king move
        psquares[newp].insert(sq2);
        psquares[psq3].erase(sq3); // rook move
        psquares[psq3].insert(sq4);

        psq1 = EMP; // king move
        psq2 = newp;
        psq4 = psq3; // rook move
        psq3 = EMP;
        active = NEXT(active);
        return pair<bool,bool>(false,false);
    }
    // enpassant: pawn move where columns are different and target square is empty
    else if (((psq1==WP)||(psq1==BP))&&(sq1%SZ!=sq2%SZ)&& (psq2==EMP)) {
        uint8_t sq3 = mv.sq1+2*(mv.sq2-mv.sq1); // captured pawn
        uint8_t& psq3 = board[sq3/SZ][sq3%SZ];
        
        psquares[psq1].erase(sq1);
        psquares[psq2].erase(sq2);
        psquares[newp].insert(sq2);
        psquares[psq3].erase(sq3);

        psq1 = EMP;
        psq2 = newp;
        psq3 = EMP;
        active = NEXT(active);
        return pair<bool,bool>(false,true);
    }
    // normal move or promotion
    else {
        bool promote = false;
        if(psq1!=newp)
            promote = true;
        
        psquares[psq1].erase(sq1);
        psquares[psq2].erase(sq2);
        psquares[newp].insert(sq2);

        psq1 = EMP;
        psq2 = newp;
        active = NEXT(active);
        return pair<bool,bool>(promote,false);
    }
}
void LazyChessState::undo_lazy_move(minfo mv,uint8_t oldp,bool promote, bool enpassant) {
    uint8_t sq1 = mv.sq1;
    uint8_t sq2 = mv.sq2;
    uint8_t newp = mv.newp;
    uint8_t castle = mv.castle;
    uint8_t& psq1 = board[sq1/SZ][sq1%SZ];
    uint8_t& psq2 = board[sq2/SZ][sq2%SZ];
    // undo castling: king and rook
    if(castle!=NCAST) {
        uint8_t sq3 = (castle==KCAST)? (sq1+2):(sq1-2);
        uint8_t sq4 = (castle==KCAST) ? (sq1+3):(sq1-4);
        uint8_t& psq3 = board[sq3/SZ][sq3%SZ];
        uint8_t& psq4 = board[sq4/SZ][sq4%SZ];

        psquares[newp].insert(sq1); // king move
        psquares[newp].erase(sq2);
        psquares[psq4].insert(sq3); // rook move
        psquares[psq4].erase(sq4);

        psq1 = newp; // king move
        psq2 = EMP;
        psq3 = psq4; // rook move
        psq4 = EMP;
    }
    // undo promotion
    else if(promote) {
        uint8_t pawn = (NEXT(active)==WT)?WP:BP; // pawn that moved
        psquares[newp].erase(sq2);
        psquares[pawn].insert(sq1);
        if(oldp!=EMP) // replace captured piece if existed
            psquares[oldp].insert(sq2);
        
        psq1 = pawn;
        psq2 = oldp;
    }
    // undo enpassant: NEED to fix
    else if (enpassant) {
        uint8_t sq3 = mv.sq1+2*(mv.sq2-mv.sq1); // captured pawn
        uint8_t& psq3 = board[sq3/SZ][sq3%SZ];
        uint8_t pawn = (active==WT)?WP:BP;
        
        psquares[newp].erase(sq2);
        psquares[newp].insert(sq1);
        psquares[pawn].insert(sq3);

        psq1 = newp;
        psq2 = EMP;
        psq3 = pawn;
    }
    // normal move
    else {
        psquares[newp].erase(sq2);
        psquares[newp].insert(sq1);
        if(oldp!=EMP) // replace captured piece if existed
            psquares[oldp].insert(sq2);
        
        psq1 = newp;
        psq2 = oldp;
    }
    active = NEXT(active);
}
void ChessState::all_legal_moves(vector<minfo>& lmvlist) {
    // assumes current position is legal!
    vector<minfo> mvlist;
    all_moves(mvlist);
    LazyChessState backup = *this;
    for(minfo mv: mvlist) {
        // ex: active=W, play white's move on backup board
        pair<bool,bool> mvdata = backup.execute_lazy_move(mv);
        uint8_t ksq = *backup.psquares[(active==WT) ? WK : BK].begin();
        if(!backup.is_checking(ksq)) { // white king cannot be in check by black after white has moved
            if (mv.castle==QCAST) { // white king cannot move through check to castle
                if(!backup.is_checking(ksq-1)&&!backup.is_checking(ksq-2))
                    lmvlist.push_back(mv);
            } else if (mv.castle==KCAST) {
                if(!backup.is_checking(ksq+1)&&!backup.is_checking(ksq+2))
                    lmvlist.push_back(mv);
            } else {
                lmvlist.push_back(mv);
            }
            // undo normal move
            backup.undo_lazy_move(mv,board[mv.sq1/SZ][mv.sq1%SZ],mvdata.first,mvdata.second);
        }

    }
}


bool LazyChessState::is_checking(uint8_t sq1, uint8_t sq2) {
    return false;
}
bool LazyChessState::is_checking(uint8_t sq) {
    return false;
}
bool LazyChessState::is_pawn_checking(uint8_t sq1, uint8_t sq2) {
    // is pawn on sq1 checking sq2? pawn must be the same color as the active player

    // adjacent column. white r1 = r2+1 OR black r1 = r2-1.
    return ((abs(sq1%SZ-sq2%SZ)==1)&&((active==WT)&&(sq1/SZ-sq2/SZ == 1))&&((active==BT)&&(sq2/SZ-sq1/SZ == 1)));
}

bool LazyChessState::is_limited_checking(uint8_t sq1, uint8_t sq2,vector<pair<int8_t,int8_t>>& dirs) {
    // ex: can a knight move on sq1 move to sq2?
    uint8_t r1 = sq1/SZ;
    uint8_t c1 = sq1%SZ;
    uint8_t r2 = sq2/SZ;
    uint8_t c2 = sq2%SZ;
    for(pair<int8_t,int8_t> dir: dirs) {
        if((r1+dir.first==r2)&&(c1+dir.second==c2))
            return true;        
    }
    return false;
}
bool LazyChessState::is_knight_checking(uint8_t sq1, uint8_t sq2) {
    return is_limited_checking(sq1,sq2,knight_dirs);
}
bool LazyChessState::is_king_checking(uint8_t sq1, uint8_t sq2) {
    return is_limited_checking(sq1,sq2,king_dirs);
}

bool LazyChessState::is_unlimited_checking(uint8_t sq1, uint8_t sq2,vector<pair<int8_t,int8_t>>& dirs) {
    // ex: bishop, go along each diagonal until sq2 or non-empty. if sq2==sq3 return true
    uint8_t r1 = sq1/SZ;
    uint8_t c1 = sq1%SZ;
    uint8_t r2 = sq2/SZ;
    uint8_t c2 = sq2%SZ;
    uint8_t r3;
    uint8_t c3;
    for(pair<int8_t,int8_t> dir: dirs) {
        r3 = r1+dir.first; // move forward
        c3 = c1+dir.second;
        // continue moving forward until sq2 or non-empty
        while(!((r2==r3)&&(c2==c3))&&(ELEM(board,r3,c3)==EMP)) {
            r3+=dir.first;
            c3+=dir.second;
        }
        if((r2==r3)&&(c2==c3)) // sq1 can capture sq2!
            return true;
    }
    return false;
}

bool LazyChessState::is_bishop_checking(uint8_t sq1, uint8_t sq2) {
    return is_limited_checking(sq1,sq2,bishop_dirs);
}
bool LazyChessState::is_rook_checking(uint8_t sq1, uint8_t sq2) {
    return is_limited_checking(sq1,sq2,rook_dirs);
}
bool LazyChessState::is_queen_checking(uint8_t sq1, uint8_t sq2) {
    return is_limited_checking(sq1,sq2,queen_dirs);
}