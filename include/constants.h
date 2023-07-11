#pragma once

#include <string>

namespace constants
{

const std::string STARTFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const std::string RANKS = "12345678";
const std::string FILES = "abcdefgh";

const size_t NUM_PIECE_TYPES = 12; // white knight, black king, etc...

enum Square : uint32_t { A1, B1, C1, D1, E1, F1, G1, H1,
                                 A2, B2, C2, D2, E2, F2, G2, H2,
                                 A3, B3, C3, D3, E3, F3, G3, H3,
                                 A4, B4, C4, D4, E4, F4, G4, H4,
                                 A5, B5, C5, D5, E5, F5, G5, H5,
                                 A6, B6, C6, D6, E6, F6, G6, H6,
                                 A7, B7, C7, D7, E7, F7, G7, H7,
                                 A8, B8, C8, D8, E8, F8, G8, H8, NONE };

enum Rank : uint32_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
enum File : uint32_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };


}