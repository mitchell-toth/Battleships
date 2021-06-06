/**
 * @brief USSWhite AI for battleships
 * @file USSWhite.cpp
 * @author Stefan Brandle, Jonathan Geisler
 * Mitchell Toth, Alec Houseman
 * @date September, 2004 Updated 2015 for multi-round play.
 *
 * The constructor
 */

#include <iostream>
#include <cstdio>
#include <vector>

#include "USSWhite.h"


/**
 * @brief Constructor that initializes any inter-round data structures.
 * @param boardSize Indication of the size of the board that is in use.
 *
 * The constructor runs when the AI is instantiated (the object gets created)
 * and is responsible for initializing everything that needs to be initialized
 * before any of the rounds happen. The constructor does not get called 
 * before rounds; newRound() gets called before every round.
 */
USSWhite::USSWhite( int boardSize )
    :PlayerV2(boardSize)
{
    // Could do any initialization of inter-round data structures here.
    this->initializeBoard();

    gamesPlayed = 0;
    huntingAnEnemyShip = false;
    resetShotBools();
    SHIP_PLACEMENT_low = false;
    SHIP_PLACEMENT_unlikely = true;
    SHIP_PLACEMENT_learning = false;
    doMiddleScan = false;
    doProbabilityScan = true;

    if (doMiddleScan) {
        lastRow = int(boardSize/2)-1;
        lastCol = 0-MIN_SHIP_SIZE;
    }
    else {
        lastRow = 0;
        lastCol = 0-MIN_SHIP_SIZE;
    }
    numShipsPlaced = 0;

    resetBoards();

    resetBoard(enemyShotsIncrementBoard);
    resetBoard(enemyShotsIncrementBoardCopy);
}

/**
 * @brief Destructor placeholder.
 * If your code does anything that requires cleanup when the object is
 * destroyed, do it here in the destructor.
 */
USSWhite::~USSWhite( ) {}

/*
 * Private internal function that initializes a MAX_BOARD_SIZE 2D array of char to water.
 */
void USSWhite::initializeBoard() {
    for(int row=0; row<boardSize; row++) {
        for(int col=0; col<boardSize; col++) {
            this->board[row][col] = WATER;
        }
    }
}


/**
 * @brief Specifies the AI's shot choice and returns the information to the caller.
 * @return Message The most important parts of the returned message are 
 * the row and column values. 
 *
 * See the Message class documentation for more information on the 
 * Message constructor.
 */
Message USSWhite::getMove() {

    initializeProbabilities(probabilityBoard, myShotsBoard);
    overallShotCount++;
    //addToProbabilitiesBoard();

    //If killed enemy ship
    if (board[lastRow][lastCol] == KILL) {
        huntingAnEnemyShip = false;
        resetShotBools();
        lastRow = tempLastRow;
        lastCol = tempLastCol;
    }
    

    //If found enemy ship (or more of it) with last position
    if (board[lastRow][lastCol] == HIT) {

        huntingAnEnemyShip = true;        
        lastRow = tempLastRow;
        lastCol = tempLastCol;
        if (shotVertical) {
            enemyShipIsVertical = true;
        }
        else if (shotHorizontal) {
            enemyShipIsHorizontal = true;
        }
        return branchOut();
    }


    //If miss while hunting an enemy ship, go back to unbranched position and try new branch
    if (huntingAnEnemyShip && board[lastRow][lastCol] == MISS) {
        lastRow = tempLastRow;
        lastCol = tempLastCol;
        return branchOut();
    }


    //If not hunting but theres a hit to yet be explored. Go there
    if (unpursuedHit()) {
        huntingAnEnemyShip = true;
        //Find the hit
        for (int row=0; row < boardSize; row++) {
            for (int col=0; col < boardSize; col++) {
                if (board[row][col] == HIT) {
                    lastRow = row;
                    lastCol = col;
                    tempLastRow = lastRow;
                    tempLastCol = lastCol;
                    return branchOut();
                }
            }
        }
    }

    return RegularScanMove();
}


Message USSWhite::RegularScanMove() {
    if (doProbabilityScan) {
        return getProbabilityScanMove();
    }
    else {
        return getCleanPlayerScanMove();
    }
}


Message USSWhite::getCleanPlayerScanMove() {
    lastCol += MIN_SHIP_SIZE;

    //Rap around: column. Go to new row
    if (lastCol >= boardSize) {
        lastRow++;
        //Rap around: row. Go back to start
        if (lastRow >= boardSize) {
            lastRow = 0;
        }
        lastCol = lastRow % MIN_SHIP_SIZE;
    }


    //Alter the shot if we've already tried this spot
    while (true) {
        if (myShotsBoard[lastRow][lastCol] == WATER) {  //Not a duplicate
            if (lastRow != 0 && myShotsBoard[lastRow-1][lastCol] == MISS) {  //Good scan?
                adjustShotToBeNew();
                continue;
            }
            else {
                break;
            }
        }
        else {  //Already tried this spot
            adjustShotToBeNew();
        }
    }


    //Keep track of shot
    tempLastRow = lastRow;
    tempLastCol = lastCol;

    Message result( SHOT, lastRow, lastCol, "Bang", None, 1 );
    return result;
}


