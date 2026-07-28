///
/// DECOMMENTA QUESTA RIGA SE USI UN SINGOLO FILE v v v v v
/// #define ONE_FILE  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// E' Richiesto lo Standard C++20

#define ret return
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define integer(x) (::std::fabs(x - ::std::round(x)) < 1e-9)

// inclusioni
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <Windows.h>
#include <Windowsx.h>

// altre inclusioni
#include <commctrl.h> // inclusione dopo Windows.h
#pragma comment(lib, "Comctl32.lib")
#ifndef ONE_FILE
#include "resource.h"
#endif

#pragma region GLOBALS

// random e tempo
using namespace std;
random_device rng;
mt19937 gen(rng());
uniform_int_distribution<size_t> density_roll(1, 100);
chrono::steady_clock::time_point TimeBegin, TimeEnd;

// robe per la finestra
HINSTANCE MainHinstance;
HWND MainHwnd, TooltipHwnd;
RECT WindowRect;
DWORD WindowStyle;
HMENU hMenu;
HICON hIconBrushOff, hIconRubberOff, hIconBrushOn, hIconRubberOn;
HICON hIconSelectOff, hIconSelectOn;
struct LINE_RECT {
	int hi;
	int hf;
	int vi;
	int vf;
};

// handle ai controlli
#pragma region handles
HWND handles[24]{};
#define hBirth         handles[0]
#define hSurvive       handles[1]
#define hApply         handles[2]
#define hRuleLabel     handles[3]
#define hDensity       handles[4]
#define hSpeed         handles[5]
#define hGrid          handles[6]
#define hBorders       handles[7]
#define hLabelSurvive  handles[8]
#define hLabelBirth    handles[9]
#define hLabelSpeed    handles[10]
#define hLabelDensity  handles[11]
#define hLabelSpeed2   handles[12]
#define hLabelDensity2 handles[13]
#define hFps           handles[14]
#define hGen           handles[15]
#define hLivec         handles[16]
#define hBrush         handles[17]
#define hRubber        handles[18]
#define hBornc         handles[19]
#define hMedc          handles[20]
#define hOldc          handles[21]
#define hSelect        handles[22]
#define hColor         handles[23]
#pragma endregion

// variabili globali
const int vs(10), hs(20), SL(50), L(70), ML(165), LL(300), H(18);
const int GridSize(512), TrackSize(30);
int NoDrawSpace(5 * vs + 4 * H);
int Pix(20), Density(30), CurrentId;
int SpeedMinimum(10), Speed(200), SpeedLimit(600);
struct coord {
	int X = 0;
	int Y = 0;
	inline bool operator==(const coord& other) const
	{
		ret X == other.X and Y == other.Y;
	}
};
coord CurrentSelected{ GridSize / 2, GridSize / 2 }, SelectionEnd;
coord Position{ GridSize * Pix / 2, GridSize * Pix / 2 };
const int PixMin(5), PixMax(67);

// altre variabili globali
double Fps;
int ptr, ptr2, offset, selecting_phase;
int Gen, pGen, AliveCells, BornCells, MediumCells, OldCells;
bool ShowGrid{ true }, ToroidalBorders{ true }, Color{ true }, Fullscreen{ false };
int** NewGrid = new int* [GridSize];
vector<int**> Backtrack(TrackSize);
bool Painting{ true }, enable{ false }, cursormoved{ false };
bool Pasting{ false }, Writing{ false }, Erasing{ false }, Selecting{ false };
bool vkshift{ false }, vkalt{ false }, vkcontrol{ false };

// identificatori dei vari elementi
#define Grid Backtrack[(offset + ptr2) % TrackSize]
#define ID_APPLY_COMMAND 100
#define ID_EDIT_BIRTH    101
#define ID_EDIT_SURVIVE  102
#define ID_DENSITY       103
#define ID_SPEED         104
#define ID_SHOW_GRID     105
#define ID_BORDERS       106
#define ID_BRUSH         107
#define ID_RUBBER        108
#define ID_SELECT        109
#define ID_COLOR         110
#define NORMAL    0
#define SELECTING 1
#define SELECTED  2
#pragma endregion

// stringhe piu' comuni
struct Rule {
	wstring params;
	wstring name;
};
struct GameMode {
	int ID;
	wstring params;
	wstring name;
	vector<int> B;
	vector<int> S;
};
vector<wstring> Categories
{
	L"Life Classici",
	L"Crescita e Colonie",
	L"Citta' e Strutture",
	L"Biologia e Organismi",
	L"Fuoco e Distruzione",
	L"Labirinti e Texture",
	L"Oscillatori e Pattern",
	L"Sperimentali",
};
vector<vector<Rule>> RuleLibrary
{
	vector<Rule>{
		{ L"B3/S2",         L"LifeLite"                 },
		{ L"B3/S23",        L"Conway's Game of Life"    },
		{ L"B36/S23",       L"HighLife"                 },
		{ L"B3678/S34678",  L"Day and Night"            },
		{ L"B3/S12345678",  L"Life Without Death 1"     },
		{ L"B3/S2345678",   L"Life Without Death 2"     },
		{ L"B3/S345678",    L"Life Without Death Gamma" },
		{ L"B3/S012345678", L"Life Without Death"       },
		{ L"B34/S34",       L"34 Life"                  },
		{ L"B1357/S1357",   L"Replicator"               },
	},
	vector<Rule>{
		{ L"B3/S234",          L"Ant Colony"                },
		{ L"B34/S234",         L"Expanding Phalanx"         },
		{ L"B345/S456",        L"Slow Assimilation"         },
		{ L"B345/S4567",       L"Assimilation"              },
		{ L"B345/S45",         L"Corrupted Assimilation"    },
		{ L"B3456/S4567",      L"Assimilation Wannabe"      },
		{ L"B3456/S45678",     L"Diamond with Medium Flaws" },
		{ L"B345678/S2345678", L"Jack Black\'s Octagon II"  },
		{ L"B34567/S45678",    L"Diamond with Small Flaws"  },
		{ L"B345/S45678",      L"Diamond with Big Flaws"    },
	},
	vector<Rule>{
		{ L"B34/S2345",        L"Building A City"       },
		{ L"B34/S23456",       L"Dormant City"          },
		{ L"B2/S234",          L"Standard Wire Casing"  },
		{ L"B2/S2345",         L"Dormant Wire Casing"   },
		{ L"B23/S234",         L"Flammable Wire Casing" },
		{ L"B23/S2345",        L"Buzzing Wire Casing"   },
		{ L"B23456/S01234567", L"Safety Pod Grid"       },
		{ L"B2/S0123",         L"Dodgeball Texture"     },
	},
	vector<Rule>{
		{ L"B34/S2345",   L"Brain Surface"        },
		{ L"B34/S4",      L"Pathetic Bacteria"    },
		{ L"B34/S45",     L"Pathetic Bacteria II" },
		{ L"B34/S456",    L"Bacteria"             },
		{ L"B34/S4567",   L"Rotten Egg"           },
		{ L"B34/S45678",  L"Healthy Egg"          },
		{ L"B3/S45678",   L"Coral"                },
		{ L"B345/S5",     L"Long Life"            },
		{ L"B345/S567",   L"Fuzzy on the Edges 1" },
		{ L"B3456/S567",  L"Fuzzy on the Edges 2" },
		{ L"B3456/S5678", L"Fuzzy on the Edges 3" },
	},
	vector<Rule>{
		{ L"B34/S23",        L"A World On Fire"      },
		{ L"B2345678/S567",  L"Wildfire from Within" },
		{ L"B2345678/S5678", L"Stripes then Fill"    },
		{ L"B234567/S5678",  L"Stripes vs Fill"      },
		{ L"B3/S4",          L"Fast Splash"          },
		{ L"B3/S45",         L"Medium Splash"        },
		{ L"B3/S456",        L"Slow Splash"          },
		{ L"B3/S4567",       L"Lifeguard 2"          },
		{ L"B3/S0",          L"Fuzzy Memory"         },
		{ L"B2/S0",          L"Live Free or Die"     },
	},
	vector<Rule>{
		{ L"B3/S1234",   L"Mazectric"           },
		{ L"B3/S12345",  L"Maze"                },
		{ L"B37/S12345", L"Mazectric with Mice" },
		{ L"B2/S6",      L"High Seeds"          },
		{ L"B2/S",       L"Seeds"               },
		{ L"B3/S0123",   L"Order vs Chaos"      },
		{ L"B234/S5678", L"Sane vs Insane"      },
	},
	vector<Rule>{
		{ L"B34/S2",        L"Slow Fizzle Out"         },
		{ L"B345/S2",       L"Blinkers"                },
		{ L"B345/S23",      L"Flickering Insides"      },
		{ L"B345/S234",     L"Shimmering Octagon"      },
		{ L"B3456/S2",      L"Tenacious Blinkers"      },
		{ L"B3456/S23",     L"More Flickering Insides" },
		{ L"B34/S12345678", L"Jack Black's Octagon"    },
		{ L"B3/S1",         L"Couple Counseling"       },
		{ L"B3/S12",        L"Flock"                   },
	},
	vector<Rule>{
		{ L"B36/S125",     L"2x2"                },
		{ L"B368/S245",    L"Morley"             },
		{ L"B4678/S35678", L"Anneal"             },
		{ L"B35678/S5678", L"Diamoeba"           },
		{ L"B345/S4",      L"Stripes Gone Rogue" },
	},
};
vector<GameMode> Rules;
COLORREF AgeColor[] // colori in formato B-G-R
{
	RGB(0  , 0  , 0  ), //
	RGB(20 , 20 , 20 ), //
	RGB(40 , 40 , 40 ), // cella morta
	RGB(60 , 60 , 60 ), //
	RGB(85 , 85 , 85 ), //
	RGB(115, 115, 115), //

	RGB(255, 255, 255), //
	RGB(40 , 230, 255), //
	RGB(30 , 150, 255), //
	RGB(60 , 60 , 255), // cella viva
	RGB(220, 40 , 220), //
	RGB(255, 70 , 120), //
	RGB(255, 180, 40 ), //

	RGB(0  , 255, 0  ), // cella selezionata
};

