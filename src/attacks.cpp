#include "include/attacks.h"
#include "include/bitboard.h"
#include "include/board.h"
#include "include/pieces.h"
#include "include/utils.h"

#include <stddef.h>

// instantiates the lookup tables internal state 
LookUpTable::LookUpTable()
{
  bitboard rank = 0x00000000000000FF;
  bitboard file = 0x0101010101010101;
  /* creating rank and file masks and clears */
  for(size_t i = 0; i < 8; i++) {
    mask_rank[i] = rank;
    clear_rank[i] = ~rank;
    mask_file[i] = file;
    clear_file[i] = ~file;

    rank = rank << 8;
    file = file << 1;
  }

  /* creating diagonal masks */
  /* a diagonal is identified by file - rank if file >= rank else 15 - (rank - file) */
  mask_diagonal[0] = 0x8040201008040201;
  mask_diagonal[1] = 0x0080402010080402;
  mask_diagonal[2] = 0x0000804020100804;
  mask_diagonal[3] = 0x0000008040201008;
  mask_diagonal[4] = 0x0000000080402010;
  mask_diagonal[5] = 0x0000000000804020;
  mask_diagonal[6] = 0x0000000000008040;
  mask_diagonal[7] = 0x0000000000000080;
  mask_diagonal[8] = 0x0100000000000000;
  mask_diagonal[9] = 0x0201000000000000;
  mask_diagonal[10] = 0x0402010000000000;
  mask_diagonal[11] = 0x0804020100000000;
  mask_diagonal[12] = 0x1008040201000000;
  mask_diagonal[13] = 0x2010080402010000;
  mask_diagonal[14] = 0x4020100804020100;

  /* creating antidiagonal masks */
  /* an antidiagonal is identified by (7 - file) - rank if file >= rank else rank - (7 - file) + 1*/
  mask_antidiagonal[0] = 0x0102040810204080;
  mask_antidiagonal[1] = 0x0001020408102040;
  mask_antidiagonal[2] = 0x0000010204081020;
  mask_antidiagonal[3] = 0x0000000102040810; 
  mask_antidiagonal[4] = 0x0000000001020408;
  mask_antidiagonal[5] = 0x0000000000010204;
  mask_antidiagonal[6] = 0x0000000000000102;
  mask_antidiagonal[7] = 0x0000000000000001;
  mask_antidiagonal[8] = 0x8000000000000000;
  mask_antidiagonal[9] = 0x4080000000000000;
  mask_antidiagonal[10] = 0x2040800000000000;
  mask_antidiagonal[11] = 0x1020408000000000;
  mask_antidiagonal[12] = 0x0810204080000000;
  mask_antidiagonal[13] = 0x0408102040800000;
  mask_antidiagonal[14] = 0x0204081020408000;

  /* creating piece masks */
  bitboard piece = 0x0000000000000001;
  for(size_t i = 0; i < 64; i++) {
    pieces[i] = piece;
    piece = piece << 1;
  }

  /* creating knight_attacks LUT */
  bitboard spot_1_clip = clear_file[constants::FILE_H] & clear_file[constants::FILE_G];
  bitboard spot_2_clip = clear_file[constants::FILE_H];
  bitboard spot_3_clip = clear_file[constants::FILE_A];
  bitboard spot_4_clip = clear_file[constants::FILE_A] & clear_file[constants::FILE_B];

  bitboard spot_5_clip = spot_4_clip;
  bitboard spot_6_clip = spot_3_clip;
  bitboard spot_7_clip = spot_2_clip;
  bitboard spot_8_clip = spot_1_clip;

  bitboard spot_1;
  bitboard spot_2;
  bitboard spot_3;
  bitboard spot_4;
  bitboard spot_5;
  bitboard spot_6;
  bitboard spot_7;
  bitboard spot_8;

  bitboard knight;
  for(size_t sq = 0; sq < 64; sq++) {
    knight = pieces[sq];
    spot_1 = (knight & spot_1_clip) >> 6;
    spot_2 = (knight & spot_2_clip) >> 15;
    spot_3 = (knight & spot_3_clip) >> 17;
    spot_4 = (knight & spot_4_clip) >> 10;

    spot_5 = (knight & spot_5_clip) << 6;
    spot_6 = (knight & spot_6_clip) << 15;
    spot_7 = (knight & spot_7_clip) << 17;
    spot_8 = (knight & spot_8_clip) << 10;
    knight_attacks[sq] = spot_1 | spot_2 | spot_3 | spot_4 | spot_5 | spot_6 |
                   spot_7 | spot_8;
  }

  /* creating king_attacks LUT */
  spot_1_clip = clear_file[constants::FILE_A];
  spot_3_clip = clear_file[constants::FILE_H];
  spot_4_clip = clear_file[constants::FILE_H];

  spot_5_clip = clear_file[constants::FILE_H];
  spot_7_clip = clear_file[constants::FILE_A];
  spot_8_clip = clear_file[constants::FILE_A];

  bitboard king;
  for(size_t sq = 0; sq < 64; sq++) {
    king = pieces[sq];
    spot_1 = (king & spot_1_clip) << 7;
    spot_2 = king << 8;
    spot_3 = (king & spot_3_clip) << 9;
    spot_4 = (king & spot_4_clip) << 1;

    spot_5 = (king & spot_5_clip) >> 7;
    spot_6 = king >> 8;
    spot_7 = (king & spot_7_clip) >> 9;
    spot_8 = (king & spot_8_clip) >> 1;
    king_attacks[sq] = spot_1 | spot_2 | spot_3 | spot_4 | spot_5 | spot_6 |
                 spot_7 | spot_8;
  }

  /* creating white_pawn_attacks LUT */
  bitboard pawn;
  spot_1_clip = clear_file[constants::FILE_A];
  spot_4_clip = clear_file[constants::FILE_H];
  for(size_t sq = 0; sq < 64; sq++) {
    pawn = pieces[sq];
    spot_1 = (pawn & spot_1_clip) << 7;
    spot_4 = (pawn & spot_4_clip) << 9;
    white_pawn_attacks[sq] = spot_1 | spot_4;
  }

  /* creating black_pawn_attacks LUT */
  spot_1_clip = clear_file[constants::FILE_A];
  spot_4_clip = clear_file[constants::FILE_H];
  for(size_t sq = 0; sq < 64; sq++) {
    pawn = pieces[sq];
    spot_1 = (pawn & spot_4_clip) >> 7;
    spot_4 = (pawn & spot_1_clip) >> 9;
    black_pawn_attacks[sq] = spot_1 | spot_4;
  }

  /* 
    creating white_pawn_pushes LUT 
    does not include double pawn pushes for 2nd rank pawns
    that calculation is done in the generate_pawn_moves function
  */
  for(size_t sq = 0; sq < 64; sq++) {
    pawn = pieces[sq];
    spot_2 = pawn << 8;
    white_pawn_pushes[sq] = spot_2;
  }

  /* 
    creating black_pawn_pushes LUT 
    does not include double pawn pushes for 7th rank pawns
    that calculation is done in the generate_pawn_moves function
  */
  for(size_t sq = 0; sq < 64; sq++) {
    pawn = pieces[sq];
    spot_2 = pawn >> 8;
    black_pawn_pushes[sq] = spot_2;
  }

  /* creating rank_attacks LUT */  
  size_t LSB_to_first_1;
  size_t LSB_to_second_1;
  bitboard mask_1 = 0x1;
  bitboard rank_attack_mask = 0xFF; // shift this at end to get attacks
  for(size_t sq = 0; sq < 64; sq++) {
    size_t file = utils::file(sq); // will have to multiply later to shift the bitboard into the right spot
    for(size_t pattern = 0; pattern < 256; pattern++) {
      LSB_to_first_1 = 0;
      LSB_to_second_1 = 7;
      for(size_t i = 0; i < file; i++) {
        if((pattern >> i) & mask_1 == 1) {
          LSB_to_first_1 = i;
        }
      }
      for(size_t i = file + 1; i < 8; i++) {
        if((pattern >> i) & mask_1 == 1) {
          LSB_to_second_1 = i;
          break;
        }
      }
      bitboard unplaced_mask = rank_attack_mask >> (7 - (LSB_to_second_1 - LSB_to_first_1));
      rank_attacks[sq][pattern] = (unplaced_mask << (sq - (file - LSB_to_first_1))) 
                        & ~pieces[sq]; // removes piece square
    }
  }

  /* creating file_attacks LUT */
  for(size_t i = 0; i < 8; i++) {
    for(size_t j = 0; j < 8; j++){
      for(size_t pattern = 0; pattern < 256; pattern++) {
        file_attacks[i*8 + j][pattern] = rotate_90_clockwise(rank_attacks[j*8 + (7-i)][pattern]);
      }
    }
    
  }

  /* creating diagonal_attacks LUT */
  size_t rotated_rank;
  size_t rotated_sq;
  size_t diag;
  for(size_t i = 0; i < 8; i++) {
    for(size_t j = 0; j < 8; j++){
      if(i >= j) rotated_rank = i - j;
      else       rotated_rank = 8 - (j - i);
      rotated_sq = rotated_rank * 8 + j;
      if(j >= i) diag = j - i;
      else       diag = 15 - (i - j);
      size_t sq = i * 8 + j;
      for(size_t pattern = 0; pattern < 256; pattern++) {
        /* still need to mask the correct diagonal */
        diagonal_attacks[sq][pattern] = 
          undo_pseudo_rotate_45_clockwise(rank_attacks[rotated_sq][pattern])
          & mask_diagonal[diag] & ~pieces[sq]; // removes piece square
      }
    }
    
  }

  /* creating antidiagonal_attacks LUT */
  for(size_t i = 0; i < 8; i++) {
    for(size_t j = 0; j < 8; j++){
      if(j > (7 - i)) diag = 15 - (j - (7 - i));
      else             diag = (7 - i) - j;
      if(diag == 0)     rotated_rank = 0;
      else if(diag <= 7) rotated_rank = 8 - diag;
      else              rotated_rank = 8 - (diag - 7);
      rotated_sq = rotated_rank * 8 + j;
      size_t sq = i * 8 + j;
      for(size_t pattern = 0; pattern < 256; pattern++) {
        /* still need to mask the correct diagonal */
        antidiagonal_attacks[sq][pattern] = 
          undo_pseudo_rotate_45_anticlockwise(rank_attacks[rotated_sq][pattern])
          & mask_antidiagonal[diag] & ~pieces[sq]; // removes piece square
      }
    }
  }
}

