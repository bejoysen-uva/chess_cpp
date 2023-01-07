#include "chess_state.h"
#include <sstream>

ChessState::ChessState(const string& fen) {
    // board
    uint8_t crow = 0;
    uint8_t ccol = 0;
    auto it = fen.begin();
    for(;crow<SZ;it++) {
        if(*it=='/'||*it==' ') {
            crow+=1;
            ccol=0;
        } else if(isdigit(*it)) {
            for(uint8_t j=0; j<*it-'0';j++)
                board[crow][ccol++] = EMP;
        } else
            board[crow][ccol++] = char2p[*it];//wrong
    }

    active = (*(it++)=='w')?WT:BT;
    it++; // skip over space

    cast = (*(it++)=='K') ? (1<<WKCAST) : 0;
    if(*(it++)=='Q') cast |= 1<<WQCAST;
    if(*(it++)=='k') cast |= 1<<BKCAST;
    if(*(it++)=='q') cast |= 1<<BQCAST;
    it++; // skip over space

    char ch = *(it++);
    if(ch=='-')
        enpassant = SZ*SZ;
    else
        enpassant = (SZ-(*(it++)-'0'))*SZ+char2col[ch]; // row and col
    

    hmove = 0;
    ch = *(++it);
    while(ch!=' ') {
        hmove += 10*hmove+(ch-'0');
        ch = *(++it);
    }
    
    // ch == ' ', it points to ch
    it++;
    fmove = 0;
    while(it!=fen.end())
        fmove += 10*fmove+(*(it++)-'0');


    // psquares
    for(int i=0; i<INV; i++) {
        psquares[i] = set<uint8_t>();
    }
    fill_psquares();
}
string ChessState::get_FEN() {
    stringstream ss;
    uint8_t cnt; // number of empty squares consecutively in a row
    uint8_t psq;

    // board
    for(uint8_t i=0; i<SZ; i++) {
        cnt = 0;
        for(uint8_t j=0;j<SZ;j++) {
            psq = board[i][j];
            if(psq==EMP)
                cnt+=1;
            else {
                if(cnt!=0) {
                    ss << (int) cnt;
                }
                cnt = 0;
                ss << pchars[psq];
            }
        }
        if(cnt!=0) {
            ss << (int) cnt; // how many empty squares?
        }
        ss << ((i==SZ-1)?' ':'/');
    }
    // w/b turn
    ss << ((active==WT)?'w':'b') << ' ';
    // castling availability
    ss << (((cast>>WKCAST)%2==1)?'K':'-')
        << (((cast>>WQCAST)%2==1)?'Q':'-')
        << (((cast>>BKCAST)%2==1)?'k':'-')
        << (((cast>>BQCAST)%2==1)?'q':'-');
    ss << ' ';
    // enpassant
    if(enpassant<SZ*SZ)
        ss << cols[enpassant%8] << int(SZ-enpassant/8);
    else
        ss << '-';
    ss << ' ' << hmove << ' ' << fmove;
    return ss.str();
}
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
    if(!(((active==WT)&&((cast>>WKCAST)%2))||((active==BT)&&((cast>>BKCAST)%2))))
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
    
    // enpassant
    if ((newp==WP||newp==BP)&&sq2==enpassant) {
        // pawn captured  by enpassant has the same row as sq1 and same column as sq2
        uint8_t sq3 = sq1/SZ*SZ+sq2%SZ;
        uint8_t& psq3 = board[sq3/SZ][sq3%SZ];
        psquares[psq3].erase(sq3); // update psquares
        psq3 = EMP; // update board
    } // castling
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