/**
 * @brief Tells the AI that a new round is beginning.
 * The AI show reinitialize any intra-round data structures.
 */
void USSWhite::newRound() {
    /* USSWhite is too simple to do any inter-round learning. Smarter players 
     * reinitialize any round-specific data structures here.
     */
    overallShotCount = 0;
    probabilityScanShotCount = 0;

    /*
    if (doMiddleScan) {
        lastRow = int(boardSize/2)-1;
        lastCol = 0-MIN_SHIP_SIZE;
    }
    else {
        lastRow = 0;
        lastCol = 0-MIN_SHIP_SIZE;
    }
    */

    numShipsPlaced = 0;
    initializeBoard();
    huntingAnEnemyShip = false;

    //Learning ship placement.
    /*
    updateEnemyShotsIncrementBoard();
    if (gamesPlayed > 1) {
        SHIP_PLACEMENT_learning = true;
    }
    */

    updateMyShotsIncrementBoard();
    //copyBoard(myShotsIncrementBoardCopy, myShotsIncrementBoard);

    resetShotBools();
    resetBoards();
    gamesPlayed++;

    copyBoard(enemyShotsIncrementBoardCopy, enemyShotsIncrementBoard);

    initializeProbabilities(probabilityBoard, myShotsBoard);
}

/**
 * @brief Gets the AI's ship placement choice. This is then returned to the caller.
 * @param length The length of the ship to be placed.
 * @return Message The most important parts of the returned message are 
 * the direction, row, and column values. 
 *
 * The parameters returned via the message are:
 * 1. the operation: must be PLACE_SHIP 
 * 2. ship top row value
 * 3. ship top col value
 * 4. a string for the ship name
 * 5. direction Horizontal/Vertical (see defines.h)
 * 6. ship length (should match the length passed to placeShip)
 */
Message USSWhite::placeShip(int length) {
    char shipName[10];
    //Initializing variables, ignore the values
    int topRow = 0;
    int topCol = 0;
    Direction direction = Vertical;
    int directionNum = 0;
    
    // Create ship names each time called: Ship0, Ship1, Ship2, ...
    snprintf(shipName, sizeof shipName, "Ship%d", numShipsPlaced);

    //Prefer lower spots
    if (SHIP_PLACEMENT_low) {
        return placeShipsLow(topRow, topCol, direction, directionNum, length, shipName);
    }

    //Prefer the edges
    else if (SHIP_PLACEMENT_unlikely) {
        return placeShipsInUnlikelySpots(topRow, topCol, direction, directionNum, length, shipName);
    }

    else if (SHIP_PLACEMENT_learning) {
        return placeShipsByLearning(topRow, topCol, direction, directionNum, length, shipName);
    }

    //Random ship placement
    else {
        return placeShipsRandomly(topRow, topCol, direction, directionNum, length, shipName);
    }
}

/**
 * @brief Updates the AI with the results of its shots and where the opponent is shooting.
 * @param msg Message specifying what happened + row/col as appropriate.
 */
void USSWhite::update(Message msg) {
    switch(msg.getMessageType()) {
	case HIT:
	case KILL:
	case MISS:
	    board[msg.getRow()][msg.getCol()] = msg.getMessageType();
        myShotsBoard[msg.getRow()][msg.getCol()] = msg.getMessageType();
        //probabilityBoard[msg.getRow()][msg.getCol()] = 0;
        //updateProbabilities(msg.getRow(), msg.getCol());
	    break;
	case WIN:
        break;
	case LOSE:
	    break;
	case TIE:
	    break;
	case OPPONENT_SHOT:
        enemyShotsBoard[msg.getRow()][msg.getCol()] = msg.getMessageType();
        //shotWeightMarker--;
	    break;
    }
}


void USSWhite::resetBoard(int Board[][MAX_BOARD_SIZE]) {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            Board[r][c] = 0;
        }
    }
}


void USSWhite::resetBoard(char Board[][MAX_BOARD_SIZE]) {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            Board[r][c] = WATER;
        }
    }
}


