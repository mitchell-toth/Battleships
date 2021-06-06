/**
 * @author Stefan Brandle, Jonathan Geisler
 * @date September, 2004
 *
 * Please type in your name[s] here: Mitchell Toth, Alec Houseman
 *
 */

#ifndef USSWhite_H		// Double inclusion protection
#define USSWhite_H

using namespace std;

#include "PlayerV2.h"
#include "Message.h"
#include "defines.h"

// USSWhite inherits from/extends PlayerV2

class USSWhite: public PlayerV2 {
    public:
	USSWhite( int boardSize );
	~USSWhite();
	void newRound();
	Message placeShip(int length);
	Message getMove();
	void update(Message msg);

    private:
    //Initial stuff
	void initializeBoard();
    int lastRow;
    int lastCol;
    int tempLastRow;
    int tempLastCol;
	int gamesPlayed;
    
    //Boards
    int numShipsPlaced;
    char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    char myShipBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    char myShotsBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE];

    //Cleanup stuff
    void resetBoard(char Board[][MAX_BOARD_SIZE]);
    void resetBoard(int Board[][MAX_BOARD_SIZE]);
    void resetBoards();
    
    //Scanning move stuff
    Message RegularScanMove();
    Message getCleanPlayerScanMove();
    bool doMiddleScan;  //start cleanPlayer scan from middle
    void adjustShotToBeNew();
    
    //Targeting a ship stuff
    bool huntingAnEnemyShip;
    bool shotVertical;
    bool shotHorizontal;
    bool enemyShipIsVertical;
    bool enemyShipIsHorizontal;
    Message branchOut();
    int findNumSpacesVertical(int Row, int Col);
    int findNumSpacesHorizontal(int Row, int Col);
    void resetShotBools();
    bool unpursuedHit();

    //Ship placement stuff
    bool isValidLocation(int Row, int Col, int shipLength, int direction);
    Message placeShipsRandomly(int Row, int Col, Direction direction, int directionNum, int shipLength, char shipName[]);
    Message placeShipsLow(int Row, int Col, Direction direction, int directionNum, int shipLength, char shipName[]);
    bool SHIP_PLACEMENT_low;  //true = place ships low
    bool SHIP_PLACEMENT_unlikely;  //true = place ships on edges
    Message placeShipsInUnlikelySpots(int Row, int Col, Direction direction, int directionNum, int shipLength, char shipName[]);
    
    //Probability stuff
    bool doProbabilityScan;  //true = do gambler-like scanning
    int probabilityBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    void initializeProbabilities(int Board[][MAX_BOARD_SIZE], char shotsBoard[][MAX_BOARD_SIZE]);
	void calculateHorizontal(int Board[][MAX_BOARD_SIZE],char shotsBoard[][MAX_BOARD_SIZE]);
	void calculateVertical(int Board[][MAX_BOARD_SIZE],char shotsBoard[][MAX_BOARD_SIZE]);
	int findHighestProbability(int probabilityBoard[][MAX_BOARD_SIZE]);
	Message fireBestShot(int probabilityBoard[][MAX_BOARD_SIZE]);
    Message getProbabilityScanMove();
    int randNum;

    //Learning-based ship placement stuff
    char enemyShotsBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    int enemyShotsIncrementBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    int enemyShotsIncrementBoardCopy[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    void copyBoard(int destinationBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE],int sourceBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE]);
    void updateEnemyShotsIncrementBoard();
    bool SHIP_PLACEMENT_learning;
    Message placeShipsByLearning(int Row, int Col, Direction direction, int directionNum, int shipLength, char shipName[]);
	void findBestPlaceForShip(int& bestRow, int& bestCol, int shipLength, int directionNum, Direction direction);
    void setDirectionBasedOnProbability(Direction& direction, int& directionNum, int shipLength);
    
    //Learning-based shot placement stuff
    int probabilityScanShotCount;
    int overallShotCount;
    void dealWithLearningShotPlacement();
    void updateProbabilities(int lastRow, int lastCol);
    int myShotsIncrementBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    int myShotsIncrementBoardCopy[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    void updateMyShotsIncrementBoard();
    void addToProbabilitiesBoard();
};

#endif
