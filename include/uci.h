#pragma once

#include "include/board.h"
#include "include/search.h"

/**
 * Reference: https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf
 * 
 * Rules of UCI:
 * All communication is done via stdin and stdout
 * Must ALWAYS be able to process incoming commands while also thinking (threading?)
 * All sent and received commands should end with "\n"
 * Arbitrary whitespace is allowed... how am I going to handle that?
 * Never start pondering or calculating until receiving a "go" command 
 * Before a "go" command, there will always be a "position" command 
 * All of the opening book is done by the GUI, but you can set it so the engine uses it's own
 * If the engine or GUI receives an unknown command, it should just keep reading and try to parse the rest of it
 * If the engine receives a command which is not supposed to come, it should just ignore it ("stop" when not calculating)
 * 
 * 
 * Flow of UCI:
 * Engine: Boot and wait for either "isready" or "setoption" commands to set engine parameters
 * 
 * Move formats: long algebraic notation; nullmove is 0000 (as a string?)
 * 
 * Example:
 *  GUI: tell the engine to use the UCI protocol
    uci

    // ENGINE: identify  
    id name Chess Engine
    id author John Smith

    // ENGINE: send the options that can be changed
    //         in this case the hash size can have a value from 1 to 128 MB
    option name Hash type spin default 1 min 1 max 128

    // ENGINE: sent all parameters and is ready
    uciok

    // GUI: set hash to 32 MB
    setoption name Hash value 32

    // GUI: waiting for the engine to finish initializing
    isready

    // ENGINE: finished setting up the internal values and is ready to start
    readyok

    // GUI: let the engine know if starting a new game
    ucinewgame

    // GUI: tell the engine the position to search
    position startpos moves e2e4

    // GUI: tell the engine to start searching
    //      in this case give it the timing information in milliseconds
    go wtime 122000 btime 120000 winc 2000 binc 2000

    // ENGINE: send search information continuously during search
    //         this includes depth, search value, time, nodes, speed, and pv line
    info depth 1 score cp -1 time 10 nodes 26 nps 633 pv e7e6
    info depth 2 score cp -38 time 22 nodes 132 nps 2659 pv e7e6 e2e4
    info depth 3 score cp -6 time 31 nodes 533 nps 10690 pv d7d5 e2e3 e7e6
    info depth 4 score cp -30 time 55 nodes 1292 nps 25606 pv d7d5 e2e3 e7e6 g1f3

    // ENGINE: return the best move found
    bestmove d7d5
*/

#include <vector>
#include <string>

class UCICommunicator
{
public:
  UCICommunicator() : m_board{std::make_shared<Board>()}, m_searcher{m_board}, m_move_gen{m_board} {}
  ~UCICommunicator() {}

  void start_uci_communication();

private:
  Board::Ptr m_board;
  Searcher m_searcher;
  MoveGenerator m_move_gen; /// TODO: we don't need this 


  void handle_uci();
  void handle_is_ready();
  void handle_new_game();
  void handle_position(std::vector<std::string>& parsed_cmd, std::string& cmd);
  void handle_go(std::vector<std::string>& parsed_cmd, std::string& cmd);
  void handle_stop();

  void handle_quit();

  /* My own commands */
  void handle_verify(std::vector<std::string>& parsed_cmd); /* takes in a depth param */
  void handle_show();


  /* GUI -> ENGINE COMMANDS */
  inline static const std::string UCI = "uci";
  inline static const std::string ISREADY = "isready";
  inline static const std::string SETOPTION = "setoption";
  inline static const std::string UCINEWGAME = "ucinewgame";
  inline static const std::string POSITION = "position";
  inline static const std::string QUIT = "quit";
  inline static const std::string GO = "go";
  inline static const std::string PERFT = "perft";
  inline static const std::string STOP = "stop";
  inline static const std::string MOVETIME = "movetime";

  /* ENGINE -> GUI COMMANDS */
  inline static const std::string UCIOK = "uciok\n";
  inline static const std::string READYOK = "readyok\n";
  inline static const std::string ID_NAME = "id name cbot\n";
  inline static const std::string ID_AUTHOR = "id author Jason Stentz\n";

  /* other constants */
  inline static const std::string STARTPOS = "startpos";
  inline static const std::string STARTFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  inline static const std::string FEN = "fen"; 
  inline static const std::string VERIFY = "verify";
  inline static const std::string SHOW = "show";
};
