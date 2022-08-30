#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include "openings.h" /* just include this to get the split function */

using namespace std;

bool PLAY_MODE;
bool TEST_MODE;
string FEN_STRING;
bool AIVSAI;
int SEARCH_TIME;

void load_settings_from_config() {
    ifstream config_file("assets/config.txt");
    string line;
    string setting;

    getline(config_file, line);
    setting = split(line, ':')[1];
    if(setting == "Play") {PLAY_MODE = true; TEST_MODE = false;}
    else if(setting == "Test") {PLAY_MODE = false; TEST_MODE = true;}

    getline(config_file, line);
    setting = split(line, ':')[1];
    FEN_STRING = setting;

    getline(config_file, line);
    setting = split(line, ':')[1];
    if(setting == "True") AIVSAI = true;
    else AIVSAI = false;

    getline(config_file, line);
    setting = split(line, ':')[1];
    SEARCH_TIME = stoi(setting);

    config_file.close();
}