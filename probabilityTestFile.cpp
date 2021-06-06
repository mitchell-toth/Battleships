#include <iostream>
#include <cstdlib>
#include <vector>
using namespace std;

#define boardSize 10
#define MAX_BOARD_SIZE 10
#define MIN_SHIP_SIZE 3
#define WATER 'W'
#define MISS 'M'
#define HIT 'H'
#define KILL 'K'

void resetBoard(int Board[][MAX_BOARD_SIZE]);
void resetBoard(char Board[][MAX_BOARD_SIZE]);
void printBoard(int Board[][MAX_BOARD_SIZE]);
void printBoard(char Board[][MAX_BOARD_SIZE]);
void updateProbabilities(int Board[][MAX_BOARD_SIZE], char shotsBoard[][MAX_BOARD_SIZE]);
void calculateHorizontal(int Board[][MAX_BOARD_SIZE],char shotsBoard[][MAX_BOARD_SIZE]);
void calculateVertical(int Board[][MAX_BOARD_SIZE],char shotsBoard[][MAX_BOARD_SIZE]);
void fireRandomShotsOnBoard(char shotsBoard[][MAX_BOARD_SIZE], int numShots);
int findHighestProbability(int probabilityBoard[][MAX_BOARD_SIZE]);
void fireBestShot(int probabilityBoard[][MAX_BOARD_SIZE]);

int main() {
    char myShotsBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    resetBoard(myShotsBoard);
    fireRandomShotsOnBoard(myShotsBoard,5);
    printBoard(myShotsBoard);

    int probabilityBoard[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    resetBoard(probabilityBoard);
    updateProbabilities(probabilityBoard,myShotsBoard);
    fireBestShot(probabilityBoard);
    printBoard(probabilityBoard);
    return 0;
}


void resetBoard(int Board[][MAX_BOARD_SIZE]) {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            Board[r][c] = 0;
        }
    }
}


void resetBoard(char Board[][MAX_BOARD_SIZE]) {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            Board[r][c] = WATER;
        }
    }
}


void printBoard(int Board[][MAX_BOARD_SIZE]) {
    cout << endl;
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            cout << Board[r][c] << " ";
        }
        cout << endl;
    }
    cout << endl;
}


void printBoard(char Board[][MAX_BOARD_SIZE]) {
    cout << endl;
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            cout << Board[r][c] << " ";
        }
        cout << endl;
    }
    cout << endl;
}


void fireRandomShotsOnBoard(char shotsBoard[][MAX_BOARD_SIZE], int numShots) {
    int row;
    int col;
    for (int i=0; i < numShots; i++) {
        row = rand() % 10;
        col = rand() % 10;
        shotsBoard[row][col] = MISS;
    }
}


void updateProbabilities(int Board[][MAX_BOARD_SIZE], char shotsBoard[][MAX_BOARD_SIZE]) {
    calculateHorizontal(Board,shotsBoard);
    calculateVertical(Board,shotsBoard);
}


void calculateHorizontal(int Board[][MAX_BOARD_SIZE], char shotsBoard[][MAX_BOARD_SIZE]) {
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize-MIN_SHIP_SIZE+1; c++) {
            if (shotsBoard[r][c] == WATER && shotsBoard[r][c+1] == WATER && shotsBoard[r][c+2] == WATER) {
                Board[r][c] += 1;
                Board[r][c+1] += 1;
                Board[r][c+2] += 1;
            }
        }
    }
}



void calculateVertical(int Board[][MAX_BOARD_SIZE], char shotsBoard[][MAX_BOARD_SIZE]) {
    for (int c=0; c<boardSize; c++) {
        for (int r=0; r<boardSize-MIN_SHIP_SIZE+1; r++) {
            if (shotsBoard[r][c] == WATER && shotsBoard[r+1][c] == WATER && shotsBoard[r+2][c] == WATER) {
                Board[r][c] += 1;
                Board[r+1][c] += 1;
                Board[r+2][c] += 1;
            }
        }
    }
}


void fireBestShot(int probabilityBoard[][MAX_BOARD_SIZE]) {
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
    cout << highestProbability << endl;
    cout << numPlacesToTry << endl;
    int index = rand() % numPlacesToTry;
    int row = placesToTry_Rows[index];
    int col = placesToTry_Cols[index];
    cout << row << ", " << col << endl;
}


int findHighestProbability(int probabilityBoard[][MAX_BOARD_SIZE]) {
    int highestProbability = 0;
    for (int r=0; r<boardSize; r++) {
        for (int c=0; c<boardSize; c++) {
            if (probabilityBoard[r][c] > highestProbability) {
                highestProbability = probabilityBoard[r][c];
            }
        }
    }
    return highestProbability;
}