bitboard LookUpTable::get_knight_attacks(int sq) const
{
  // doesn't depend on the current position
  return knight_attacks[sq];
}

bitboard LookUpTable::get_king_attacks(int sq) const
{
  // doesn't depend on the current position
  return king_attacks[sq];
}

bitboard LookUpTable::get_pawn_attacks(int sq, bool white_side) const
{
  // depends on who's turn it is to move
  if(white_side) return white_pawn_attacks[sq];
  return black_pawn_attacks[sq];
}

bitboard LookUpTable::get_rook_attacks(int rook_sq, bitboard blockers) const
{
  // depends on the placement of blockers on the board
  // could leave out the white king when calculating blacks attack maps
  size_t rook_rank = utils::rank(rook_sq);
  size_t rook_file = utils::file(rook_sq);
  bitboard rank_pattern = (blockers & mask_rank[rook_rank]) >> (rook_rank * 8);
  bitboard file_pattern = rotate_90_anticlockwise(blockers & mask_file[rook_file]) >> (rook_file * 8);
  return rank_attacks[rook_sq][rank_pattern] |
       file_attacks[rook_sq][file_pattern];
}

bitboard LookUpTable::get_bishop_attacks(int bishop_sq, bitboard blockers) const
{
  // depends on the placement of blockers on the board
  // could leave out the white king when calculating blacks attack maps
  // this code is literal dog shit please clean it up later jason...
  size_t bishop_rank = utils::rank(bishop_sq);
  size_t bishop_file = utils::file(bishop_sq);
  size_t bishop_diag;
  if(bishop_file >= bishop_rank) bishop_diag = bishop_file - bishop_rank;
  else                           bishop_diag = 15 - (bishop_rank - bishop_file);
  // do same for antidiag
  size_t bishop_antidiag;
  if(bishop_file > (7 - bishop_rank)) bishop_antidiag = 15 - (bishop_file - (7 - bishop_rank));
  else                                bishop_antidiag = (7 - bishop_rank) - bishop_file;

  size_t rotated_rank;
  if(bishop_diag == 0) rotated_rank = 0;
  else if (bishop_diag < 8) rotated_rank = 8 - bishop_diag;
  else rotated_rank = 8 - (bishop_diag - 7);
  bitboard diag_pattern = pseudo_rotate_45_clockwise(blockers & mask_diagonal[bishop_diag]) >> (8 * rotated_rank);
  size_t antirotated_rank;
  if(bishop_antidiag == 0)      antirotated_rank = 0;
  else if(bishop_antidiag < 8)  antirotated_rank = 8 - bishop_antidiag;
  else                          antirotated_rank = 8 - (bishop_antidiag - 7);
  bitboard antidiag_pattern = pseudo_rotate_45_anticlockwise(blockers & mask_antidiagonal[bishop_antidiag]) >> (8 * antirotated_rank);
  return diagonal_attacks[bishop_sq][diag_pattern] |
         antidiagonal_attacks[bishop_sq][antidiag_pattern];
}