// lista dei pattern principali
vector<wstring> PatternTypes
{
	L"Still Lives",
	L"Oscillators",
	L"Spaceships",
	L"Methuselahs",
};
vector<vector<wstring>> Patterns
{
	vector<wstring>{
		L"Block (B)",
		L"Loaf (D)",
		L"Double Loaf (E)",
		L"Beehive (H)",
		L"Quadruple Beehive (I)",
		L"Eye (J)",
		L"Octagon (O)",
		L"Double L-Trimino (S)",
		L"Boat (X)",
		L"Double Two-Boat (2)",
		L"Tub (4)",
		L"Two-Boat (6)",
	},
	vector<wstring>{
		L"Clock (K)",
		L"Blinker (L)",
		L"Beacon (N)",
		L"Pulsar (P)",
		L"Octagon 2 (Q)",
		L"Traffic Light (T)",
		L"Pentadecathlon (5)",
		L"Figure 8 (8)",
	},
	vector<wstring>{
		L"Copperhead (C)",
		L"Glider (G)",
		L"Light Weight Ship (V)",
		L"Medium Weight Ship (W)",
	},
	vector<wstring>{
		L"Acord (A)",
		L"R-Pentomino (R)",
	},
};

// mappa dall'ID ai bit
vector<wstring> PatternBits
{
	L"11/11", // block
	L"0100/1010/1001/0110", // loaf
	L"0000110/0001001/0000101/0100010/1010000/1001000/0110000", // 2loaf
	L"010/101/101/010", // beehive
	L"0000001000000/0000010100000/0000010100000/0000001000000/0000000000000/\
0110000000110/1001000001001/0110000000110/0000000000000/0000001000000/\
0000010100000/0000010100000/0000001000000", // 4beehive
	L"01100/10010/01001/00110", // eye
	L"0110/1001/1001/0110", // octagon
	L"1101/1011", // 2l-trimino
	L"011/101/010", // boat
	L"011/101/110", // 2,2boat
	L"010/101/010", // tub
	L"000011/000101/000110/011000/101000/110000", // 2boat
	L"0111/1110", // clock
	L"111", // blinker
	L"0011/0011/1100/1100", // beacon
	L"0011100011100/0000000000000/1000010100001/1000010100001/1000010100001/\
0011100011100/0000000000000/0011100011100/1000010100001/1000010100001/\
1000010100001/0000000000000/0011100011100", // pulsar
	L"0000110000/0001001000/0010000100/0100000010/1000000001/1000000001/0100000010/\
0010000100/0001001000/0000110000", // octagon2
	L"0011100/0000000/1000001/1000001/1000001/0000000/0011100", // traffic light
	L"0010000100/1101111011/0010000100", // pentadecathlon
	L"000011/001011/010000/000010/110100/110000", // figure8
	L"01100110/00011000/00011000/10100101/10000001/00000000/10000001/01100110/\
00111100/00000000/00011000/00011000", // copperhead
	L"010/011/101", // glider
	L"01001/10000/10001/11110", // lwss
	L"0110000/1101111/0111111/0011110", // mwss
	L"0100000/0001000/1100111", // acord
	L"011/110/010", // r-pentomino
	L"" // slot per copia-incolla
};

// funzioni di utilita'
LPARAM Coords;
bool Birth  []{ 0, 0, 0, 1, 0, 0, 0, 0, 0 };
bool Survive[]{ 0, 0, 1, 1, 0, 0, 0, 0, 0 };
vector<int> B{ 3 }, S{ 2, 3 };
inline static int Pixfloor(double x)
{
	ret floor(x / Pix) * Pix;
}
inline static bool In(vector<int> vect, int val)
{
	for (const auto& el : vect) if (el == val) ret true;
	ret false;
}
static vector<int> Parse(wstring text)
{
	vector<int> output;
	for (const auto& c : text) {
		if (!isdigit(c)) ret { -1 };

		if (In(output, c) or c < L'0' or c > L'8') continue;
		output.push_back(c - L'0');
	}

	for (int i = 0; i < output.size(); ++i)
		for (int j = i + 1; j < output.size(); ++j)
			if (output[i] > output[j])
				swap(output[i], output[j]);
	ret output;
}
inline static wstring Elaborator(vector<int> b, vector<int> s)
{
	wstring res{ L"B" };
	for (const auto& c : b) res += char(c + L'0');
	res += L"/S";
	for (const auto& c : s) res += char(c + L'0');
	ret res;
}