Message USSWhite::branchOut() {

    int numSpacesVertical = findNumSpacesVertical(lastRow, lastCol);
    int numSpacesHorizontal = findNumSpacesHorizontal(lastRow, lastCol);

    if (! shotHorizontal && ! shotVertical && numSpacesVertical < MIN_SHIP_SIZE-1) {
        enemyShipIsHorizontal = true;
    }
    else if (! shotHorizontal && ! shotVertical && numSpacesHorizontal < MIN_SHIP_SIZE-1) {
        enemyShipIsVertical = true;
    }


    if (enemyShipIsVertical) {  //if suspected that enemy ship is vertical, change up order
        //Shoot up
        for (int row = lastRow-1; row >= 0; row--) {
            if (board[row][lastCol] == WATER) {
                lastRow = row;
                Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                return attemptShot;
            }
            else if (board[row][lastCol] == HIT) {
                continue;
            }
            else {
                break;
            }
        }

        //Shoot down
        for (int row = lastRow+1; row < boardSize; row++) {
            if (board[row][lastCol] == WATER) {
                lastRow = row;
                Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                return attemptShot;
            }
            else if (board[row][lastCol] == HIT) {
                continue;
            }
            else {
                break;
            }
        }

        //Shoot right
        for (int col = lastCol+1; col < boardSize; col++) {
            //If no chance.
            if (probabilityBoard[lastRow][col] == 0) {
                break;
            }
            else if (board[lastRow][col] == WATER) {
                lastCol = col;
                Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                return attemptShot;
            }
            else if (board[lastRow][col] == HIT) {
                continue;
            }
            else {
                break;
            }
        }

        //Shoot left
        for (int col = lastCol-1; col >= 0; col--) {
            //If no chance.
            if (probabilityBoard[lastRow][col] == 0) {
                break;
            }
            else if (board[lastRow][col] == WATER) {
                lastCol = col;
                Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                return attemptShot;
            }
            else if (board[lastRow][col] == HIT) {
                continue;
            }
            else {
                break;
            }
        }
    }


    else if (enemyShipIsHorizontal) {
        //Shoot right
        for (int col = lastCol+1; col < boardSize; col++) {
            if (board[lastRow][col] == WATER) {
                lastCol = col;
                Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                return attemptShot;
            }
            else if (board[lastRow][col] == HIT) {
                continue;
            }
            else {
                break;
            }
        }

        //Shoot left
        for (int col = lastCol-1; col >= 0; col--) {
            if (board[lastRow][col] == WATER) {
                lastCol = col;
                Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                return attemptShot;
            }
            else if (board[lastRow][col] == HIT) {
                continue;
            }
            else {
                break;
            }
        }

        //Shoot up
        for (int row = lastRow-1; row >= 0; row--) {
            //If no chance.
            if (probabilityBoard[row][lastCol] == 0) {
                break;
            }
            if (board[row][lastCol] == WATER) {
                lastRow = row;
                Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                return attemptShot;
            }
            else if (board[row][lastCol] == HIT) {
                continue;
            }
            else {
                break;
            }
        }

        //Shoot down
        for (int row = lastRow+1; row < boardSize; row++) {
            //If no chance.
            if (probabilityBoard[row][lastCol] == 0) {
                break;
            }
            else if (board[row][lastCol] == WATER) {
                lastRow = row;
                Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                return attemptShot;
            }
            else if (board[row][lastCol] == HIT) {
                continue;
            }
            else {
                break;
            }
        }
    }




    /*
    //If exploring a single hit.
    if ((! shotHorizontal) && (! shotVertical)) {
        //Pick where to initially try based on probability.
        int probabilityUp = 0;
        int probabilityDown = 0;
        int probabilityRight = 0;
        int probabilityLeft = 0;
        if (lastRow-1 >= 0) {
            probabilityUp = probabilityBoard[lastRow-1][lastCol];
        }
        if (lastRow+1 < boardSize) {
            probabilityDown = probabilityBoard[lastRow+1][lastCol];
        }
        if (lastCol+1 < boardSize) {
            probabilityRight = probabilityBoard[lastRow][lastCol+1];
        }
        if (lastCol-1 >= 0) {
            probabilityLeft = probabilityBoard[lastRow][lastCol-1];
        }

        //Up is best.
        if (probabilityUp >= probabilityDown && probabilityUp >= probabilityRight && probabilityUp >= probabilityLeft) {
            //Shoot up
            for (int row = lastRow-1; row >= 0; row--) {
                if (board[row][lastCol] == WATER) {
                    lastRow = row;
                    shotHorizontal = false;
                    shotVertical = true;
                    Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                    return attemptShot;
                }
                else if (board[row][lastCol] == HIT) {
                    continue;
                }
                else {
                    break;
                }
            }
        }
        //Down is best.
        if (probabilityDown >= probabilityUp && probabilityDown >= probabilityRight && probabilityDown >= probabilityLeft) {
            //Shoot down
            for (int row = lastRow+1; row < boardSize; row++) {
                if (board[row][lastCol] == WATER) {
                    lastRow = row;
                    shotHorizontal = false;
                    shotVertical = true;
                    Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                    return attemptShot;
                }
                else if (board[row][lastCol] == HIT) {
                    continue;
                }
                else {
                    break;
                }
            }
        }
        //Right is best.
        if (probabilityRight >= probabilityDown && probabilityRight >= probabilityUp && probabilityRight >= probabilityLeft) {
            //Shoot right
            for (int col = lastCol+1; col < boardSize; col++) {
                if (board[lastRow][col] == WATER) {
                    lastCol = col;
                    shotVertical = false;
                    shotHorizontal = true;
                    Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                    return attemptShot;
                }
                else if (board[lastRow][col] == HIT) {
                    continue;
                }
                else {
                    break;
                }
            }
        }
        //Left is best.
        if (probabilityLeft >= probabilityDown && probabilityLeft >= probabilityRight && probabilityLeft >= probabilityUp) {
            //Shoot left
            for (int col = lastCol-1; col >= 0; col--) {
                if (board[lastRow][col] == WATER) {
                    lastCol = col;
                    shotVertical = false;
                    shotHorizontal = true;
                    Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
                    return attemptShot;
                }
                else if (board[lastRow][col] == HIT) {
                    continue;
                }
                else {
                    break;
                }
            }
        }
    }
    */



    //Shoot up
    for (int row = lastRow-1; row >= 0; row--) {
        //If no chance.
        if (probabilityBoard[row][lastCol] == 0) {
            break;
        }
        else if (board[row][lastCol] == WATER) {
            lastRow = row;
            shotHorizontal = false;
            shotVertical = true;
            Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
            return attemptShot;
        }
        else if (board[row][lastCol] == HIT) {
            continue;
        }
        else {
            break;
        }
    }

    //Shoot right
    for (int col = lastCol+1; col < boardSize; col++) {
        //If no chance.
        if (probabilityBoard[lastRow][col] == 0) {
            break;
        }
        else if (board[lastRow][col] == WATER) {
            lastCol = col;
            shotVertical = false;
            shotHorizontal = true;
            Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
            return attemptShot;
        }
        else if (board[lastRow][col] == HIT) {
            continue;
        }
        else {
            break;
        }
    }

    //Shoot down
    for (int row = lastRow+1; row < boardSize; row++) {
        //If no chance.
        if (probabilityBoard[row][lastCol] == 0) {
            break;
        }
        else if (board[row][lastCol] == WATER) {
            lastRow = row;
            shotHorizontal = false;
            shotVertical = true;
            Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
            return attemptShot;
        }
        else if (board[row][lastCol] == HIT) {
            continue;
        }
        else {
            break;
        }
    }

    //Shoot left
    for (int col = lastCol-1; col >= 0; col--) {
        //If no chance.
        if (probabilityBoard[lastRow][col] == 0) {
            break;
        }
        if (board[lastRow][col] == WATER) {
            lastCol = col;
            shotVertical = false;
            shotHorizontal = true;
            Message attemptShot( SHOT, lastRow, lastCol, "Bang", None, 1);
            return attemptShot;
        }
        else if (board[lastRow][col] == HIT) {
            continue;
        }
        else {
            break;
        }
    }

    
    //Otherwise, if all else fails...
    return RegularScanMove();
}


