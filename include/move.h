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
    QUIET_MOVE,
    DOUBLE_PUSH,
    KING_SIDE_CASTLE,
    QUEEN_SIDE_CASTLE,
    NORMAL_CAPTURE,
    EN_PASSANT_CAPTURE,
    KNIGHT_PROMO,
    BISHOP_PROMO,
    ROOK_PROMO,
    QUEEN_PROMO,
    KNIGHT_PROMO_CAPTURE,
    BISHOP_PROMO_CAPTURE,
    ROOK_PROMO_CAPTURE,
    QUEEN_PROMO_CAPTURE
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