// funzione per disegnare la griglia
static void DrawGrid
(HDC hdc, HDC hdcmem, HBITMAP hbmem, uint32_t* pixels, RECT& client)
{
	const int height{ client.bottom - client.top };
	const int width{ client.right - client.left };

	// calcolo coordinata di inizio
	coord Corner{ Position.X - width / 2, Position.Y - height / 2 };
	coord Floor{ Pixfloor(Position.X), Pixfloor(Position.Y) };
	coord Remaining{ Floor.X - Corner.X, Floor.Y - Corner.Y };
	coord Remx{ Pixfloor(Remaining.X), Pixfloor(Remaining.Y) };
	coord Start{ Floor.X - Remx.X, Floor.Y - Remx.Y };
	coord StartRel{ Start.X - Corner.X, Start.Y - Corner.Y };
	coord First{ Start.X / Pix, Start.Y / Pix };

	// calcolo dimensioni
	const int sizex = (width - StartRel.X) / Pix;
	const int sizey = (height - StartRel.Y) / Pix;

	// creazione linee della griglia
	if (ShowGrid)
	{
		// disegno linee
		for (int i = 0; i <= sizex; ++i) { // rette verticali
			
			int IndexI{ First.X + i };
			if (IndexI < 0 or IndexI >= GridSize) continue;

			int px{ StartRel.X + i * Pix };
			if (px < 0 or px >= width) continue;

			for (int y = 0; y < height; ++y)
				pixels[y * width + px] = RGB(255, 255, 255);
		}
		for (int i = 0; i <= sizey; ++i) { // rette orizzontali

			int IndexJ{ First.Y + i };
			if (IndexJ < 0 or IndexJ >= GridSize) continue;

			int py{ StartRel.Y + i * Pix };
			if (py < 0 or py >= height) continue;

			for (int x = 0; x < width; ++x)
				pixels[py * width + x] = RGB(255, 255, 255);
		}

	}

	// disegno celle
	for (int i = -1; i <= sizex; ++i) for (int j = -1; j <= sizey; ++j) {
		int IndexI{ int(Start.X / Pix) + i };
		int IndexJ{ int(Start.Y / Pix) + j };

		if (IndexI < 0 or IndexI >= GridSize or IndexJ < 0 or IndexJ >= GridSize)
			continue;

		// scelta colore
		int age{ Grid[IndexI][IndexJ] }, indexer;
		if (!Color) indexer = (age > 0) ? 6 : 0;
		else if (age <= -32)                indexer = 0;
		else if (-31 <= age and age <= -16) indexer = 1;
		else if (-15 <= age and age <= -8)  indexer = 2;
		else if (-7  <= age and age <= -4)  indexer = 3;
		else if (-3  <= age and age <= -1)  indexer = 4;
		else if (age == 0)                  indexer = 5; 
		else if (age == 1)                  indexer = 6;
		else if (2  <= age and age <= 4)    indexer = 7;
		else if (5  <= age and age <= 9)    indexer = 8;
		else if (10 <= age and age <= 19)   indexer = 9;
		else if (20 <= age and age <= 39)   indexer = 10;
		else if (40 <= age and age <= 79)   indexer = 11;
		else                                indexer = 12;

		// se si sta selezionando un area
		if (selecting_phase != NORMAL)
			if (IndexI == CurrentSelected.X and IndexJ == CurrentSelected.Y)
				indexer = 13;
		if (selecting_phase == SELECTED)
			if (IndexI == SelectionEnd.X and IndexJ == SelectionEnd.Y)
				indexer = 13;
		if (!indexer) continue;

		// colorazione
		int px{ StartRel.X + i * Pix };
		int py{ StartRel.Y + j * Pix };
		for (int y = py; y < py + Pix; ++y) {
			if (y < 0 or y >= height) continue;
			
			for (int x = px; x < Pix + px; ++x) {
				if (x < 0 or x >= width) continue;
				pixels[y * width + x] = AgeColor[indexer];
			}
		}
	}

	// disegno dei confini dell'area di selezione
	if (selecting_phase == SELECTED)
	{
		// calcolo indici delle rette delimitanti
		int hstart, hend, vstart, vend;
		hstart = min(CurrentSelected.Y, SelectionEnd.Y) - First.Y;
		hend   = max(CurrentSelected.Y, SelectionEnd.Y) - First.Y + 1;
		vstart = min(CurrentSelected.X, SelectionEnd.X) - First.X;
		vend   = max(CurrentSelected.X, SelectionEnd.X) - First.X + 1;

		// conversione tra coordinate della griglia e coordinate dello schermo
		LINE_RECT selection{
			StartRel.Y + Pix * hstart,
			StartRel.Y + Pix * hend,
			StartRel.X + Pix * vstart,
			StartRel.X + Pix * vend
		};

		// disegno rette
		COLORREF color{ AgeColor[13] };
		for (int y = max(0, selection.hi); y <= selection.hf; ++y) {
			if (y >= height) break;
			
			if (selection.vi >= 0 and selection.vi < width)
				pixels[y * width + selection.vi] = color;

			if (selection.vf >= 0 and selection.vf < width)
				pixels[y * width + selection.vf] = color;
		}
		for (int x = max(0, selection.vi); x <= selection.vf; ++x) {
			if (x >= width) break;

			if (selection.hi >= 0 and selection.hi < height)
				pixels[selection.hi * width + x] = color;
				
			if (selection.hf >= 0 and selection.hf < height)
				pixels[selection.hf * width + x] = color;
		}
	}
}

// funzione per controllare gli elementi della griglia in modo sicuro
static bool CheckGridBorder(int i, int j)
{
	// bordi morti
	if (!ToroidalBorders) {
		if (i < 0 or i >= GridSize or j < 0 or j >= GridSize) ret false;
		else ret Grid[i][j] > 0;
	}

	// bordi toroidali
	if (i < 0) i += GridSize;
	else if (i >= GridSize) i %= GridSize;
	if (j < 0) j += GridSize;
	else if (j >= GridSize) j %= GridSize;
	ret Grid[i][j] > 0;
}

// funzione che incrementa la posizione della griglia che si usa, nel vettore
static void GridPass()
{
	if (ptr2 < TrackSize - 1) {
		ptr2++;
		ptr = ptr2;
	}
	else {
		ptr = ptr2 = TrackSize - 1;
		offset++;
		if (offset >= TrackSize) offset -= TrackSize;
	}
}

// funzione per calcolare la prossima generazione
static void ElabGrid()
{
	Gen++;
	pGen++;

	// misura del tempo
	int mod = max(1.0, Speed / 50.0);
	if (pGen > 0 and pGen % mod == 0) {
		TimeEnd = chrono::steady_clock::now();
		auto interval = chrono::duration_cast<chrono::milliseconds>(
			TimeEnd - TimeBegin
		).count();

		Fps = mod * 1000.0 / interval;
		TimeBegin = chrono::steady_clock::now();
	}

	// conta delle celle vive
	OldCells = MediumCells = BornCells = AliveCells = 0;
	for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j)
		AliveCells += (Grid[i][j] > 0);

	bool WillBeAlive, WasAlive;
	for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j)
	{
		int AliveNeighbours{};

		// conta dei vicini vivi
		if (0 < i and i < GridSize - 1 and 0 < j and j < GridSize - 1)
			AliveNeighbours =
				(Grid[i - 1][j - 1] > 0) +
				(Grid[i - 1][j]     > 0) +
				(Grid[i - 1][j + 1] > 0) +
				(Grid[i][j - 1]     > 0) +
				(Grid[i][j + 1]     > 0) +
				(Grid[i + 1][j - 1] > 0) +
				(Grid[i + 1][j]     > 0) +
				(Grid[i + 1][j + 1] > 0);
		else for (int di = -1; di <= 1; ++di) for (int dj = -1; dj <= 1; ++dj) {
			if (!di and !dj) continue;
			if (CheckGridBorder(i + di, j + dj)) AliveNeighbours++;
		}

		// calcolo degli stati della cella prima e dopo
		WasAlive = Grid[i][j] > 0;
		WillBeAlive = (!WasAlive and Birth[AliveNeighbours]) or
			(WasAlive and Survive[AliveNeighbours]);

		// calcolo della nuova eta'
		if (WillBeAlive) NewGrid[i][j] = WasAlive ? Grid[i][j] + 1 : 1;
		else NewGrid[i][j] = WasAlive ? 0 : Grid[i][j] - 1;
	}

	// conta delle altre celle
	GridPass();
	for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j) {
		int& age = Grid[i][j] = NewGrid[i][j];
		BornCells += (age == 1);
		MediumCells += (5 <= age and age <= 9);
		OldCells += (age >= 80);
	}
}

