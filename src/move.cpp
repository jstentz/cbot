#include "include/move.h"

Move::Move() {}

Move::Move(int from, int to, int flags)
{
  m_move = (from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0xF) << 12);
}

Move::Move(int from, int to, int flags, int score)
{
  m_move = (from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0xF) << 12) | ((score & 0xFFFF) << 16);
}

Move::Move(int move)
{
  m_move = move;
}

int Move::from() const
{
  return m_move & 0x3F;
}

int Move::to() const
{
  return (m_move >> 6) & 0x3F;
}

int Move::type() const
{
  return (m_move >> 12) & 0xF;
}

bool Move::is_capture() const
{
  return m_move & 0x4000;
}

bool Move::is_promo() const
{
  return m_move & 0x8000;
}

int Move::score() const
{
  return m_move >> 16;
}

void Move::set_score(int score)
{
  m_move &= 0x0000FFFF; 
  m_move |= score << 16;
}

int Move::get_move() const
{
  return m_move;
}

bool Move::is_no_move() const
{
  return m_move == NO_MOVE;
}

bool Move::operator==(Move other_move) const
{
  return (to() == other_move.to()) && (from() == other_move.from()) && (type() == other_move.type());
}