void USSWhite::resetShotBools() {
    shotVertical = false;
    shotHorizontal = false;
    enemyShipIsVertical = false;
    enemyShipIsHorizontal = false;
}


bool USSWhite::unpursuedHit() {
    if (huntingAnEnemyShip) {  //If currently hunting, don't try yet
        return false;
    }
    for (int row=0; row < boardSize; row++) {
        for (int col=0; col < boardSize; col++) {
            if (board[row][col] == HIT) {
                return true;
            }
        }
    }
    return false;
}


bool USSWhite::isValidLocation(int Row, int Col, int shipLength, int direction) {
    //Check if legal (horizontal)
    if (direction == 0) {
        if ((Col+shipLength-1) < boardSize) {
            for (int col=Col; col<(Col+shipLength); col++) {
                if (myShipBoard[Row][col] == SHIP) {
                    return true;
                }
            }
        }
        else {
            return true;
        }
    }

    //Check if legal (vertical)
    else {
        if ((Row+shipLength-1) < boardSize) {
            for (int row=Row; row<(Row+shipLength); row++) {
                if (myShipBoard[row][Col] == SHIP) {
                    return true;
                }
            }
        }
        else {
            return true;
        }
    }

    return false;
}


Message USSWhite::placeShipsRandomly(int Row, int Col, Direction direction, int directionNum, int shipLength, char shipName[]) {
    bool invalid = true;  //get the loop started
    while (invalid) {
        invalid = false;
        directionNum = rand() % 2;
        if (directionNum == 0) {
            direction = Horizontal;
            Row = rand() % boardSize;
            Col = rand() % (boardSize - shipLength + 1);
        }
        else {
            direction = Vertical;
            Row = rand() % (boardSize - shipLength + 1);
            Col = rand() % boardSize;
        }
        invalid = isValidLocation(Row, Col, shipLength, directionNum);
    }

    // parameters = mesg type (PLACE_SHIP), row, col, a string, direction (Horizontal/Vertical)
    Message response( PLACE_SHIP, Row, Col, shipName, direction, shipLength );
    numShipsPlaced++;

    //Update myShipBoard to keep track
    if (directionNum == 0) {
        for (int col=Col; col<(Col+shipLength); col++) {
            myShipBoard[Row][col] = SHIP;
        }
    }
    else {
        for (int row=Row; row<(Row+shipLength); row++) {
            myShipBoard[row][Col] = SHIP;
        }
    }

    return response;
}



