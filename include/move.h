#pragma once

/**
 * @brief Representation for a move. The first 16 bits hold the from square, 
 * the to square, and the move type. This is signed so the higher order bits can
 * give us moves with a negative score, therefore easier sorting.
 * 
 */
class Move
{
public:
  // move types, needs to be exposed so others can see
  enum MoveType : int
  {
    QUIET_MOVE =           0x0,
    DOUBLE_PUSH =          0x1,
    KING_SIDE_CASTLE =     0x2,
    QUEEN_SIDE_CASTLE =    0x3,
    NORMAL_CAPTURE =       0x4,
    EN_PASSANT_CAPTURE =   0x5,
    KNIGHT_PROMO =         0x8,
    BISHOP_PROMO =         0x9,
    ROOK_PROMO =           0xA,
    QUEEN_PROMO =          0xB,
    KNIGHT_PROMO_CAPTURE = 0xC,
    BISHOP_PROMO_CAPTURE = 0xD,
    ROOK_PROMO_CAPTURE =   0xE,
    QUEEN_PROMO_CAPTURE =  0xF
  };

  const static int NO_MOVE = 0;

  Move();
  Move(int from, int to, int type);
  Move(int from, int to, int type, int score);
  Move(int move);

  int from() const;
  int to() const;
  int type() const;
  bool is_capture() const;
  bool is_promo() const;

  int score() const;
  void set_score(int score);

  bool is_no_move() const;

  int get_move() const;

  bool operator==(Move other_move) const;

private:
  int m_move{};
};