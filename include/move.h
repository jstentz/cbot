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

  inline Move() {}

  inline Move(int from, int to, int flags)
  {
    m_move = (from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0xF) << 12);
  }

  inline Move(int from, int to, int flags, int score)
  {
    m_move = (from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0xF) << 12) | ((score & 0xFFFF) << 16);
  }

  inline Move(int move)
  {
    m_move = move;
  }

  inline int from() const
  {
    return m_move & 0x3F;
  }

  inline int to() const
  {
    return (m_move >> 6) & 0x3F;
  }

  inline int type() const
  {
    return (m_move >> 12) & 0xF;
  }

  inline bool is_capture() const
  {
    return m_move & 0x4000;
  }

  inline bool is_promo() const
  {
    return m_move & 0x8000;
  }

  inline int score() const
  {
    return m_move >> 16;
  }

  inline void set_score(int score)
  {
    m_move &= 0x0000FFFF; 
    m_move |= score << 16;
  }

  inline int get_move() const
  {
    return m_move;
  }

  inline bool is_no_move() const
  {
    return m_move == NO_MOVE;
  }

  inline bool operator==(Move other_move) const
  {
    return (to() == other_move.to()) && (from() == other_move.from()) && (type() == other_move.type());
  }

private:
  int m_move{};
};