/*
	OneLoneCoder.com - Command Line First Person Shooter (FPS) Engine
	"Why were games not done like this is 1990?" - @Javidx9

	License
	~~~~~~~
	Copyright (C) 2018  Javidx9
	This program comes with ABSOLUTELY NO WARRANTY.
	This is free software, and you are welcome to redistribute it
	under certain conditions; See license for details. 
	Original works located at:
	https://www.github.com/onelonecoder
	https://www.onelonecoder.com
	https://www.youtube.com/javidx9

	GNU GPLv3
	https://github.com/OneLoneCoder/videos/blob/master/LICENSE

	From Javidx9 :)
	~~~~~~~~~~~~~~~
	Hello! Ultimately I don't care what you use this for. It's intended to be 
	educational, and perhaps to the oddly minded - a little bit of fun. 
	Please hack this, change it and use it in any way you see fit. You acknowledge 
	that I am not responsible for anything bad that happens as a result of 
	your actions. However this code is protected by GNU GPLv3, see the license in the
	github repo. This means you must attribute me if you use it. You can view this
	license here: https://github.com/OneLoneCoder/videos/blob/master/LICENSE
	Cheers!

	Background
	~~~~~~~~~~
	Whilst waiting for TheMexicanRunner to start the finale of his NesMania project,
	his Twitch stream had a counter counting down for a couple of hours until it started.
	With some time on my hands, I thought it might be fun to see what the graphical
	capabilities of the console are. Turns out, not very much, but hey, it's nice to think
	Wolfenstein could have existed a few years earlier, and in just ~200 lines of code.

	IMPORTANT!!!!
	~~~~~~~~~~~~~
	READ ME BEFORE RUNNING!!! This program expects the console dimensions to be set to 
	120 Columns by 40 Rows. I recommend a small font "Consolas" at size 16. You can do this
	by running the program, and right clicking on the console title bar, and specifying 
	the properties. You can also choose to default to them in the future.
	
	Controls: A = Turn Left, D = Turn Right, W = Walk Forwards, S = Walk Backwards

	Future Modifications
	~~~~~~~~~~~~~~~~~~~~
	1) Shade block segments based on angle from player, i.e. less light reflected off
	walls at side of player. Walls straight on are brightest.
	2) Find an interesting and optimised ray-tracing method. I'm sure one must exist
	to more optimally search the map space
	3) Add bullets!
	4) Add bad guys!

	Author
	~~~~~~
	Twitter: @javidx9
	Blog: www.onelonecoder.com

	Video:
	~~~~~~	
	https://youtu.be/xW8skO7MFYw

	Last Updated: 27/02/2017
*/

#include <iostream>
#include <algorithm>
#include <utility>
#include <vector>
#include <thread>
#include <chrono>
using namespace std;
#include <math.h>
#include <stdio.h>
#include <Windows.h>
// #include "utils.hpp"
// #include "olcConsoleGameEngine.h"

const int nScreenWidth = 120;
const int nScreenHeight = 40;

const int nMapHeight = 32;
const int nMapWidth = 32;

const float fFOV = 3.14159 / 4.0; // field of view
const float fDepth = 30.0f; // the max distance we can see

float fPlayerX = 1.1f; // position of player
float fPlayerY = 1.1f; 
float fPlayerA = 0.0f; // orientation of player
const float fPlayerSpeedMov = 4.0;
const float fPlayerSpeedRot = 0.8;
float fPlayerHP = 100;
const float fShootSpeed = 2; // the minimum seconds between shootings

const int nNumMonsters = 5;
const float fMonSpeed = 0.02;
const float fMonDamage = 2.0;
float fMonWidth = 0.15; // the threshold for angle detect, using both for the drawing of monsters, and the bullet hitting