Message USSWhite::placeShipsLow(int Row, int Col, Direction direction, int directionNum, int shipLength, char shipName[]) {
    bool invalid = true;  //get the loop started
    int counter = 0;
    while (invalid) {
        invalid = false;

        if (counter >= 50) {
            Row = rand() % boardSize;
            Col = rand() % boardSize;
            directionNum = rand() % 2;
        }

        else if (numShipsPlaced == 0) {  //Have at least one ship way at the bottom
            Row = boardSize-1;
            Col = rand() % boardSize;
            directionNum = 0;
        }

        else if (numShipsPlaced < 3) {  //Prefer lower spots for first 3 ships
            Row = int(boardSize/2) + rand() % int(boardSize/2);
            Col = rand() % boardSize;
            directionNum = rand() % 2;
            if (directionNum == 0) {
                direction = Horizontal;
                Row = int(boardSize/2) + rand() % int(boardSize/2);
                Col = rand() % (boardSize - shipLength + 1);
            }
            else {
                direction = Vertical;
                Row = int(boardSize/2) + rand() % int(boardSize/2);
                Col = rand() % boardSize;
            }
        }

        else {  //Otherwise just random
            directionNum = rand() % 2;
            if (directionNum == 0) {
                direction = Horizontal;
                Row = rand() % boardSize;
                Col = rand() % (boardSize - shipLength + 1);
            }
            else {
                direction = Vertical;
                Row = rand() % (boardSize - shipLength + 1);
                Col = rand() % boardSize;
            }
        }

        invalid = isValidLocation(Row, Col, shipLength, directionNum);
        counter++;
    }


    if (directionNum == 0) {
        direction = Horizontal;
    }
    else {
        direction = Vertical;
    }

    // parameters = mesg type (PLACE_SHIP), row, col, a string, direction (Horizontal/Vertical)
    Message response( PLACE_SHIP, Row, Col, shipName, direction, shipLength );
    numShipsPlaced++;

    //Update myShipBoard to keep track
    if (directionNum == 0) {
        for (int col=Col; col<(Col+shipLength); col++) {
            myShipBoard[Row][col] = SHIP;
        }
    }
    else {
        for (int row=Row; row<(Row+shipLength); row++) {
            myShipBoard[row][Col] = SHIP;
        }
    }

    return response;
}


void USSWhite::adjustShotToBeNew() {
    lastCol++;
    if (lastCol >= boardSize) {
        lastCol = 0;
        lastRow++;
    }
    if (lastRow >= boardSize) {
        lastRow = 0;
    }
    return;
}



//Probability stuff
void USSWhite::initializeProbabilities(int Board[][MAX_BOARD_SIZE], char shotsBoard[][MAX_BOARD_SIZE]) {
    resetBoard(probabilityBoard);
    calculateHorizontal(Board,shotsBoard);
    calculateVertical(Board,shotsBoard);
}


void USSWhite::calculateHorizontal(int Board[][MAX_BOARD_SIZE], char shotsBoard[][MAX_BOARD_SIZE]) {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize-MIN_SHIP_SIZE+1; c++) {
            if ((shotsBoard[r][c] == WATER || shotsBoard[r][c] == HIT) && (shotsBoard[r][c+1] == WATER || shotsBoard[r][c+1] == HIT) && (shotsBoard[r][c+2] == WATER || shotsBoard[r][c+2] == HIT)) {
                Board[r][c] += 1;
                Board[r][c+1] += 1;
                Board[r][c+2] += 1;
            }
        }
    }
}



void USSWhite::calculateVertical(int Board[][MAX_BOARD_SIZE], char shotsBoard[][MAX_BOARD_SIZE]) {
    for (int c=0; c<boardSize; c++) {
        for (int r=0; r<boardSize-MIN_SHIP_SIZE+1; r++) {
            if ((shotsBoard[r][c] == WATER || shotsBoard[r][c] == HIT) && (shotsBoard[r+1][c] == WATER || shotsBoard[r+1][c] == HIT) && (shotsBoard[r+2][c] == WATER || shotsBoard[r+2][c] == HIT)) {
                Board[r][c] += 1;
                Board[r+1][c] += 1;
                Board[r+2][c] += 1;
            }
        }
    }
}


