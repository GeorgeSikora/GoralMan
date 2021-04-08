#include <conio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <cwchar>
#include <string>
#include <sstream>
#include <dos.h>
using namespace std;

/* velikost mapy */
#define ROWS 15 // øádky
#define COLS 45 // sloupce

/* velikost okna */
#define CONSOLE_WIDTH 400 // šíøka
#define CONSOLE_HEIGHT 300 // výška

/* dlaždice mapy */
#define ALL -1
#define AIR 0
#define BLOCK 1
#define PLAYER 2
#define ENEMY 3
#define FOOD 4

/* hra */
int frameCount = 0;
char screenColor = 'f';
int flashCount = 0;
int foodCount = 0;

/* hráè */
int playerScore = 0, playerLives = 10;
int playerX = COLS>>1, playerY = ROWS>>1;

/* mapa */
int map[ROWS][COLS];

/* nepøátelé */
typedef struct {
	bool alive = false;
	int x;
	int y;
} T_ENEMY;

T_ENEMY enemies[10];

/* funkce hry */
void enemyMove(int, int, int, int, int, int);

void placeBlockSymetric		(int, int, int);
void placeBlockSymetricX	(int, int, int);
void placeBlockSymetricY	(int, int, int);

int  getTile				(int, int);
int  getSolidTile			(int, int);
int  getBlock				(int, int);

void drawMap				();
char getTileTexture			(int, int);
bool neighborsEqual			(int[], int[]);
void setTextColor			(char c);

