/*
Structure of Config File:
Mode:"Test"/"Play"
FEN:*Fen string*
AIvsAI:"False"/"True"
Search Time: *miliseconds*
*/
#pragma once

#include <string>
using namespace std;

/* Config Settings */
extern bool PLAY_MODE;
extern bool TEST_MODE;
extern string FEN_STRING;
extern bool AIVSAI;
extern int SEARCH_TIME;

/**
 * @brief Loads the settings from the config file.
 * 
 */
void load_settings_from_config();