// funzione per cambiare la velocità massima della simulazione
static void ModifySpeed(int NewSpeed)
{
	Speed = NewSpeed;

	wostringstream speed;
	speed << fixed << setprecision(1) << Speed / 10.0;
	SetWindowText(
		hLabelSpeed2, (L" " + speed.str() + L" fps").c_str()
	);

	if (!Painting) {
		KillTimer(MainHwnd, 1);
		SetTimer(MainHwnd, 1, 10'000 / Speed, NULL);
	}
}

// funzione per modificare lo stato di una cella in base alle variabili globali
static void Qclick(int i, int j)
{
	if (Selecting or i < 0 or j < 0 or i >= GridSize or j >= GridSize) ret;
	if (Writing) Grid[i][j] = 1;
	else if (Erasing) Grid[i][j] = -32;
	else Grid[i][j] = (Grid[i][j] > 0 ? -32 : 1);
}

// funzione che interpola una retta tra due posizioni ricevute del mouse
static void Connect(int OldexX, int OldexY, int IndexX, int IndexY)
{
	// caso retta verticale
	if (IndexX == OldexX) {
		if (OldexY < IndexY) swap(OldexY, IndexY);
		for (int i = IndexY; i <= OldexY; ++i) Qclick(IndexX, i);
		ret ;
	}

	// caso retta orizzontale
	if (IndexY == OldexY) {
		if (OldexX < IndexX) swap(OldexX, IndexX);
		for (int i = IndexX; i <= OldexX; ++i) Qclick(i, IndexY);
		ret;
	}

	// creazione di un vettore con i punti del rettangolo
	if (IndexX > OldexX) {
		swap(IndexX, OldexX);
		swap(IndexY, OldexY);
	}
	int DeltaX{ OldexX - IndexX }, DeltaY{ OldexY - IndexY };
	vector<vector<int>> cuts(abs(DeltaY) + 1);
	for (int i = 0; i <= abs(DeltaY); ++i) cuts[i].resize(DeltaX + 1, 0);

	// calcolo dei tagli fatti dalla retta che congiunge inizio e fine
	double AngCoeff{ (double)DeltaY / DeltaX };
	for (int j = 0; j < DeltaX; ++j) {
		double cut = 0.5 + (j + 0.5) * abs(AngCoeff);
		cuts[(int)cut][j]++;
		cuts[(int)cut][j + 1]++;

		// la retta taglia un vertice
		if (integer(cut)) {
			cuts[(int)cut][j]--;
			cuts[(int)cut - 1][j]++;
		}
	}
	for (int i = 0; i < abs(DeltaY); ++i) {
		double cut = 0.5 + (i + 0.5) / abs(AngCoeff);
		cuts[i][(int)cut]++;
		cuts[i + 1][(int)cut]++;

		// la retta ha gia' tagliato il vertice
		if (integer(cut)) continue;
	}

	// colorazione finale
	for (int y = 0; y <= abs(DeltaY); ++y) for (int x = 0; x <= DeltaX; ++x)
		if (cuts[y][x] > 0) {
			if (DeltaY > 0) {
				Qclick(IndexX + x, IndexY + y);
				continue;
			}

			// e' necessaria una riflessione
			Qclick(IndexX + x, IndexY - y);
		}
}

// funzione per colorare le caselle in modo avanzato
static coord ClicCell
(int xpos, int ypos, RECT client, int oldxpos = -1, int oldypos = -1)
{
	if (ypos < NoDrawSpace) ret coord{ -1, -1 };

	// calcolo posizione cursore
	coord Endpos{
		Position.X - (client.right - client.left) / 2 + xpos,
		Position.Y - (client.bottom - client.top) / 2 + ypos - NoDrawSpace / 2
	};

	// fuori griglia
	int IndexX = Endpos.X / Pix, IndexY = Endpos.Y / Pix;
	if (IndexX < 0 or IndexX >= GridSize or IndexY < 0 or IndexY >= GridSize)
		ret coord{ -1, -1 };

	// click semplice
	if (oldxpos == -1 and oldypos == -1) {
		Qclick(IndexX, IndexY);
		ret coord{ IndexX, IndexY };
	}

	// interpolazione
	coord Startpos{
		Position.X - (client.right - client.left) / 2 + oldxpos,
		Position.Y - (client.bottom - client.top) / 2 + oldypos
			- NoDrawSpace / 2
	};
	int OldexX = Startpos.X / Pix, OldexY = Startpos.Y / Pix;

	Connect(OldexX, OldexY, IndexX, IndexY);
	ret CurrentSelected;
}

// funzione per disegnare il pattern sulla finestra
static void PastePattern(int patternID = CurrentId)
{
	wstring instr{ PatternBits[patternID - 300] };
	if (patternID == CurrentId) if (ptr2 > 0) ptr2--;
	CurrentId = patternID;

	// misura delle dimensioni
	bool AddToX{ true };
	int sizex{}, sizey{ 1 };
	for (const auto& bit : instr) {
		if (bit == L'/') {
			sizey++;
			AddToX = false;
			continue;
		}
		if (AddToX) sizex++;
	}

	// non si sa mai
	for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j)
		NewGrid[i][j] = Grid[i][j];

	// scrittura
	int dx{}, dy{};
	for (const auto& bit : instr) {
		if (bit == L'/') {
			dy++;
			dx = 0;
			continue;
		}
		else dx++;

		// eventuale applicazione di rotazioni e simmetrie
		int realx{ dx - 1 }, realy{ dy };
		if (vkalt) realx = sizex - 1 - realx;
		if (vkcontrol) realy = sizey - 1 - realy;
		if (vkshift) {
			int oldx{ realx }, oldy{ realy };
			realy = oldx;
			realx = sizey - 1 - oldy;
		}
		coord point{ CurrentSelected.X + realx, CurrentSelected.Y + realy };

		// caso bordi morti
		if (!ToroidalBorders) {
			if (point.X < 0 or point.X >= GridSize
				or point.Y < 0 or point.Y >= GridSize)
				continue;
		}

		// caso bordi toroidali
		else {
			if (point.X < 0) point.X += GridSize;
			else if (point.X >= GridSize) point.X %= GridSize;
			if (point.Y < 0) point.Y += GridSize;
			else if (point.Y >= GridSize) point.Y %= GridSize;
		}

		NewGrid[point.X][point.Y] = (bit == L'1' ? 1 : -32);
	}

	// impostazione corretta della griglia
	GridPass();
	for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j)
		Grid[i][j] = NewGrid[i][j];
}

// funzione per disattivare uno strumento
static void TurnToolOff(bool& stdbool, HWND ID, HICON setter)
{
	if (stdbool) {
		stdbool = false;
#ifndef ONE_FILE
		SendMessage(
			ID, BM_SETIMAGE, IMAGE_ICON, (LPARAM)setter
		);
#endif
	}
	if (!Selecting) selecting_phase = NORMAL;
}