int main() {
	
	/* nastavení šíøky a výšky okna */
	HWND console = GetConsoleWindow();
	RECT ConsoleRect;
	GetWindowRect(console, &ConsoleRect);
	
	/* posunutí do støedu monitoru */
	int displayWidth  = GetSystemMetrics(SM_CXSCREEN);
	int displayHeight = GetSystemMetrics(SM_CYSCREEN);
	MoveWindow(console, (displayWidth>>1) - CONSOLE_WIDTH, (displayHeight>>1) - CONSOLE_HEIGHT, CONSOLE_WIDTH, CONSOLE_HEIGHT, TRUE);
	
	/* nastavení fontu a velikosti písma */
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = 16;
	cfi.dwFontSize.Y = 24;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	std::wcscpy(cfi.FaceName, L"Consolas");
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
	
	/* vypnout zobrazení kurzoru */
	HANDLE hOut;
	CONSOLE_CURSOR_INFO ConCurInf;
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	ConCurInf.dwSize = 10;
	ConCurInf.bVisible = FALSE;
	SetConsoleCursorInfo(hOut, &ConCurInf);
	
	/*** GENEROVÁNÍ MAPY ***/
	
	/* ohranièení mapy */
	for(int y = 0; y < ROWS; y++) placeBlockSymetricY(BLOCK, 0, y);
	for(int x = 1; x < COLS-1; x++) placeBlockSymetricX(BLOCK, x, 0);
	
	/* boèní otvory na pøechod */
	placeBlockSymetricY(AIR, 0, ROWS>>1);
	
	/* vygeneruje vnitøní grid */
	/*for(int y = 1; y < ROWS-1; y++) {
		for(int x = 1; x < COLS-1; x++) {
			int lt = map[y-1][x-1];
			int mt = map[y-1][x  ];
			int rt = map[y-1][x+1];
			
			int lm = map[y  ][x-1];
			int mm = map[y  ][x  ];
			int rm = map[y  ][x+1];
			
			int lb = map[y+1][x-1];
			int mb = map[y+1][x  ];
			int rb = map[y+1][x+1];
			
			if (lt == 0 && mt == 0 && rt == 0 && lm == 0 && mm == 0 && rm == 0 && lb == 0 && mb == 0 && rb == 0) {
				map[y][x] = 1;
			}
		}
	}*/
	
	int midY = ROWS >> 1;
	
	/* komín brány po bocích */
	placeBlockSymetric(BLOCK, 1, midY-1);
	placeBlockSymetric(BLOCK, 2, midY-1);
	
	/* boèní svislé èáry v okolí brány */
	placeBlockSymetric(BLOCK, 4, midY-3);
	placeBlockSymetric(BLOCK, 4, midY-2);
	placeBlockSymetric(BLOCK, 4, midY-1);
	
	/* vodorovné èáry pokraèující od brán */
	placeBlockSymetric(BLOCK, 5, midY-2);
	
	/* horní a dolní lajny */
	placeBlockSymetric(BLOCK, 2, midY-3);
	placeBlockSymetric(BLOCK, 2, midY-4);
	placeBlockSymetric(BLOCK, 2, midY-5);
	
	placeBlockSymetric(BLOCK, 3, midY-5);
	placeBlockSymetric(BLOCK, 4, midY-5);
	placeBlockSymetric(BLOCK, 5, midY-5);
	
	/* pøidání jídla */
	for(int y = 1; y < ROWS-1; y++) {
		for(int x = 1; x < COLS-1; x++) {
			if (map[y][x] != AIR) continue;
			
			int lt = map[y-1][x-1];
			int mt = map[y-1][x  ];
			int rt = map[y-1][x+1];
			
			int lm = map[y  ][x-1];
			int mm = map[y  ][x  ];
			int rm = map[y  ][x+1];
			
			int lb = map[y+1][x-1];
			int mb = map[y+1][x  ];
			int rb = map[y+1][x+1];
			
			if (lt != FOOD && mt != FOOD && rt != FOOD && lm != FOOD && mm != FOOD && rm != FOOD && lb != FOOD && mb != FOOD && rb != FOOD) {
				map[y][x] = FOOD;
				foodCount++;
			}
		}
	}
	
	/*** KONEC GENEROVÁNÍ MAPY ***/

	/* vytvoøení nepøátel */
	enemies[0].x = 5;
	enemies[0].y = 6;
	enemies[0].alive = 1;
	
	enemies[1].x = 10;
	enemies[1].y = 10;
	enemies[1].alive = 1;
	
	/* smyèka draw */
	while(1) {
		
		/* vykreslení mapy */
		system("cls");
		drawMap();
		
		/* blikání barev */
		if (flashCount > 0) {
			screenColor = screenColor == 'f'?'c':'f';
			setTextColor(screenColor);
			flashCount--;
		} else {
			setTextColor('f');
		}
	
		/* získání hráèova tahu */
		int key = 0;
		if (kbhit()) {
			key = getch();
        }
		
		/* pohyb hráèe */
		// pdx .. player delta x
		// pdy .. player delta y
		int pdx = 0, pdy = 0;
		switch (key) {
			case 'w': case 'W':
				pdy = -1;
				break;
			case 'a': case 'A':
				pdx = -1;
				break;
			case 's': case 'S':
				pdy = 1;
				break;
			case 'd': case 'D':
				pdx = 1;
				break;
		}
		// pct .. player collision tile
		int pct = getSolidTile(playerX +pdx, playerY +pdy);
		
		if (pct == AIR || pct == ENEMY) {
			playerX += pdx;
			playerY += pdy;
		}
		
		/* získání jídla */
		if (getTile(playerX, playerY) == FOOD) {
			playerScore++;
			map[playerY][playerX] = 0;
		}
		
		
		/* støet nepøátel s hráèem */
		/* for (int e = 0; e < 10; e++) {
			if (!enemies[e].alive) continue;
			
			if (playerX == enemies[e].x && playerY == enemies[e].y) {
				
				if (playerLives <= 0) {
				}
				
				flashCount = 3;
				
				playerLives--;
				break;
			}
		} */
		
		/* v pøípadì když je hráè mimo mapu, pøesune ho to na druhou stranu */
		if (playerX >= COLS) playerX = 0;
		if (playerX < 0) playerX = COLS-1;
		if (playerY >= ROWS) playerY = 0;
		if (playerY < 0) playerY = ROWS-1;
		
		/* nastaví informace titulku okna hry */
		wstringstream titleStream;
		titleStream << "Jiri Sikora - GoralMan POINTS: " << playerScore << "/" << foodCount << " LIVES: " << playerLives;
		SetConsoleTitleW(titleStream.str().c_str());
		
		/* pohyb nepøátel */
		if (frameCount%2 == 0) {
			for (int e = 0; e < 10; e++) {
				if (!enemies[e].alive) continue;
				
				T_ENEMY E = enemies[e];
			
				int pX = playerX;
				int pY = playerY;
				
				int eX = E.x;
				int eY = E.y;
	
				if (pX > eX) {
					enemyMove(e, 1, 0, 0, 0, 0);
					continue;
				}
				if (pX < eX) {
					enemyMove(e,-1, 0, 0, 0, 0);
					continue;
				}
				if (pY > eY) {
					enemyMove(e, 0, 1, 0, 0, 0);
					continue;
				}
				if (pY < eY) {
					enemyMove(e, 0,-1, 0, 0, 0);
					continue;
				}
			}
		}
		
		frameCount++;
		Sleep(100);
	}
}

// e   .. enemy
// dx  .. direction x
// dy  .. direction y
// rdx .. repeir direction x
// rdy .. repeir direction y
// dis .. distribution
void enemyMove(int e, int dx, int dy, int rdx, int rdy, int dis = 0) {
	
	dis++;
	if (dis > 2) return;
	
	T_ENEMY E = enemies[e];

	int eX = E.x;
	int eY = E.y;
	
	int t = getSolidTile(eX+dx, eY+dy);

	/* støet s hráèem */
	if (t == PLAYER) {
		if (playerLives <= 0) {
			/* END */
		}
		
		flashCount = 3;
		
		playerLives--;
		return;
	}

	if (t != AIR) {
		
		if (dy == 0) {
			if (rdx == 0) {
				enemyMove(e, 0, 1, 1, 0, dis);
			}
			if (rdx == 1) {
				enemyMove(e, 0, -1, -1, 0, dis);
			}
			if (rdx == -1) {
			}
			return;
		}
		if (dx == 0) {
			if (rdy == 0) {
				enemyMove(e, 1, 0, 0, 1, dis);
			}
			if (rdy == 1) {
				enemyMove(e, -1, 0, 0, -1, dis);
			}
			if (rdy == -1) {
			}
			return;
		}
	}
		
	enemies[e].x += dx;
	enemies[e].y += dy;
}