Message USSWhite::fireBestShot(int probabilityBoard[][MAX_BOARD_SIZE]) {
    int highestProbability = findHighestProbability(probabilityBoard);
    std::vector<int> placesToTry_Rows;
    std::vector<int> placesToTry_Cols;
    int numPlacesToTry = 0;
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            if (probabilityBoard[r][c] == highestProbability) {
                numPlacesToTry += 1;
                placesToTry_Rows.push_back(r);
                placesToTry_Cols.push_back(c);
            }
        }
    }
    int index = rand() % numPlacesToTry;
    int row = placesToTry_Rows[index];
    int col = placesToTry_Cols[index];
    Message result( SHOT, row, col, "Bang", None, 1 );
    return result;
}


int USSWhite::findHighestProbability(int probabilityBoard[][MAX_BOARD_SIZE]) {
    int highestProbability = 0;
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            if (probabilityBoard[r][c] > highestProbability && myShotsBoard[r][c] == WATER) {
                highestProbability = probabilityBoard[r][c];
            }
        }
    }
    return highestProbability;
}


Message USSWhite::getProbabilityScanMove() {
    dealWithLearningShotPlacement();
    probabilityScanShotCount++;
    return fireBestShot(probabilityBoard);
}


void USSWhite::resetBoards() {
    resetBoard(myShipBoard);
    resetBoard(myShotsBoard);
    resetBoard(enemyShotsBoard);
    resetBoard(probabilityBoard);
}


Message USSWhite::placeShipsInUnlikelySpots(int Row, int Col, Direction direction, int directionNum, int shipLength, char shipName[]) {
    bool invalid = true;  
    while (invalid) {
        invalid = false;

        if (numShipsPlaced == 0) {
            direction = Horizontal;
            directionNum = 0;
            Row = boardSize-1;
            Col = boardSize-shipLength;
        }
        
        else if (numShipsPlaced == 1) {
            direction = Horizontal;
            directionNum = 0;
            Row = 0;
            Col = 0;
        }

        else if (numShipsPlaced == 2) {
            direction = Vertical;
            directionNum = 1;
            Row = 0;
            Col = boardSize-1;
        }

        else if (numShipsPlaced == 3) {
            direction = Horizontal;
            directionNum = 0;
            Row = boardSize-1;
            Col = 0;
        }

        else {
            //Random placement
            directionNum = rand() % 2;
            if (directionNum == 0) {
                direction = Horizontal;
                Row = rand() % boardSize;
                Col = rand() % (boardSize - shipLength + 1);
            }
            else {
                direction = Vertical;
                Row = rand() % (boardSize - shipLength + 1);
                Col = rand() % boardSize;
            }
        }
        invalid = isValidLocation(Row, Col, shipLength, directionNum);
    }


    Message response( PLACE_SHIP, Row, Col, shipName, direction, shipLength );
    numShipsPlaced++;

    //Update myShipBoard to keep track
    if (directionNum == 0) {
        for (int col=Col; col<(Col+shipLength); col++) {
            myShipBoard[Row][col] = SHIP;
        }
    }
    else {
        for (int row=Row; row<(Row+shipLength); row++) {
            myShipBoard[row][Col] = SHIP;
        }
    }

    return response;
}


void USSWhite::copyBoard(int destinationBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE],int sourceBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE]) {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            destinationBoard[r][c] = sourceBoard[r][c];
        }
    }
}


void USSWhite::updateEnemyShotsIncrementBoard() {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            if (enemyShotsBoard[r][c] != WATER) {
                //enemyShotsIncrementBoard[r][c] += shotWeightMarker;
                enemyShotsIncrementBoard[r][c]++;
            }
        }
    }
}


Message USSWhite::placeShipsByLearning(int Row, int Col, Direction direction, int directionNum, int shipLength, char shipName[]) {

    setDirectionBasedOnProbability(direction, directionNum, shipLength);

    //Updates row and col
    findBestPlaceForShip(Row, Col, shipLength, directionNum, direction);

    Message response(PLACE_SHIP, Row, Col, shipName, direction, shipLength);
    numShipsPlaced++;

    //Update myShipBoard to keep track
    if (directionNum == 0) {
        for (int col=Col; col<(Col+shipLength); col++) {
            myShipBoard[Row][col] = SHIP;
        }
    }
    else {
        for (int row=Row; row<(Row+shipLength); row++) {
            myShipBoard[row][Col] = SHIP;
        }
    }

    return response;
}


