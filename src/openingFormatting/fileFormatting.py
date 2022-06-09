import string, os

# parsing grandmaster games in a way that my program can read

def readFile(path):
    with open(path, "rt") as f:
        return f.read()

def writeFile(path, contents):
    with open(path, "wt") as f:
        f.write(contents)

def formatGame(contents):
    result = ""
    for line in contents.splitlines():
        if(line in string.whitespace):
            result += "\n"
        elif(line[0].isnumeric()):
            result += line
            result += " "
    finalResult = ""
    for line in result.splitlines():
        if(line == ''): continue
        newLine = ""
        for move in line.split(" "):
            if(move == "1-0" or move == "0-1" or move == "1/2-1/2" or move.isspace()):
                continue
            if("." in move):
                move = move[move.index(".")+1:]
            newLine += move + " "
        finalResult += newLine + "\n"
    return finalResult

def truncateOpeningBook(contents):
    result = ""
    for line in contents.splitlines():
        count = 0
        for move in line.split(" "):
            if(count == 12):
                break
            result += move + " "
            count += 1
        result += '\n'
    return result

def removeDuplicateGames(contents):
    result = ""
    for line in contents.splitlines():
        if(line not in result):
            result += line + '\n'
    return result

def formatOpening(path, newPath):
    contents = readFile(path)
    contents = formatGame(contents)
    contents = truncateOpeningBook(contents)
    contents = removeDuplicateGames(contents)
    writeFile(newPath, contents)

def listFiles(path):
    if os.path.isfile(path):
        # Base Case: return a list of just this file
        return [ path ]
    else:
        # Recursive Case: create a list of all the recursive results from
        # all the folders and files in this folder
        files = [ ]
        for filename in os.listdir(path):
            files += listFiles(path + '/' + filename)
        return files

# allFiles = listFiles("openingBook/unformattedOpenings")

# for path in allFiles:
#     newPath = path.replace("unformattedOpenings", "formattedOpenings")
#     formatOpening(path, newPath)

allFiles = listFiles("openingBook/formattedOpenings")

for path in allFiles:
    newPath = "openingBook/formattedOpenings/openingBook.pgn"
    name = path.replace("openingBook/formattedOpenings/", "")
    name = name.replace(".pgn", "")
    name = ":" + name
    writeFile(newPath, readFile(newPath) + readFile(path))