void ChessState::undo_move(const ChessState& orig, minfo minfo) {

    // reset game data
    cast = orig.cast;
    enpassant = orig.enpassant;
    hmove = orig.hmove;
    fmove = orig.fmove;
    active = orig.active;

    vector<uint8_t> operations = {minfo.sq1,minfo.sq2};
    if(minfo.castle==QCAST) {
        operations.push_back(minfo.sq1-minfo.sq1%SZ); // qrook at leftmost col
        operations.push_back(minfo.sq2+1); // qrook ends up right of king
    } else if (minfo.castle==KCAST) {
        operations.push_back(minfo.sq1-minfo.sq1%SZ+SZ-1); // krook at rightmost col
        operations.push_back(minfo.sq2-1); // krook ends up left of king
    } 
    // pawn moved to enpassant square
    else if(minfo.sq2==orig.enpassant && map_type(minfo.newp)=='P') {
        operations.push_back(minfo.sq1/SZ*SZ+minfo.sq2%SZ);
    }
    
    for(int idx=0;idx<operations.size();idx++) {
        uint8_t sq = operations[idx];
        uint8_t& psq = board[sq/SZ][sq%SZ];
        uint8_t opsq = orig.board[sq/SZ][sq%SZ];

        // reset psquares
        if(opsq!=EMP)
            psquares[opsq].insert(sq);
        psquares[psq].erase(sq);
        // reset board
        psq = opsq;
    }
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


map<char,uint8_t> ChessState::char2p = {};
map<char,uint8_t> ChessState::char2col = {};

bool ChessState::char2p_filled = ChessState::fill_maps();

bool ChessState::fill_maps() {
    for(int i=0; i<INV; i++) {
        char2p[pchars[i]] = i;
    }
    for(int i=0; i<SZ; i++) {
        char2col[cols[i]] = i;
    }
    return true;
}

char ChessState::cols[SZ] = {'a','b','c','d','e','f','g','h'};

void ChessState::all_legal_moves(vector<minfo>& lmvlist,ChessState* backup) {
    // assumes current position is legal!
    vector<minfo> mvlist;
    all_moves(mvlist);

    // ChessInterface has its own backup boards that it updates after every move. 
    // Otherwise, we have to dynamically allocate our own backup board
    // to determine whether a move will put the king in check.
    bool cleanup = false;
    if(backup==NULL) {
        backup =  new ChessState(*this);
        cleanup = true;
    }

    for(minfo mv: mvlist) {
        // ex: active=W, play white's move on backup board
        backup->execute_move(mv);
        uint8_t ksq = *backup->psquares[(active==WT) ? WK : BK].begin();
        if(!backup->is_checking(backup->active,ksq)) { // white king cannot be in check by black after white has moved
            if (mv.castle==QCAST) { // white king cannot move through check to castle
                if(!backup->is_checking(backup->active,ksq+1)&&!backup->is_checking(backup->active,ksq+2))
                    lmvlist.push_back(mv);
            } else if (mv.castle==KCAST) {
                if(!backup->is_checking(backup->active,ksq-1)&&!backup->is_checking(backup->active,ksq-2))
                    lmvlist.push_back(mv);
            } else {
                lmvlist.push_back(mv);
            }
        }
        // undo normal move
        backup->undo_move(*this,mv);
    }
    // delete the backup if we dynamically allocated it
    if(cleanup)
        delete backup;
}

uint8_t ChessState::get_state(ChessState* backup) {
    // ChessInterface has its own backup boards that it updates after every move. 
    // Otherwise, we have to dynamically allocate our own backup board
    // to determine whether a move will put the king in check.
    bool cleanup = false;
    if(backup==NULL) {
        backup =  new ChessState(*this);
        cleanup = true;
    }

    uint8_t ksq = *psquares[(active==WT) ? WK : BK].begin();
    bool check = is_checking(NEXT(active),ksq);
    vector<minfo> lmvlist;
    all_legal_moves(lmvlist,backup);
    
    if(cleanup)
        delete backup; 
    
    if(check&&lmvlist.size()==0)
        return CHECKMATE;
    else if(check)
        return CHECK;
    else if(lmvlist.size()==0)
        return DRAW;
    else
        return NORMAL;

        
}
bool ChessState::is_checking(uint8_t sq1, uint8_t sq2) {
    switch(map_type(board[sq1/SZ][sq1%SZ])) {
        case 'P':
            return is_pawn_checking(sq1,sq2);
        case 'N':
            return is_knight_checking(sq1,sq2);
        case 'B':
            return is_bishop_checking(sq1,sq2);
        case 'R':
            return is_rook_checking(sq1,sq2);
        case 'Q':
            return is_queen_checking(sq1,sq2);
        case 'K':
            return is_king_checking(sq1,sq2);
        default:
            return true;
    }
}
bool ChessState::is_checking(bool attacker, uint8_t sq2) {
    for(uint8_t i=EMP+1; i<INV; i++) {
        if(IS_WHITE(i)!=(attacker==WT)) // only look at active pieces
            continue;
        for(uint8_t sq1: psquares[i]) {
            if(is_checking(sq1,sq2))
                return true;
        }
    }
    return false;
}
bool ChessState::is_pawn_checking(uint8_t sq1, uint8_t sq2) {
    uint8_t attacker = IS_WHITE(board[sq1/SZ][sq1%SZ])?WT:BT;
    // is pawn on sq1 checking sq2? pawn must be the same color as the active player

    // adjacent column. white r1 = r2+1 OR black r1 = r2-1.
    return (abs(sq1%SZ-sq2%SZ)==1)&&(((attacker==WT)&&(sq1/SZ-sq2/SZ == 1))||((attacker==BT)&&(sq2/SZ-sq1/SZ == 1)));
}

bool ChessState::is_limited_checking(uint8_t sq1, uint8_t sq2,vector<pair<int8_t,int8_t>>& dirs) {
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
bool ChessState::is_knight_checking(uint8_t sq1, uint8_t sq2) {
    return is_limited_checking(sq1,sq2,knight_dirs);
}
bool ChessState::is_king_checking(uint8_t sq1, uint8_t sq2) {
    return is_limited_checking(sq1,sq2,king_dirs);
}

bool ChessState::is_unlimited_checking(uint8_t sq1, uint8_t sq2,vector<pair<int8_t,int8_t>>& dirs) {
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

bool ChessState::is_bishop_checking(uint8_t sq1, uint8_t sq2) {
    return is_unlimited_checking(sq1,sq2,bishop_dirs);
}
bool ChessState::is_rook_checking(uint8_t sq1, uint8_t sq2) {
    return is_unlimited_checking(sq1,sq2,rook_dirs);
}
bool ChessState::is_queen_checking(uint8_t sq1, uint8_t sq2) {
    return is_unlimited_checking(sq1,sq2,queen_dirs);
}