void USSWhite::findBestPlaceForShip(int& bestRow, int& bestCol, int shipLength, int directionNum, Direction direction) {
    int counter = 0;
    int lowestCount = 9999999;

    //Horizontal
    if (directionNum == 0) {

        //Find lowest count.
        for (int r=0; r<boardSize; r++) {
            for (int c=0; c<boardSize-shipLength+1; c++) {
                for (int i=0; i<shipLength; i++) {
                    counter+=enemyShotsIncrementBoardCopy[r][c+i];
                }
                if (counter < lowestCount) {
                    lowestCount = counter;
                }
                counter = 0;
            }
        }

        //Now find lowest count, use that row and col.
        for (int r=0; r<boardSize; r++) {
            for (int c=0; c<boardSize-shipLength+1; c++) {
                for (int i=0; i<shipLength; i++) {
                    counter+=enemyShotsIncrementBoardCopy[r][c+i];
                }
                if (counter == lowestCount) {
                    bestRow = r;
                    bestCol = c;
                    //Update that spot so another ship won't be placed there.
                    for (int i=0; i<shipLength; i++) {
                        enemyShotsIncrementBoardCopy[bestRow][bestCol+i]+=9999999;
                    }
                    return;
                }
                counter = 0;
            }
        }
    }

    //Vertical
    else {
        //Find lowest count.
        for (int r=0; r<boardSize-shipLength+1; r++) {
            for (int c=0; c<boardSize; c++) {
                for (int i=0; i<shipLength; i++) {
                    counter+=enemyShotsIncrementBoardCopy[r+i][c];
                }
                if (counter < lowestCount) {
                    lowestCount = counter;
                }
                counter = 0;
            }
        }

        //Now find lowest count, use that row and col.
        for (int r=0; r<boardSize-shipLength+1; r++) {
            for (int c=0; c<boardSize; c++) {
                for (int i=0; i<shipLength; i++) {
                    counter+=enemyShotsIncrementBoardCopy[r+i][c];
                }
                if (counter == lowestCount) {
                    bestRow = r;
                    bestCol = c;
                    //Update that spot so another ship won't be placed there.
                    for (int i=0; i<shipLength; i++) {
                        enemyShotsIncrementBoardCopy[bestRow+i][bestCol]+=9999999;
                    }
                    return;
                }
                counter = 0;
            }
        }
    }
       
}


void USSWhite::setDirectionBasedOnProbability(Direction& direction,int& directionNum, int shipLength) {
    int counter = 0;
    int lowestCountHorizontal = 9999999;
    int lowestCountVertical = 9999999;

    //Horizontal.
    //Find lowest count.
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize-shipLength+1; c++) {
            for (int i=0; i<shipLength; i++) {
                counter+=enemyShotsIncrementBoard[r][c+i];
            }
            if (counter < lowestCountHorizontal) {
                lowestCountHorizontal = counter;
            }
            counter = 0;
        }
    }

    //To be safe.
    counter = 0;

    //Vertical.
    //Find lowest count.
    for (int r=0; r<boardSize-shipLength+1; r++) {
        for (int c=0; c<boardSize; c++) {
            for (int i=0; i<shipLength; i++) {
                counter+=enemyShotsIncrementBoard[r+i][c];
            }
            if (counter < lowestCountVertical) {
                lowestCountVertical = counter;
            }
            counter = 0;
        }
    }

    //Horizontal better.
    if (lowestCountHorizontal < lowestCountVertical) {
        direction = Horizontal;
        directionNum = 0;
    }
    //Vertical better.
    else if (lowestCountVertical < lowestCountHorizontal) {
        direction = Vertical;
        directionNum = 1;
    }
    //Both the same odds.
    else {
        directionNum = rand() % 2;
        if (directionNum == 0) {
            direction = Horizontal;
        }
        else {
            direction = Vertical;
        }
    }
}


void USSWhite::updateMyShotsIncrementBoard() {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            //If this seems to be a sweet-spot.
            if (myShotsBoard[r][c] == HIT || myShotsBoard[r][c] == KILL) {
                //Add to the increment board.
                myShotsIncrementBoard[r][c]++;
            }
        }
    }   
}


void USSWhite::addToProbabilitiesBoard() {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {   
            if (probabilityBoard[r][c] != 0) {
                probabilityBoard[r][c] += myShotsIncrementBoard[r][c];
            }
        }
    }
}