void placeBlockSymetric(int block, int x, int y) {
	placeBlockSymetricX(block, x, y);
	placeBlockSymetricY(block, x, y);
	placeBlockSymetricY(block, x, ROWS-1-y);
}
void placeBlockSymetricX(int block, int x, int y) {
	map[y][x] = block;
	map[ROWS-1-y][x] = block;
}
void placeBlockSymetricY(int block, int x, int y) {
	map[y][x] = block;
	map[y][COLS-1-x] = block;
}

int getTile(int x, int y) {
	if (x >= 0 && y >= 0 && x < COLS && y < ROWS)
		return map[y][x];
	return AIR;
}

int getSolidTile(int x, int y) {
	int tile = getTile(x, y);
	
	if (x == playerX && y == playerY) return PLAYER;
	
	for (int e = 0; e < 10; e++) {
		if (!enemies[e].alive) continue;
		if (enemies[e].x == x && enemies[e].y == y) return ENEMY;
	}
	
	if (tile == FOOD) return AIR;
	
	return tile;
}
int getBlock(int x, int y) {
	int tile = getTile(x, y);
	
	return getTile(x, y) == BLOCK ? BLOCK : AIR;
}

void drawMap() {
	for (int y = 0; y < ROWS; y++) {
		for (int x = 0; x < COLS; x++) {
			
			/* jestli je na pozici nepøátel, vykresli */
			bool founded = false;
			for (int e = 0; e < 10; e++) {
				if (!enemies[e].alive) continue;
				
				if (enemies[e].x == x && enemies[e].y == y) {
					cout << (char) 12;
					founded = true;
				}
			}
			if (founded) continue;
			
			/* jestli tam je hráè, vykresli */
			if (x == playerX && y == playerY) {
				cout << (char) 2;
				continue;
			}
			
			/* ostatní pasivní objekty na mapì */
			switch (map[y][x]) {
				case BLOCK:
					cout << getTileTexture(x, y);
					break;
				case FOOD:
					cout << (char) 250;
					break;
				default:
					cout << ' ';
					break;
			}
		}
		cout << '\n';
	}
}

char getTileTexture(int x, int y) {
	
	int neighbors[] = {
		getBlock(x-1, y-1), getBlock(x, y-1), getBlock(x+1, y-1), // B B B
		getBlock(x-1, y  ), AIR             , getBlock(x+1, y  ), // B 0 B
		getBlock(x-1, y+1), getBlock(x, y+1), getBlock(x+1, y+1), // B B B
	};
	
	int s[] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // single
	
	int tl[] = {0, 1, 0, 1, 0, 0, 0, 0, 0}; // top left
	int tr[] = {0, 1, 0, 0, 0, 1, 0, 0, 0}; // top right
	int bl[] = {0, 0, 0, 1, 0, 0, 0, 1, 0}; // bottom left
	int br[] = {0, 0, 0, 0, 0, 1, 0, 1, 0}; // bottom right
	
	int hline[] = {-1, 0,-1, -1, 0,-1, -1, 0,-1}; // horizontal line
	int vline[] = {-1,-1,-1,  0, 0, 0, -1,-1,-1}; // vertical line
	
	int ht[] = {-1, 1,-1, 1, 0, 1, -1, 0,-1}; // horizontal top
	int hb[] = {-1, 0,-1, 1, 0, 1, -1, 1,-1}; // horizontal bottom
	
	int vl[] = {-1, 1,-1, 1, 0, 0, -1, 1,-1}; // vertical left
	int vr[] = {-1,-1,-1, 0, 0, 1, -1, 1,-1}; // vertical right
	
	if (neighborsEqual(neighbors, tl	)) return 188;

	if (neighborsEqual(neighbors, tr	)) return 200;
		
	if (neighborsEqual(neighbors, bl	)) return 187;
		
	if (neighborsEqual(neighbors, br	)) return 201;
	
	if (neighborsEqual(neighbors, s 	)) return 207;
	
	if (neighborsEqual(neighbors, ht	)) return 202;
	
	if (neighborsEqual(neighbors, hb	)) return 203;
	
	if (neighborsEqual(neighbors, vl	)) return 185;
	
	if (neighborsEqual(neighbors, vr	)) return 204;
	
	if (neighborsEqual(neighbors, hline	)) return 205;
	
	if (neighborsEqual(neighbors, vline	)) return 186;
	
	return 207;
}

bool neighborsEqual(int n1[], int n2[]) {
	for (int i = 0; i < 9; i++) {
		
		int tile1 = n1[i];
		int tile2 = n2[i];
		
		if (tile1 != tile2 && tile2 != -1) return false;
	}
	return true;
}

void setTextColor(char c) {
	char changeColor[] = "color f";
	changeColor[6] = c;
	system(changeColor);
}