bitboard LookUpTable::get_queen_attacks(int queen_sq, bitboard blockers) const 
{
  return get_bishop_attacks(queen_sq, blockers) |
         get_rook_attacks(queen_sq, blockers);
}

bitboard LookUpTable::get_ray_from_bishop_to_king(int bishop_sq, int king_sq) const
{
  bitboard board = pieces[bishop_sq] | pieces[king_sq];
  bitboard bishop_attacks_from_bishop = get_bishop_attacks(bishop_sq, board);
  bitboard bishop_attacks_from_king = get_bishop_attacks(king_sq, board);
  return (bishop_attacks_from_bishop & bishop_attacks_from_king) | board;
}

bitboard LookUpTable::get_ray_from_rook_to_king(int rook_sq, int king_sq) const
{
  // assume the board is empty for calculating the rays
  bitboard board = pieces[rook_sq] | pieces[king_sq];
  bitboard rook_attacks_from_rook = get_rook_attacks(rook_sq, board);
  bitboard rook_attacks_from_king = get_rook_attacks(king_sq, board);
  return (rook_attacks_from_rook & rook_attacks_from_king) | board;
}

bitboard LookUpTable::get_ray_from_queen_to_king(int queen_sq, int king_sq) const
{
  if((utils::file(queen_sq) == utils::file(king_sq)) || (utils::rank(queen_sq) == utils::rank(king_sq))) {
    return get_ray_from_rook_to_king(queen_sq, king_sq); // if on the same rank or file, treat the queen as a rook
  }
  return get_ray_from_bishop_to_king(queen_sq, king_sq); // if on the same diagonal, treat the queen as a bishop
}

bitboard LookUpTable::get_ray_from_sq_to_sq(int start, int target) const
{
  return get_ray_from_queen_to_king(start, target);
}

bitboard LookUpTable::get_rank_mask(int rank) const
{
  return mask_rank[rank];
}

bitboard LookUpTable::get_pawn_pushes(int sq, bool white) const
{
  return white ? white_pawn_pushes[sq] : black_pawn_pushes[sq];
}

bitboard LookUpTable::get_pawn_attacks(int sq, bool white) const
{
  return white ? white_pawn_attacks[sq] : black_pawn_attacks[sq];
}