void USSWhite::updateProbabilities(int lastRow, int lastCol) {
    //Zero-out the place of shot
    bool secondSquareUp = false;
    bool secondSquareDown = false;
    bool secondSquareRight = false;
    bool secondSquareLeft = false;

    if (lastRow-1 >= 0 && probabilityBoard[lastRow-1][lastCol] != 0) {
        secondSquareUp = true;
    }
    if (lastRow+1 < boardSize && probabilityBoard[lastRow+1][lastCol] != 0) {
        secondSquareDown = true;
    }
    if (lastCol+1 < boardSize && probabilityBoard[lastRow][lastCol+1] != 0) {
        secondSquareRight = true;
    }
    if (lastCol-1 >= 0 && probabilityBoard[lastRow][lastCol-1] != 0) {
        secondSquareLeft = true;
    }

    //Up
    if (lastRow-1 >= 0) {
        probabilityBoard[lastRow-1][lastCol] -= 2;
        if (probabilityBoard[lastRow-1][lastCol] < 0) {
            probabilityBoard[lastRow-1][lastCol] = 0;
        }
    }
    if (secondSquareUp) {
        if (lastRow-2 >= 0) {
            probabilityBoard[lastRow-2][lastCol] -= 1;
            if (probabilityBoard[lastRow-2][lastCol] < 0) {
                probabilityBoard[lastRow-2][lastCol] = 0;
            }
        }
    }

    //Down
    if (lastRow+1 < boardSize) {
        probabilityBoard[lastRow+1][lastCol] -= 2;
        if (probabilityBoard[lastRow+1][lastCol] < 0) {
            probabilityBoard[lastRow+1][lastCol] = 0;
        }
    }
    if (secondSquareDown) {
        if (lastRow+2 < boardSize) {
            probabilityBoard[lastRow+2][lastCol] -= 1;
            if (probabilityBoard[lastRow+2][lastCol] < 0) {
                probabilityBoard[lastRow+2][lastCol] = 0;
            }
        }
    }

    //Right
    if (lastCol+1 < boardSize) {
        probabilityBoard[lastRow][lastCol+1] -= 2;
        if (probabilityBoard[lastRow][lastCol+1] < 0) {
            probabilityBoard[lastRow][lastCol+1] = 0;
        }
    }
    if (secondSquareRight) {
        if (lastCol+2 < boardSize) {
            probabilityBoard[lastRow][lastCol+2] -= 1;
            if (probabilityBoard[lastRow][lastCol+2] < 0) {
                probabilityBoard[lastRow][lastCol+2] = 0;
            }
        }
    }

    //Left
    if (lastCol-1 >= 0) {
        probabilityBoard[lastRow][lastCol-1] -= 2;
        if (probabilityBoard[lastRow][lastCol-1] < 0) {
            probabilityBoard[lastRow][lastCol-1] = 0;
        }
    }
    if (secondSquareLeft) {
        if (lastCol-2 >= 0) {
            probabilityBoard[lastRow][lastCol-2] -= 1;
            if (probabilityBoard[lastRow][lastCol-2] < 0) {
                probabilityBoard[lastRow][lastCol-2] = 0;
            }
        }
    }
}


void USSWhite::dealWithLearningShotPlacement() {
    if (gamesPlayed == 0) {
        resetBoard(myShotsIncrementBoard);
    }
    if (gamesPlayed > 2) {
        int index;
        int row;
        int col;
        int counter = 0;
        int numPlacesToTry = 0;
        std::vector<int> rows;
        std::vector<int> cols;
        int mostProbablePlaceForAnEnemyShipBasedOnGameHistory = findHighestProbability(myShotsIncrementBoard);

        while (counter < 10) {
            //Have the first shot be where an enemy ship is most likely to be.
            //mostProbablePlaceForAnEnemyShipBasedOnGameHistory = findHighestProbability(myShotsIncrementBoardCopy);
            for (int r=0; r<boardSize; r++) {
                for (int c=0; c<boardSize; c++) {
                    if (myShotsIncrementBoard[r][c] == mostProbablePlaceForAnEnemyShipBasedOnGameHistory) {
                        numPlacesToTry += 1;
                        rows.push_back(r);
                        cols.push_back(c);
                    }
                }
            }
            
            index = rand() % numPlacesToTry;
            row = rows[index];
            col = cols[index];
            //Check if move would be inefficient.
            if ((row-1 >= 0 && myShotsBoard[row-1][col] != WATER) || (col+1 < boardSize && myShotsBoard[row][col+1] != WATER) || (row+1 < boardSize && myShotsBoard[row+1][col] != WATER) || (col-1 >= 0 && myShotsBoard[row][col-1] != WATER)) {
                //myShotsIncrementBoardCopy[row][col] = 0;
                counter++;
            }
            else {
                probabilityBoard[row][col] += 10;
                break;
            }
            numPlacesToTry = 0;
            rows.clear();
            cols.clear();
        }
    }
}


int USSWhite::findNumSpacesVertical(int Row, int Col) {
    int numSpacesUp = 0;
    for (int r = Row-1; r >= 0; r--) {
        if (board[r][Col] == WATER) {
            numSpacesUp++;
        }
        else {
            break;
        }
    }

    int numSpacesDown = 0;
    for (int r = Row+1; r < boardSize; r++) {
        if (board[r][Col] == WATER) {
            numSpacesDown++;
        }
        else {
            break;
        }
    }

    return numSpacesUp + numSpacesDown;
}


int USSWhite::findNumSpacesHorizontal(int Row, int Col) {
    int numSpacesRight = 0;
    for (int c = Col+1; c < boardSize; c++) {
        if (board[Row][c] == WATER) {
            numSpacesRight++;
        }
        else {
            break;
        }
    }

    int numSpacesLeft = 0;
    for (int c = Col-1; c >= 0; c--) {
        if (board[Row][c] == WATER) {
            numSpacesLeft++;
        }
        else {
            break;
        }
    }

    return numSpacesRight + numSpacesLeft;
}