// funzione per elaborare gli input della finestra del grafico a due variabili
static LRESULT CALLBACK WindowProcessor3D(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	bool nofallthrough{ false };
	switch (uMsg)
	{
		// finestra chiusa
	case WM_DESTROY:
		KillTimer(hwnd, 1);
		PostQuitMessage(0);
		ret 0;

		// finestra ridimensionata
	case WM_SIZE:
		InvalidateRect(hwnd, NULL, FALSE);
		ret 0;

		// finestra creata
	case WM_CREATE:
		SetFocus(hwnd);
		UpdateWindow(hwnd);
		InvalidateRect(hwnd, NULL, FALSE);
		ret 0;

		// timer
	case WM_TIMER:
		if (wParam == 1) {
			ElabGrid();

			// frame per secondo
			wostringstream temp;
			temp << fixed << setprecision(2) << Fps << " FPS";
			SetWindowText(hFps, temp.str().c_str());

			// calcolo di alcune percentuali importanti
			wstring percb{ to_wstring(int(100.0 * BornCells / AliveCells)) };
			wstring percm{ to_wstring(int(100.0 * MediumCells / AliveCells)) };
			wstring percl{ to_wstring(int(100.0 * OldCells / AliveCells)) };

			// contatori
			SetWindowText(hGen, (L"Gen: " + to_wstring(Gen)).c_str());
			SetWindowText(
				hLivec, (to_wstring(AliveCells) + L" celle vive "
			).c_str());
			SetWindowText(
				hBornc, (to_wstring(BornCells) + L" celle nate (" + percb + L"%)"
			).c_str());
			SetWindowText(
				hMedc, (to_wstring(MediumCells) + L" celle medie (" + percm + L"%)"
			).c_str());
			SetWindowText(
				hOldc, (to_wstring(OldCells) + L" celle stabili (" + percl + L"%)"
			).c_str());

			InvalidateRect(hwnd, NULL, FALSE);
		}
		ret 0;

		// pulsanti
	case WM_COMMAND: {
		if (HIWORD(wParam) != BN_CLICKED) ret 0;
		auto Loword{ LOWORD(wParam) };
		vector<int> birth, survive;

		// vista della griglia
		if (LOWORD(wParam) == ID_SHOW_GRID) {
			ShowGrid = (SendMessage(hGrid, BM_GETCHECK, 0, 0) == BST_CHECKED);
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

		// stato dei confini della griglia
		else if (LOWORD(wParam) == ID_BORDERS) {
			ToroidalBorders =
				(SendMessage(hBorders, BM_GETCHECK, 0, 0) == BST_CHECKED);
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

		// vista della colorazione delle celle
		else if (LOWORD(wParam) == ID_COLOR) {
			Color = (SendMessage(hColor, BM_GETCHECK, 0, 0) == BST_CHECKED);
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

		// pennello
		else if (LOWORD(wParam) == ID_BRUSH) {
			Pasting = vkshift = vkalt = vkcontrol = false;

			TurnToolOff(Erasing, hRubber, hIconRubberOff);
			TurnToolOff(Selecting, hSelect, hIconSelectOff);

			Writing = !Writing;
#ifndef ONE_FILE
			SendMessage(
				hBrush,
				BM_SETIMAGE, IMAGE_ICON,
				(LPARAM)(Writing ? hIconBrushOn : hIconBrushOff)
			);
#endif
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

		// gomma
		else if (LOWORD(wParam) == ID_RUBBER) {
			Pasting = vkshift = vkalt = vkcontrol = false;

			TurnToolOff(Writing, hBrush, hIconBrushOff);
			TurnToolOff(Selecting, hSelect, hIconSelectOff);

			Erasing = !Erasing;
#ifndef ONE_FILE
			SendMessage(
				hRubber,
				BM_SETIMAGE, IMAGE_ICON,
				(LPARAM)(Erasing ? hIconRubberOn : hIconRubberOff)
			);
#endif
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

		// strumento seleziona
		else if (LOWORD(wParam) == ID_SELECT) {
			Pasting = vkshift = vkalt = vkcontrol = false;

			TurnToolOff(Writing, hBrush, hIconBrushOff);
			TurnToolOff(Erasing, hRubber, hIconRubberOff);

			Selecting = !Selecting;
			if (!Selecting) selecting_phase = NORMAL;

#ifndef ONE_FILE
			SendMessage(
				hSelect,
				BM_SETIMAGE, IMAGE_ICON,
				(LPARAM)(Selecting ? hIconSelectOn : hIconSelectOff)
			);
#endif
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

		// applicazione modifiche delle regole del gioco
		else if (Loword == ID_APPLY_COMMAND)
		{
			// ottenimento stringhe
			wchar_t buffer[64];
			GetWindowText(hBirth, buffer, 64);
			wstring textB(buffer);
			GetWindowText(hSurvive, buffer, 64);
			wstring textS(buffer);

			// modifica delle regole
			birth = Parse(textB);
			survive = Parse(textS);
			if (birth == vector<int>{ -1 } or survive == vector<int>{ -1 })
				ret 0;
		}
		else if (Loword > 400) ret 0;

		// pattern incollati
		else if (Loword > 300) {
			vkshift = vkalt = vkcontrol = false;
			Pasting = true;

			PastePattern(Loword);
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

		// altra parte del blocco che modifica le regole di gioco
		else if (Loword < 200) ret 0;
		else {
			GameMode rule{ Rules[Loword - 200] };
			birth = rule.B;
			survive = rule.S;
		}

		// aggiornamento variabili principali
		B = birth;
		S = survive;
		for (int i = 0; i < 9; ++i) Birth[i] = Survive[i] = false;
		for (int i = 0; i < B.size(); ++i) Birth[B[i]] = true;
		for (int i = 0; i < S.size(); ++i) Survive[S[i]] = true;
		wstring ruleset{ Elaborator(B, S) };
		
		// controllo se la stringa delle regole e' conosciuta
		for (const auto& rul : Rules) if (ruleset == Elaborator(rul.B, rul.S)) {
			ruleset += L" (" + rul.name + L")";
			break;
		}

		// aggiornamento
		SetWindowText(hRuleLabel, (L"REGOLA: " + ruleset).c_str());
		ret 0;
	}

		// uno slider
	case WM_HSCROLL: {
		HWND hSlider = (HWND)lParam;
		if (hSlider == hDensity) {
			Density = (int)SendMessage(hDensity, TBM_GETPOS, 0, 0);

			SetWindowText(
				hLabelDensity2, (L" " + to_wstring(Density) + L"%").c_str()
			);
		}

		else if (hSlider == hSpeed)
			ModifySpeed(SendMessage(hSpeed, TBM_GETPOS, 0, 0));
		ret 0;
	}

		// impostazione icona del cursore
	case WM_SETCURSOR: {
		if (LOWORD(lParam) != HTCLIENT) ret DefWindowProc(hwnd, uMsg, wParam, lParam);
		
		// ottenimento finestra figlia sotto al mouse
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hwnd, &pt);
		HWND hCtrl = ChildWindowFromPoint(hwnd, pt);

		// il mouse e' sopra un pulsante
		if (hCtrl == hBrush or hCtrl == hRubber or hCtrl == hApply
			or hCtrl == hGrid or hCtrl == hBorders or hCtrl == hSelect)
			SetCursor(LoadCursor(nullptr, IDC_HAND));

		// il mouse e' in qualsiasi altro posto
		else SetCursor(LoadCursor(
			nullptr, ((Writing or Erasing or Selecting) ? IDC_CROSS : IDC_ARROW))
		);
		ret TRUE;
	}
		
		// pressione tasto sinistro
	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		enable = true;
		Coords = lParam;
		ret 0;

		// rilascio tasto sinistro
	case WM_LBUTTONUP: {

		// controllo dei casi di disegno
		enable = false;
		if (cursormoved and !Writing and !Erasing) {
			cursormoved = false;
			ret 0;
		}

		// ottenimento coordinate cursore
		RECT client;
		GetClientRect(hwnd, &client);
		int xPos = GET_X_LPARAM(lParam), yPos = GET_Y_LPARAM(lParam);
		
		// esecuzione click
		coord clicked;
		if ((clicked = ClicCell(xPos, yPos, client)) == coord{ -1, -1 }) ret 0;
		
		// gestione dei casi di disegno
		Pasting = vkshift = vkalt = vkcontrol = false;
		if (selecting_phase == SELECTED) selecting_phase = NORMAL;
		else if (Selecting) selecting_phase++;
		if (selecting_phase != SELECTED) {
			if (!Writing and !Erasing) CurrentSelected = clicked;
		}
		else SelectionEnd = clicked;

		InvalidateRect(hwnd, NULL, FALSE);
		ret 0;
	}

		// cursore fuori dalla finestra
	case WM_MOUSELEAVE:
		enable = false;
		ret 0;

		// traslazione
	case WM_MOUSEMOVE: {

		// tracking del mouse
		TRACKMOUSEEVENT tme{};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hwnd;
		TrackMouseEvent(&tme);
		if (!enable) ret 0;

		// ottenimento coordinate
		RECT client;
		GetClientRect(hwnd, &client);
		int OldXpos = GET_X_LPARAM(Coords), OldYpos = GET_Y_LPARAM(Coords);
		int xPos = GET_X_LPARAM(lParam), yPos = GET_Y_LPARAM(lParam);
		if (yPos < NoDrawSpace) {
			enable = false;
			ret 0;
		}

		// pennello o gomma
		if (Writing or Erasing) {
			if (ClicCell(xPos, yPos, client, OldXpos, OldYpos).X == -1) ret 0;
		}
		
		// movimento della griglia
		else {
			Position.X += OldXpos - xPos;
			Position.Y += OldYpos - yPos;
			cursormoved = true;
		}
		
		Coords = lParam;
		InvalidateRect(hwnd, NULL, FALSE);
		ret 0;
	}

		// zoom
	case WM_MOUSEWHEEL: {
		int WheelData = GET_WHEEL_DELTA_WPARAM(wParam) / 120;
		bool decrease{ WheelData < 0 };
		WheelData = abs(WheelData);

		// aumento dimensione pixel
		int woldpix{ Pix };
		for (int i = 0; i < WheelData; ++i) {
			if (decrease) {
				Pix /= 1.2;
				Pix = max(Pix, PixMin);
				continue;
			}

			int OldPix{ Pix };
			Pix *= 1.2;
			Pix = min(Pix, PixMax);
			if (Pix == OldPix) Pix++;
		}
		
		// modifica posizione (in pixel veri)
		double modifier = (double)Pix / woldpix;
		Position.X *= modifier;
		Position.Y *= modifier;

		InvalidateRect(hwnd, NULL, FALSE);
		ret 0;
	}

		// tasto di sistema
	case WM_SYSKEYDOWN:
		if (wParam != VK_MENU) nofallthrough = true;
		[[fallthrough]];

		// tasto premuto
	case WM_KEYDOWN:
		if (nofallthrough) ret 0;
		
		// viene incollato un pattern
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000 and
			wParam != L'Y' and wParam != L'Z' and wParam != VK_CONTROL
			and !Writing and !Erasing)
		{
			// copia o incolla
			bool set{ false };
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
			{
				// copia della selezione
				if (wParam == L'C' and selecting_phase == SELECTED) {
					int xmin, xmax, ymin, ymax;
					xmin = min(CurrentSelected.X, SelectionEnd.X);
					xmax = max(CurrentSelected.X, SelectionEnd.X);
					ymin = min(CurrentSelected.Y, SelectionEnd.Y);
					ymax = max(CurrentSelected.Y, SelectionEnd.Y);

					// lettura dei bit nella griglia e salvataggio
					wstring construct;
					for (int j = ymin; j <= ymax; ++j) {
						for (int i = xmin; i <= xmax; ++i)
							construct += (Grid[i][j] > 0) ? L'1' : L'0';
						
						if (j != ymax) construct += L'/';
					}
					PatternBits.back() = construct;

					ret 0;
				}

				if (wParam != L'V') ret 0;
				set = true;
			}
			if (Selecting and !set) ret 0;
			CurrentId = 0;
			Pasting = true;
			vkshift = vkalt = vkcontrol = false;

			int semi_id = wstring(L"BDEHIJOSX246KLNPQT58CGVWAR").find(wParam);
			if (semi_id == wstring::npos) ret 0;
			if (set) semi_id = PatternBits.size() - 1;
			PastePattern(semi_id + 300);

			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

		// messaggi dei tasti
		switch (wParam)
		{
			// shift
		case VK_SHIFT:
			if (Pasting) {
				vkshift = !vkshift;
				PastePattern();

				InvalidateRect(hwnd, NULL, FALSE);
				ret 0;
			}
			ret 0;
			
			// alt
		case VK_MENU:
			if (Pasting) {
				vkalt = !vkalt;
				PastePattern();

				InvalidateRect(hwnd, NULL, FALSE);
				ret 0;
			}
			ret 0;

			// ctrl
		case VK_CONTROL:
			if (Pasting) {
				vkcontrol = !vkcontrol;
				if (CurrentId >= 300) PastePattern();

				InvalidateRect(hwnd, NULL, FALSE);
				ret 0;
			}
			ret 0;

			// invio
		case 13:
			pGen = 0;
			if (Painting) {
				Painting = false;
				if (Pasting) Pasting = vkshift = vkalt = vkcontrol = false;
				SetTimer(hwnd, 1, 10'000.0 / Speed, NULL);
				ret 0;
			}

			Painting = true;
			KillTimer(hwnd, 1);
			ret 0;

			// cancellamento dell'area selezionata
		case VK_DELETE:
			nofallthrough = true;
			[[fallthrough]];
		case L'\b': {
			if (selecting_phase != SELECTED and !nofallthrough) break;
			if (Painting) pGen = Gen = 0;

			int xmin{}, xmax{ GridSize - 1 }, ymin{}, ymax{ GridSize - 1 };
			if (selecting_phase == SELECTED) {
				xmin = min(CurrentSelected.X, SelectionEnd.X);
				xmax = max(CurrentSelected.X, SelectionEnd.X);
				ymin = min(CurrentSelected.Y, SelectionEnd.Y);
				ymax = max(CurrentSelected.Y, SelectionEnd.Y);
			}

			for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j)
				NewGrid[i][j] = Grid[i][j];
			for (int i = xmin; i <= xmax; ++i) for (int j = ymin; j <= ymax; ++j)
				NewGrid[i][j] = -32;
			GridPass();
			for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j)
				Grid[i][j] = NewGrid[i][j];
			
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

			// termine del paste di un pattern
		case L' ':
			if (Pasting) Pasting = vkshift = vkalt = vkcontrol = false;
			ret 0;

			// modifica della velocita'
		case VK_OEM_PLUS:
			ModifySpeed(min(SpeedLimit, Speed + 40));
			SendMessage(hSpeed, TBM_SETPOS, TRUE, Speed);
			break;
		case VK_OEM_MINUS:
			ModifySpeed(max(SpeedMinimum, Speed - 40));
			SendMessage(hSpeed, TBM_SETPOS, TRUE, Speed);
			break;

			// ritorno alla griglia precedente
		case L'Z':
			if (!Painting) ret 0;
			if (Pasting) Pasting = vkshift = vkalt = vkcontrol = false;
			if (!GetAsyncKeyState(VK_CONTROL) & 0x8000) ret 0;

			if (ptr2 > 0) ptr2--;
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;

			// ritorno alla griglia successiva
		case L'Y':
			if (!Painting) ret 0;
			if (Pasting) Pasting = vkshift = vkalt = vkcontrol = false;
			if (!GetAsyncKeyState(VK_CONTROL) & 0x8000) ret 0;

			if (ptr2 < ptr) ptr2++;
			else {
				ElabGrid();
				Gen++;
				pGen = 0;
			}
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;

			// schermo intero
		case VK_F11:
#pragma region vkf11
			if (!Fullscreen) {
				
				// salvataggio dello stato
				Fullscreen = true;
				WindowStyle = GetWindowLong(hwnd, GWL_STYLE);
				GetWindowRect(hwnd, &WindowRect);
			
				// hide dei controlli e del menu
				SetMenu(hwnd, NULL);
				for (auto& handle : handles) ShowWindow(handle, SW_HIDE);

				// impostazione a schermo intero
				SetWindowLong(hwnd, GWL_STYLE, WindowStyle & ~(WS_OVERLAPPEDWINDOW));
				MONITORINFO mi{ sizeof(MONITORINFO) };
				GetMonitorInfo(
					MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi
				);
				NoDrawSpace = 0;

				// impostazione delle dimensioni a monitor intero
				SetWindowPos(
					hwnd,
					HWND_TOP,
					mi.rcMonitor.left,
					mi.rcMonitor.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_FRAMECHANGED
				);

				InvalidateRect(hwnd, NULL, FALSE);
				ret 0;
			}
			// uscita da schermo intero
			
			// ripristino dimensioni
			SetWindowLong(hwnd, GWL_STYLE, WindowStyle);
			SetWindowPos(
				hwnd,
				NULL,
				WindowRect.left,
				WindowRect.top,
				WindowRect.right - WindowRect.left,
				WindowRect.bottom - WindowRect.top,
				SWP_FRAMECHANGED
			);
			NoDrawSpace = 5 * vs + 4 * H;

			// ripristino dei controlli
			SetMenu(hwnd, hMenu);
			for (auto& handle : handles) ShowWindow(handle, SW_SHOW);

			Fullscreen = false;
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
#pragma endregion

			// reset posizione e zoom
		case L'R':
			Pix = 20;
			enable = false;
			Position = coord{ int(GridSize * Pix / 2), int(GridSize * Pix / 2) };
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;

			// riempimento casuale
		case L'X': {
			if (Painting) pGen = Gen = 0;

			int xmin{}, xmax{ GridSize - 1 }, ymin{}, ymax{ GridSize - 1 };
			if (selecting_phase == SELECTED) {
				xmin = min(CurrentSelected.X, SelectionEnd.X);
				xmax = max(CurrentSelected.X, SelectionEnd.X);
				ymin = min(CurrentSelected.Y, SelectionEnd.Y);
				ymax = max(CurrentSelected.Y, SelectionEnd.Y);
			}
			
			for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j)
				NewGrid[i][j] = Grid[i][j];
			for (int i = xmin; i <= xmax; ++i) for (int j = ymin; j <= ymax; ++j)
				NewGrid[i][j] = (density_roll(gen) <= Density ? 1 : -32);
			GridPass();
			for (int i = 0; i < GridSize; ++i) for (int j = 0; j < GridSize; ++j)
				Grid[i][j] = NewGrid[i][j];

			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		}

			// tasti WASD
		case L'W':
			Position.Y -= 8 * Pix;
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		case L'A':
			Position.X -= 8 * Pix;
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		case L'S':
			Position.Y += 8 * Pix;
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;
		case L'D':
			Position.X += 8 * Pix;
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;

			// mostra griglia
		case L'G':
			ShowGrid = !ShowGrid;
			SendMessage(
				hGrid,
				BM_SETCHECK,
				(ShowGrid? BST_CHECKED : BST_UNCHECKED),
				NULL
			);
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;

			// confini toroidali
		case L'T':
			ToroidalBorders = !ToroidalBorders;
			SendMessage(
				hBorders,
				BM_SETCHECK,
				(ToroidalBorders ? BST_CHECKED : BST_UNCHECKED),
				NULL
			);
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;

			// colore
		case L'C':
			Color = !Color;
			SendMessage(
				hColor,
				BM_SETCHECK,
				(Color ? BST_CHECKED : BST_UNCHECKED),
				NULL
			);
			InvalidateRect(hwnd, NULL, FALSE);
			ret 0;

		default: ret DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		ret 0;

		// aggiornamento dei pixel
	case WM_PAINT: {
		
		// inizio disegno
		PAINTSTRUCT ps;
		RECT client;
		GetClientRect(hwnd, &client);
		if (client.right == 0 and client.bottom == 0)
			ret DefWindowProc(hwnd, uMsg, wParam, lParam);
		client.top = NoDrawSpace;
		HDC hdc = BeginPaint(hwnd, &ps);

		// creazione del bitmapinfo
		BITMAPINFO bmi{};
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = client.right - client.left;
		bmi.bmiHeader.biHeight = -(client.bottom - client.top);
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		// creazione del backbuffer
		void* bits = nullptr;
		HDC hdcmem{ CreateCompatibleDC(hdc) };
		HBITMAP hbmem = CreateDIBSection(
			hdcmem,
			&bmi,
			DIB_RGB_COLORS,
			&bits,
			nullptr,
			0
		);
		uint32_t* pixels = static_cast<uint32_t*>(bits);

		// disegno griglia
		SelectObject(hdcmem, hbmem);
		DrawGrid(hdc, hdcmem, hbmem, pixels, client);
		BitBlt(
			hdc,
			client.left,
			client.top,
			client.right - client.left,
			client.bottom - client.top,
			hdcmem,
			0, 0,
			SRCCOPY
		);

		// fine disegno
		DeleteObject(hbmem);
		DeleteDC(hdcmem);
		EndPaint(hwnd, &ps);
		ret 0;
	}

	default: ret DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

// funzioni per creare gli elementi della finestra
static HWND Static(wstring text, int x, int y, int dx, int dy)
{
	ret CreateWindowEx(
		0, L"STATIC", text.c_str(),
		WS_CHILD | WS_VISIBLE,
		x, y, dx, dy,
		MainHwnd, NULL,
		MainHinstance, NULL
	);
}
static HWND Edit(int Id, wstring startext, int x, int y, int dx, int dy)
{
	ret CreateWindowEx(
		0, L"EDIT",
		startext.c_str(),
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
		x, y, dx, dy,
		MainHwnd, (HMENU)Id,
		MainHinstance,
		NULL
	);
}
static HWND Button(int Id, wstring text, int x, int y, int dx, int dy)
{
	ret CreateWindowEx(
		0, L"BUTTON",
		text.c_str(),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, dx, dy,
		MainHwnd, (HMENU)Id,
		MainHinstance,
		NULL
	);
}
static HWND ButtonIcon(int Id, wstring text, int x, int y, int dx, int dy)
{
	ret CreateWindowEx(
		0, L"BUTTON",
		text.c_str(),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON,
		x, y, dx, dy,
		MainHwnd, (HMENU)Id,
		MainHinstance,
		NULL
	);
}
static HWND Checkbox(int Id, wstring text, int x, int y, int dx, int dy)
{
	HWND Hwnd = CreateWindowEx(
		0, L"BUTTON",
		text.c_str(),
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
		x, y, dx, dy,
		MainHwnd, (HMENU)Id,
		MainHinstance,
		NULL
	);
	SendMessage(Hwnd, BM_SETCHECK, BST_CHECKED, 0);
	ret Hwnd;
}
static HWND Slider
(int Id, int start, int mid, int end, int x, int y, int dx, int dy)
{
	HWND Hwnd = CreateWindowEx(
		0, TRACKBAR_CLASS, L"",
		WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
		x, y, dx, dy,
		MainHwnd, (HMENU)Id,
		MainHinstance,
		NULL
	);
	SendMessage(Hwnd, TBM_SETRANGE, TRUE, MAKELPARAM(start, end));
	SendMessage(Hwnd, TBM_SETPOS, TRUE, mid);
	ret Hwnd;
}
static HICON Icon(int Id)
{
	ret (HICON)LoadImage(
		MainHinstance,
		MAKEINTRESOURCE(Id),
		IMAGE_ICON,
		32, 32,
		LR_DEFAULTCOLOR
	);
}
static void TextOnMenu(HMENU menu, wstring text)
{
	AppendMenu(
		menu, MF_STRING | MF_DISABLED, 0, text.c_str()
	);
}

int main()
{
	// allocazione della memoria
	for (int i = 0; i < GridSize; ++i) NewGrid[i] = new int[GridSize];
	for (auto& grid : Backtrack) {
		grid = new int* [GridSize];
		for (int i = 0; i < GridSize; ++i) {
			grid[i] = new int[GridSize];
			for (int j = 0; j < GridSize; ++j) grid[i][j] = -32;
		}
	}

	// costruzione di un vettore con tutti gli universi
	int counter{ 200 };
	for (auto& subvect : RuleLibrary) for (const auto& rule : subvect) {
		GameMode Test;
		Test.ID = counter++;
		Test.name = rule.name;
		Test.params = rule.params;

		size_t one{ rule.params.find(L'B') };
		size_t two{ rule.params.find(L'S') };
		Test.B = Parse(rule.params.substr(one + 1, two - one - 2));
		Test.S = Parse(rule.params.substr(two + 1));

		Rules.push_back(Test);
	}

	// common controls
	INITCOMMONCONTROLSEX icc{};
	icc.dwSize = sizeof(icc);	
	icc.dwICC = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

	// dati finestra
	HINSTANCE hInstance = MainHinstance = GetModuleHandle(0);
	WNDCLASS wc{};
	wc.lpfnWndProc = WindowProcessor3D;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"conway";
#ifndef ONE_FILE
	wc.hIcon = LoadIcon(
		hInstance,
		MAKEINTRESOURCE(IDI_APPICON)
	);
#endif
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	RegisterClass(&wc);

	// creazione finestra
	HWND hwnd = MainHwnd = CreateWindowEx(
		0,
		L"conway", L"The Conway Multiverse",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1200, 700,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	if (!hwnd) {
		wcerr << "Qualcosa e' andato storto\n";
		ret 0;
	}
	ShowWindow(hwnd, SW_SHOW);
	
#pragma region menu
	// creazione del menu
	counter = 200;
	hMenu = CreateMenu();
	vector<HMENU> hPopupMenus(Categories.size());
	for (size_t i = 0; i < Categories.size(); ++i) {
		hPopupMenus[i] = CreatePopupMenu();
		
		for (const auto& rule : RuleLibrary[i]) AppendMenu(
			hPopupMenus[i], MF_STRING, UINT(counter++), rule.name.c_str()
		);

		AppendMenu(
			hMenu,
			MF_POPUP,
			(UINT_PTR)hPopupMenus[i],
			Categories[i].c_str()
		);
	}

	// aggiunta del doppio popup menu con i pattern
	HMENU hPatterns = CreatePopupMenu();
	counter = 300;
	for (int i = 0; i < PatternTypes.size(); ++i) {
		HMENU hType = CreatePopupMenu();

		for (const auto& pattern : Patterns[i]) AppendMenu(
			hType, MF_STRING, counter++, pattern.c_str()
		);

		AppendMenu(
			hPatterns, MF_POPUP, (UINT_PTR)hType, PatternTypes[i].c_str()
		);
	}
	AppendMenu(hPatterns, MF_SEPARATOR, 0, nullptr);
	TextOnMenu(hPatterns, L"Shift: rotate 90 degrees          ");
	TextOnMenu(hPatterns, L"Alt: mirror X                     ");
	TextOnMenu(hPatterns, L"Ctrl: mirror Y                    ");
	TextOnMenu(hPatterns, L"Space: confirm direction          ");
	TextOnMenu(hPatterns, L"CTRL + SHIFT + C:  COPY (selected)");
	TextOnMenu(hPatterns, L"CTRL + SHIFT + V:  PASTE (copied) ");
	AppendMenu(
		hMenu, MF_POPUP, (UINT_PTR)hPatterns, L"Patterns (CTRL + Key)"
	);
	SetMenu(hwnd, hMenu);
#pragma endregion
	
	// elementi della finestra
#pragma region elements
	hLabelDensity       = Static    (L"Densita': ",
		vs, vs, L, H
	);
	hDensity            = Slider    (ID_DENSITY, 1, Density, 100,
		vs + hs + L, vs, LL, H
	);
	hLabelDensity2      = Static    (L" 30%",
		vs + 2 * hs + L + LL, vs, L, H
	);
	hFps                = Static    (L"",
		vs + 3 * hs + 2 * L + LL, vs, ML, H
	);
	hGen                = Static    (L"",
		vs + 4 * hs + 2 * L + LL + ML, vs, ML, H
	);
	hLivec              = Static    (L"",
		vs + 5 * hs + 2 * L + LL + 2 * ML, vs, ML, H
	);

	hBrush              = ButtonIcon(ID_BRUSH, L"",
		vs + 6 * hs + 2 * L + LL + 3 * ML, vs, 2 * H, 2 * H
	);
	hRubber             = ButtonIcon(ID_RUBBER, L"",
		vs + 7 * hs + 2 * L + LL + 3 * ML + 2 * H, vs, 2 * H, 2 * H
	);
	hSelect             = ButtonIcon(ID_SELECT, L"",
		vs + 6 * hs + 2 * L + LL + 3 * ML, hs + vs + 2 * H, 2 * H, 2 * H
	);

	hLabelSpeed         = Static    (L"Velocita': ",
		vs, 2 * vs + H, L, H
	);
	hSpeed              = Slider    (ID_SPEED, SpeedMinimum, Speed, SpeedLimit,
		vs + hs + L, 2 * vs + H, LL, H
	);
	hLabelSpeed2        = Static    (L" 20 fps",
		vs + 2 * hs + L + LL, 2 * vs + H, L, H
	);
	hBornc              = Static    (L"",
		vs + 3 * hs + 2 * L + LL, 2 * vs + H, ML, H
	);
	hMedc               = Static    (L"",
		vs + 4 * hs + 2 * L + LL + ML, 2 * vs + H, ML, H
	);
	hOldc               = Static    (L"",
		vs + 5 * hs + 2 * L + LL + 2 * ML, 2 * vs + H, ML, H
	);

	hGrid               = Checkbox  (ID_SHOW_GRID, L"Mostra Griglia (G)",
		vs, 3 * vs + 2 * H, 2 * L, H
	);
	hBorders            = Checkbox  (ID_BORDERS, L"Confini Toroidali (T)",
		vs + hs + 2 * L, 3 * vs + 2 * H, 2.5 * L, H
	);
	hColor              = Checkbox  (ID_COLOR, L"Colore Celle (C)",
		vs + 2 * hs + 4.5 * L, 3 * vs + 2 * H, 2 * L, H
	);

	hLabelBirth         = Static    (L"Birth: ",
		vs, 4 * vs + 3 * H, L, H
	);
	hBirth              = Edit      (ID_EDIT_BIRTH, L"3",
		vs + hs + L, 4 * vs + 3 * H, SL, H
	);
	hLabelSurvive       = Static    (L"Survive: ",
		vs + 2 * hs + L + SL, 4 * vs + 3 * H, L, H
	);
	hSurvive            = Edit      (ID_EDIT_SURVIVE, L"23",
		vs + 3 * hs + 2 * L + SL, 4 * vs + 3 * H, SL, H
	);
	hApply              = Button    (ID_APPLY_COMMAND, L"APPLICA",
		vs + 4 * hs + 2 * L + 2 * SL, 4 * vs + 3 * H, L, H
	);
	hRuleLabel          = Static    (L"REGOLA: B3/S23 (Conway's Game of Life)",
		vs + 5 * hs + 3 * L + 2 * SL, 4 * vs + 3 * H, LL, H
	);
#pragma endregion

#pragma region icons
	// Tooltip
	TooltipHwnd = CreateWindowEx(
		WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		NULL,
		WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hwnd,
		NULL,
		hInstance,
		NULL
	);

	// Tooltip dello strumento seleziona
	TOOLINFO ti{};
	ti.cbSize = TTTOOLINFOW_V2_SIZE;
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = hwnd;
	ti.uId = (UINT_PTR)hSelect;
	ti.lpszText = const_cast<LPWSTR>(
		L"Select Tool: click the first cell of the rectangle, \
then click the last cell"
	);
	SendMessage(TooltipHwnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
	SendMessage(TooltipHwnd, TTM_SETDELAYTIME, TTDT_INITIAL, 20);

#ifndef ONE_FILE
	// icone degli strumenti
	hIconBrushOff  = Icon(IDI_BRUSH);
	hIconRubberOff = Icon(IDI_ERASER);
	hIconBrushOn   = Icon(IDI_BRUSH_ON);
	hIconRubberOn  = Icon(IDI_ERASER_ON);
	hIconSelectOff = Icon(IDI_SELECT);
	hIconSelectOn  = Icon(IDI_SELECT_ON);
	
	SendMessage(hBrush , BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconBrushOff );
	SendMessage(hRubber, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconRubberOff);
	SendMessage(hSelect, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconSelectOff);
#endif
#pragma endregion

	// ciclo dei messaggi
	MSG msg{}; int io{};
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	ret 0;
}
///