int main()
{
    // create screen buffer
    wchar_t *screen = new wchar_t[nScreenWidth*nScreenWidth];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    wstring map;
    map += L"################################";
    map += L"#......#.......#......#........#";
    map += L"#......#................#......#";
    map += L"#......#.............####......#";
    map += L"#..........#............#......#";
    map += L"#..........#...................#";
    map += L"#..............................#";
    map += L"#####..........................#";
    map += L"#........................#######";
    map += L"#..............................#";
    map += L"#..............................#";
    map += L"#..............................#";
    map += L"#.......#######.........#......#";
    map += L"#.......................#......#";
    map += L"#..............................#";
    map += L"#.......#......................#";
    map += L"#...#######....................#";
    map += L"#..............................#";
    map += L"#..............................#";
    map += L"#.................#............#";
    map += L"#..........#......#............#";
    map += L"###........#......#....#####...#";
    map += L"#.................#....#####...#";
    map += L"#.......#.........#............#";
    map += L"#.......#.........#............#";
    map += L"#.......#......................#";
    map += L"#.......#...............#......#";
    map += L"#.......#...............#......#";
    map += L"#.........#####.........####...#";
    map += L"#..............................#";
    map += L"#.......................#......#";
    map += L"################################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    // initialize monsters
    srand((unsigned int)(time(NULL)));
    float *fMonsterPositionX = new float[nNumMonsters];
    float *fMonsterPositionY = new float[nNumMonsters];
    bool *bMonDeath = new bool[nNumMonsters]();
    for(int n = 0; n < nNumMonsters; n++)
    {
        fMonsterPositionX[n] = rand() % nMapWidth;
        fMonsterPositionY[n] = rand() % nMapHeight;
        // initial position cant be too close to player
        while((sqrt((fMonsterPositionX[n] - fPlayerX)*(fMonsterPositionX[n] - fPlayerX) +    \
                   (fMonsterPositionY[n] - fPlayerY)*(fMonsterPositionY[n] - fPlayerY)) < 3) \
           || (map[(int)fMonsterPositionY[n] * nMapWidth + (int)fMonsterPositionX[n]] == '#'))
        {
            fMonsterPositionX[n] = rand() % nMapWidth;
            fMonsterPositionY[n] = rand() % nMapHeight;
        }
    }

    bool bShooting = false;
    float fShot = 0.0f; // record CD of gun
    
    bool bGameOver = false;
    // game loop
    while(fPlayerHP > 0)
    {
        this_thread::sleep_for(10ms);

        auto tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        // control
        if(GetAsyncKeyState((unsigned short)'Q') & 0x8000)
            fPlayerA -= fPlayerSpeedRot * fElapsedTime; // Computers run the same program at different speeds under different loads, so time is multiplied here to achieve a more consistent control effect.
        if(GetAsyncKeyState((unsigned short)'E') & 0x8000)
            fPlayerA += fPlayerSpeedRot * fElapsedTime;
        if(GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            fPlayerX += cosf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            fPlayerY += sinf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') // collision detection
            {
                fPlayerX -= cosf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
                fPlayerY -= sinf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            }
        }
        if(GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            fPlayerX -= cosf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            fPlayerY -= sinf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') // collision detection
            {
                fPlayerX += cosf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
                fPlayerY += sinf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            }
        }
        if(GetAsyncKeyState((unsigned short)'A') & 0x8000)
        {
            fPlayerX += sinf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') // collision detection
            {
                fPlayerX -= sinf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            }
        }
        if(GetAsyncKeyState((unsigned short)'D') & 0x8000)
        {
            fPlayerX -= sinf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') // collision detection
            {
                fPlayerX += sinf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * fPlayerSpeedMov * fElapsedTime;
            }
        }
        if(fShot <= 0)
        {
            if(GetAsyncKeyState((unsigned short)' ') & 0x8000)
            {
                bShooting = true;
                fShot = fShootSpeed;
            }
        }
        else
        {
            bShooting = false;
            fShot -= fElapsedTime;
        }

        // monsters move
        for(int n = 0; n < nNumMonsters; n++)
        {
            if(bMonDeath[n])
                continue;
            
            float vx = fPlayerX - fMonsterPositionX[n];
            float vy = fPlayerY - fMonsterPositionY[n];
            float d = sqrt(vx * vx + vy * vy);
            fMonsterPositionX[n] += fMonSpeed * vx / d; // monsters move towards player
            fMonsterPositionY[n] += fMonSpeed * vy / d;
            if(map[(int)fMonsterPositionY[n] * nMapWidth + (int)fMonsterPositionX[n]] == '#') // collision detection
            {
                fMonsterPositionX[n] -= fMonSpeed * vx / d;
                fMonsterPositionY[n] -= fMonSpeed * vy / d;
            }
            else if(d <= 1) // player takes damage
            {
                fPlayerHP -= fElapsedTime * fMonDamage;

                if(d < 0.8) // collision detection
                {
                    fMonsterPositionX[n] -= fMonSpeed * vx / d;
                    fMonsterPositionY[n] -= fMonSpeed * vy / d;
                }
            }
        }

        // for each column of the view
        for(int x = 0; x < nScreenWidth; x++)
        {
            //using the method of ray tracing, the ray starts from the eye of player, and travels until hitting a wall or travels too far

            // for each column, calculate the projected ray angle into world space
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            float fEyeX = cosf(fRayAngle); // Unit vector for ray in player space
            float fEyeY = sinf(fRayAngle);

            float fDistanceToWall = 0; // how far the ray has traveled
            bool bHitWall = false; // if the ray reached the wall
            bool bHitBoundary = false; // if the ray reached the boundary of wall block

            // start the ray
            while(!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += 0.1f;

                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall); // where the ray ends
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall); // cause the map is discrete, so we use int

                // check if ray is out of bounds
                if(nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;
                    fDistanceToWall = fDepth; // set distance to maxmium depth
                }
                else
                {
                    // ray is inbounds so check if the reached cell is a wall block
                    if(map[nTestY * nMapWidth + nTestX] == '#')
                    {
                        bHitWall = true;

                        // now detect the boundary: make four rays trace back to the player from each corner of the block,
                        // if the ray we now emit is very similar to one of the four rays, then the current ray reaches the boundary of wall block
                        // wonder why use 4 corners: plus 0 and plus 1, why not 9 corners: minus 1, plus 0 and plus 1? and then use the middle one ,instead of the closest
                        vector<pair<float, float>> p; // distance, dot
                        // for each corner
                        for(int tx = 0; tx < 2; tx++)
                            for(int ty = 0; ty < 2; ty++)
                            {
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx*vx + vy*vy); // the distance between this corner and player
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d); // dot product of unit vectors of player ray and corner ray
                                p.push_back(make_pair(d, dot));
                            }

                        // in order to avoid showing the corner at the back of the block,
                        // we sort pairs from closest to farthest, and we only consider the closest 2 corners
                        // it has a bug I think, if we look at the block from a skewed view, a back corner could be closer than a outside corner
                        // wait, no, the column has already been drawn at the last block
                        sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right) {return left.first < right.first;});

                        float fBound = 0.01; // threshold of the angle between two rays
                        // a·b = |a| |b| cosθ = cosθ
                        if(acos(p.at(0).second) < fBound) bHitBoundary = true;
                        else if(acos(p.at(1).second) < fBound) bHitBoundary = true;
                        // else if(acos(p.at(2).second) < fBound) bHitBoundary = true;
                        // else if(acos(p.at(3).second) < fBound) bHitBoundary = true; // we can't see the back boundary of block physically
                    }
                }
            }

            // calcultate distance to ceiling and floor
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall); //where the ceiling ends. further the wall, smaller wall we see
            int nFloor = nScreenHeight - nCeiling; // where the floor starts. symmetrical to ceiling

            short nShade; // how this wall should be painted. closer the wall, denser it is shown
            if(bHitBoundary)                               nShade = L'-'; // boundray of wall blocks
            else if(fDistanceToWall <= fDepth / 4.0f)      nShade = L'█'; //█ very close
            else if(fDistanceToWall <= fDepth / 3.0f)      nShade = L'▓'; //▓
            else if(fDistanceToWall <= fDepth / 2.0f)      nShade = L'▒'; //▒■■▧▮▯▩▦▣▤▧▥▬▭▨
            else if(fDistanceToWall <= fDepth)             nShade = L'░'; //░ some fonts may show these symbols as transparent
            else                                           nShade = L' '; // too far away

            // draw the column
            for(int y = 0; y < nScreenHeight; y++)
            {
                if(y < nCeiling) // ceiling
                    screen[y*nScreenWidth + x] = ' ';
                else if(y >= nCeiling && y < nFloor) // wall
                    screen[y*nScreenWidth + x] = nShade;
                else // floor
                {
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if(b < 0.25)        nShade = L'#';
                    else if(b < 0.5)    nShade = L'x';
                    else if(b < 0.75)   nShade = L'_';
                    else if(b < 0.9)    nShade = L'.';
                    else                nShade = L' ';
                    screen[y*nScreenWidth + x] = nShade;
                }
            }

            // draw monsters
            for(int n = 0; n < nNumMonsters; n++)
            {
                if(bMonDeath[n])
                    continue;
                
                // detect visibility
                bool bVisible = false;
                float vx = fPlayerX - fMonsterPositionX[n];
                float vy = fPlayerY - fMonsterPositionY[n];
                float d = sqrt(vx * vx + vy * vy);
                float dot = (fEyeX * vx / d) + (fEyeY * vy / d); // dot product of unit vectors of player ray and monster ray
                if(acos(-dot) < fMonWidth / d) // within the vision angle
                {
                    bVisible = true;
                    for(float s = 0; s < d; s += 0.5)
                    {
                        if(map[(int)(fPlayerY + s * fEyeY) * nMapWidth + (int)(fPlayerX + s * fEyeX)] == '#') // blocked by the wall
                        {
                            bVisible = false;
                            break;
                        }
                    }
                }
                // draw and shoot monster
                if(bVisible)
                {
                    int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / (float)d; //where the ceiling ends. further the wall, smaller wall we see
                    if(nCeiling < 0) nCeiling = 0;
                    int nFloor = nScreenHeight - nCeiling; // where the floor starts. symmetrical to ceiling
                    for(int y = nCeiling; y < nFloor; y++)
                        screen[y*nScreenWidth + x] = L'¥';

                    // first visible, then shootable
                    if(bShooting)
                    {
                        float fEyeX = cosf(fPlayerA); // Unit vector for player
                        float fEyeY = sinf(fPlayerA);
                        float dot = (fEyeX * vx / d) + (fEyeY * vy / d); // dot product of unit vectors of player ray and monster ray
                        if(acos(-dot) < fMonWidth / d) // hit
                        {
                            bMonDeath[n] = true;
                            bShooting = false;
                        }
                    }
                }
            }

            // draw bullet
            // if(bShooting && x > nScreenWidth/2.0f - 3 && x < nScreenWidth/2.0f + 3)
            // {
            //     screen[x + nScreenWidth * (int)(sqrt(9.0f - (x - nScreenWidth/2.0f)*(x - nScreenWidth/2.0f)) + nScreenHeight/2.0f)] = L'';
            // }
        }
        
        // draw bullet
        if(bShooting)
        {
            screen[nScreenWidth/2 + nScreenWidth * (nScreenHeight/2 -1)] = L'|';
            screen[nScreenWidth/2-2 + nScreenWidth * (nScreenHeight/2)] = L'-';
            screen[nScreenWidth/2-1 + nScreenWidth * (nScreenHeight/2)] = L'-';
            screen[nScreenWidth/2 + nScreenWidth * (nScreenHeight/2)] = L'+';
            screen[nScreenWidth/2+1 + nScreenWidth * (nScreenHeight/2)] = L'-';
            screen[nScreenWidth/2+2 + nScreenWidth * (nScreenHeight/2)] = L'-';
            screen[nScreenWidth/2 + nScreenWidth * (nScreenHeight/2 +1)] = L'|';
        }

        // display stats
        swprintf_s(screen, 47, L"HP=%3.1f ,X=%3.2f, Y=%3.2f, A=%3.2f FPS=%5.2f ", fPlayerHP, fPlayerX, fPlayerY, fPlayerA, 1.0f/fElapsedTime);

        // display map
        for(int nx = 0; nx < nMapWidth; nx++)
            for(int ny = 0; ny < nMapHeight; ny++)
            {
                screen[(ny+1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
            }
        for(int n = 0; n < nNumMonsters; n++) // show monsters
        {
            if(bMonDeath[n])
                continue;
            screen[((int)fMonsterPositionY[n] + 1) * nScreenWidth + (int)fMonsterPositionX[n]] = 'M';
        }
        screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P'; // show player

        // display frame
        screen[nScreenWidth*nScreenHeight -1] = '\0';
        WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth*nScreenHeight, {0,0}, &dwBytesWritten);
    }

    // game over
    cout << "Game Over!" << endl;

    return 0;
}

// That's It!! - Jx9
