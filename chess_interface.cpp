#include "chess_interface.h"
#include <sstream>
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
int ChessInterface::get_sq(string str) {
    for(int i=0;i<SZ;i++) { // column
        if(str[0]==cols[i])
            return (SZ-(str[1]-'0'))*SZ+i;
    }
    return SZ*SZ; // mistake, throw error
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
    else if(regex_match(str,regex("^[NBRKQ]x?[abcdefgh][0-8]$"))) { // piece moves, maybe captures, no ambiguity
        regex_search(str,sm,regex("[abcdefgh][0-8]"));
        uint8_t sq2 = get_sq(sm[0]);

        uint8_t piece = map_piece(active,str[0]);
        uint8_t sq1 = attack_sq(sq2,piece,str[0]);

        return ((minfo){sq1,sq2,piece,NCAST});
    }
    // rank disambiguation
        else if(regex_match(str,regex("^[NBRKQ][0-8]x?[abcdefgh][0-8]$"))) {
        regex_search(str,sm,regex("[abcdefgh][0-8]"));
        uint8_t sq2 = get_sq(sm[0]);

        uint8_t piece = map_piece(active,str[0]);
        regex_search(str,sm,regex("[0-8]"));
        uint8_t sq1 = attack_sq(sq2,piece,str[0],SZ-(str[1]-'0'),SZ);
        
        return ((minfo){sq1,sq2,piece,NCAST});
    }
    // file disambiguation
    else if(regex_match(str,regex("^[NBRKQ][abcdefgh]x?[abcdefgh][0-8]$"))) {
        regex_search(str,sm,regex("[abcdefgh][0-8]"));
        uint8_t sq2 = get_sq(sm[0]);

        uint8_t piece = map_piece(active,str[0]);
        regex_search(str,sm,regex("[abcdefgh]"));
        uint8_t sq1 = attack_sq(sq2,piece,str[0],SZ,get_sq(sm[0].str()+"1")%SZ);

        return ((minfo){sq1,sq2,piece,NCAST});
    }
    // square disambiguation
    else if(regex_match(str,regex("^[NBRKQ][abcdefgh][0-8]x?[abcdefgh][0-8]$"))) {
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