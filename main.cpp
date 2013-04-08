#include <iostream>
#include <iomanip>

#ifdef __cplusplus
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif
#ifdef __APPLE__
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

#include "SDL_gfxPrimitives.h"
#include "SDL_rotozoom.h"

#include <utility>
#include <map>
#include <queue>
#include <list>
#include <set>

typedef std::map<unsigned int, unsigned int> PaletteIndex;
typedef std::pair<int, int> Point;
typedef std::list<Point> Polygon;

typedef std::pair<Point, Point> Edge;

unsigned char valencePattern[256];

void dumpSurface(SDL_Surface* mySurface);
void dumpTable(unsigned int* table, int width, int height);
void initAlgorithm(void);
void loadValencePattern(void);
unsigned int rgb2yuv(unsigned int rgb);
unsigned int yuv2rgb(unsigned int yuv);
unsigned int* surface2yuv(SDL_Surface* mySurface);
unsigned int* yuv2indexRGB(unsigned int* myYuvSurface, int width, int height, PaletteIndex& paletteRGB);

bool hqxDiff(unsigned int yuv1, unsigned int yuv2);
unsigned int* hqxGetSimilarityGraph(unsigned int* yuvSurface, int width, int height);
void simplifyFullyBlockSimilarityGraph(unsigned int* similarityGraph, int width, int height);
void simplifyCrossesSimilarityGraph(unsigned int* similarityGraph, int width, int height);
void getWeightCurvesFromCrossesInSimilarityGraph(unsigned* weights, unsigned int* similarityGraph, int width, int height);
void getWeightSparsePixelsFromCrossesInSimilarityGraph(unsigned* weights, unsigned int* similarityGraph, int width, int height);
void getWeightIslandsFromCrossesInSimilarityGraph(unsigned* weights, unsigned int* similarityGraph, int width, int height);
void drawSimilarityGraph(SDL_Surface* screen, SDL_Surface* sfOrigin, unsigned int* similarityGraph);

Polygon* extractVoronoiGraph(unsigned int* similarityGraph, int width, int height);
void drawVoronoiGraph(SDL_Surface* screen, SDL_Surface* sfOrigin, Polygon* voronoiGraph);

std::set<Edge> extractSpLines(Polygon* voronoiGraph, SDL_Surface* sfOrigin);
std::set<Edge> extractVisibleEdges(Polygon* voronoiGraph, SDL_Surface* sfOrigin);
void drawVisibleEdges(SDL_Surface* screen, SDL_Surface* sfOrigin, std::set<Edge>& visibleEdges);

std::set<Polygon> extractCurves(Polygon* voronoiGraph, SDL_Surface* sfOrigin);
void drawCurves(SDL_Surface* screen, SDL_Surface* sfOrigin, std::set<Polygon>& curves);

#undef main
int main ( int argc, char** argv )
{
    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
    //SDL_Surface* screen = SDL_SetVideoMode(500, 1024, 16,
    SDL_Surface* screen = SDL_SetVideoMode(1024, 1024, 16,
    //SDL_Surface* screen = SDL_SetVideoMode(320, 240, 16,
                                           SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

    SDL_WM_SetCaption("DePixel", "DePixel");

    // load an image
    SDL_Surface* bmp = SDL_LoadBMP("Image.bmp");
    //SDL_Surface* bmp = SDL_LoadBMP("VampireKiller.bmp");
    if (!bmp)
    {
        printf("Unable to load bitmap: %s\n", SDL_GetError());
        return 1;
    }

    // centre the bitmap on screen
    SDL_Rect dstrect;
    dstrect.x = 10; //(screen->w - bmp->w) / 2;
    dstrect.y = 10; //(screen->h - bmp->h) / 2;

    SDL_Surface *bmpZoom = zoomSurface(bmp, 3.0, 3.0, SMOOTHING_OFF);
    //SDL_Surface *bmpZoom = zoomSurface(bmp, 0.5, 0.5, SMOOTHING_OFF);

    // DRAWING STARTS HERE

    // clear screen
    SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 128, 128, 128));

    initAlgorithm();

    unsigned int* yuvSurface = surface2yuv(bmp);
    unsigned int* similarityGraph = hqxGetSimilarityGraph(yuvSurface, bmp->w, bmp->h);

    simplifyFullyBlockSimilarityGraph(similarityGraph, bmp->w, bmp->h);
    simplifyCrossesSimilarityGraph(similarityGraph, bmp->w, bmp->h);

    Polygon* voronoiGraph = extractVoronoiGraph(similarityGraph, bmp->w, bmp->h);

    //std::set<Edge> spLines = extractSpLines(voronoiGraph, bmp);
    std::set<Polygon> curves = extractCurves(voronoiGraph, bmp);

    std::cout << "Numero curvas extraidas:" << curves.size() << std::endl;

    drawVoronoiGraph(screen, bmp, voronoiGraph);
    //drawVisibleEdges(screen, bmp, spLines);
    //drawSimilarityGraph(screen, bmp, similarityGraph);
    drawCurves(screen, bmp, curves);

    delete [] voronoiGraph;
    delete [] similarityGraph;
    delete [] yuvSurface;


    // draw bitmap
    SDL_BlitSurface(bmpZoom, 0, screen, &dstrect);

    // DRAWING ENDS HERE

    // finally, update the screen :)
    SDL_Flip(screen);

    // program main loop
    bool done = false;
    while (!done)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // check for messages
            switch (event.type)
            {
                // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;

                // check for keypresses
            case SDL_KEYDOWN:
                {
                    // exit if ESCAPE is pressed
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        done = true;
                    break;
                }
            } // end switch
        } // end of message processing


    } // end main loop

    // free loaded bitmap
    SDL_FreeSurface(bmp);

    // all is well ;)
    printf("Exited cleanly\n");
    return 0;
}

void dumpSurface(SDL_Surface* mySurface)
{
    int width = mySurface->w;
    int height = mySurface->h;
    unsigned int* img = (unsigned int*)(mySurface->pixels);
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            std::cout << std::hex << *img++ << "; ";
        }
        std::cout << std::endl;
    }
}

void dumpTable(unsigned int* table, int width, int height)
{
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            std::cout << std::hex << *table++ << "; ";
        }
        std::cout << std::endl;
    }
}

void initAlgorithm(void)
{
    loadValencePattern();
}

void loadValencePattern(void)
{
    unsigned int aux;
    unsigned int valence;
    unsigned int bit;
    for(unsigned int pattern = 0; pattern < 256; ++pattern)
    {
        aux = pattern;
        valence = 0;
        for(bit = 0; bit < 8; ++bit)
        {
            if(aux & 1) ++valence;
            aux >>= 1;
        }

        valencePattern[pattern] = valence;
    }
}

unsigned int rgb2yuv(unsigned int rgb)
{
    unsigned int y, u, v, r, g, b;

    r = ((rgb & 0xFF0000) >> 16);
    g = ((rgb & 0xFF00) >> 8);
    b = rgb & 0xFF;

    y = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
    u = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;
    v = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;

    return (y << 16) | (u << 8) | v;
}

unsigned int yuv2rgb(unsigned int yuv)
{
    unsigned int y, u, v, r, g, b;

    y = ((yuv & 0xFF0000) >> 16);
    u = ((yuv & 0xFF00) >> 8);
    v = yuv & 0xFF;

    r = 1.164 * (y - 16) + 1.596 * (v - 128);
    g = 1.164 * (y - 16) - 0.813 * (v - 128) - 0.391 * (u - 128);
    b = 1.164 * (y - 16) + 2.018 * (u - 128);

    return (r << 16) | (g << 8) | b;
}


unsigned int* surface2yuv(SDL_Surface* mySurface)
{
    int width = mySurface->w;
    int height = mySurface->h;
    unsigned int* img = (unsigned int*)(mySurface->pixels);
    unsigned int* buffer = new unsigned int[width * height];
    unsigned int index = 0;

    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            buffer[index++] = rgb2yuv(*img++);
            //std::cout << "Convirtiendo rgb a yuv:" << x << ", " << y << "," << width << "," << height << std::endl;
        }
    }

    return buffer;
}

unsigned int* yuv2indexRGB(unsigned int* myYuvSurface, int width, int height, PaletteIndex& paletteRGB)
{
    unsigned int* buffer = new unsigned int[width * height];

    unsigned int index = 0;

    unsigned int yuvColorSurface;
    unsigned int yuvColorPalette;
    unsigned int ys, us, vs, yp, up, vp;
    unsigned int colorIndex;
    unsigned int closest;
    unsigned int prox;



    PaletteIndex paletteYUV;

    for(PaletteIndex::iterator it = paletteRGB.begin(); it != paletteRGB.end(); ++it)
    {
        paletteYUV[rgb2yuv(it->first)] = it->second;
    }

    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            yuvColorSurface = *myYuvSurface++;
            ys = yuvColorSurface >> 16;
            us = (yuvColorSurface >> 8) & 0xFF;
            vs = yuvColorSurface & 0xFF;

            closest = 0xFFFFFFFF;

            for(PaletteIndex::iterator it =  paletteYUV.begin(); it != paletteYUV.end(); ++it)
            {
                yuvColorPalette = it->first;

                yp = yuvColorPalette >> 16;
                up = (yuvColorPalette >> 8) & 0xFF;
                vp = yuvColorPalette & 0xFF;

                prox = ((yp - ys) * (yp - ys)) + ((up - us) * (up - us)) + ((vp - vs) * (vp - vs));

                if(prox < closest)
                {
                    closest = prox;
                    colorIndex = it->second;
                }
            }

            //std::cout << std::setw(2) << std::setfill('0') << colorIndex << "; ";
            buffer[index++] = colorIndex;
        }
    }

    std::cout << "================================================================" << std::endl;
    std::cout << "Index Table" << std::endl;
    std::cout << "================================================================" << std::endl;
    dumpTable(buffer, width, height);
    std::cout << "================================================================" << std::endl;

    return buffer;
}

//  Parte relativa al algoritmo HQX. Me baso principalmente en lo que hay aquí: http://code.google.com/p/hqx/
bool hqxDiff(unsigned int yuv1, unsigned int yuv2)
{
    return !((abs((yuv1 & 0xFF0000) - (yuv2 & 0xFF0000)) > 0x00300000) ||
             (abs((yuv1 & 0xFF00) - (yuv2 & 0xFF00)) > 0x00000700) ||
             (abs((yuv1 & 0xFF) - (yuv2 & 0xFF)) > 0x00000006));
}

unsigned int* hqxGetSimilarityGraph(unsigned int* yuvSurface, int width, int height)
{
    int  x, y, k;
    int  prevline, nextline;
    uint32_t  w[10];

    unsigned int* similarityGraph = new unsigned int[width * height];
    int index = 0;
    int pattern = 0;
    int flag = 1;

    //   +----+----+----+
    //   |    |    |    |
    //   | w1 | w2 | w3 |
    //   +----+----+----+
    //   |    |    |    |
    //   | w4 | w5 | w6 |
    //   +----+----+----+
    //   |    |    |    |
    //   | w7 | w8 | w9 |
    //   +----+----+----+

    for (y = 0; y < height; ++y)
    {
        if(y > 0)
        {
            prevline = -width;
        }
        else
        {
            prevline = 0;
        }

        if(y < height - 1)
        {
            nextline = width;
        }
        else
        {
            nextline = 0;
        }

        for(x = 0; x < width; ++x)
        {
            w[2] = *(yuvSurface + prevline);
            w[5] = *yuvSurface;
            w[8] = *(yuvSurface + nextline);

            if(x > 0)
            {
                w[1] = *(yuvSurface + prevline - 1);
                w[4] = *(yuvSurface - 1);
                w[7] = *(yuvSurface + nextline - 1);
            }
            else
            {
                w[1] = w[2];
                w[4] = w[5];
                w[7] = w[8];
            }

            if(x < width - 1)
            {
                w[3] = *(yuvSurface + prevline + 1);
                w[6] = *(yuvSurface + 1);
                w[9] = *(yuvSurface + nextline + 1);
            }
            else
            {
                w[3] = w[2];
                w[6] = w[5];
                w[9] = w[8];
            }

            pattern = 0;
            flag = 1;

            //yuv1 = rgb_to_yuv(w[5]);

            for(k = 1; k <= 9; ++k)
            {
                if(k == 5) continue;

                if(w[k] != w[5])
                {
                    //yuv2 = rgb_to_yuv(w[k]);
                    if(hqxDiff(w[5], w[k])) pattern |= flag;
                }
                else
                {
                    pattern |= flag;

                }
                flag <<= 1;
            }

            similarityGraph[index++] = pattern;
            ++yuvSurface;
        }
    }

    for(int x = 0; x < width; ++x)
    {
        similarityGraph[x]                            &= 0xF8; // &B11111000
        similarityGraph[x + (height - 1) * width]     &= 0x1F; // &B00011111
    }

    for(int y = 0; y < height; ++y)
    {
        similarityGraph[y * width]                    &= 0xD6; // &B11010110
        similarityGraph[width - 1 + y * width]        &= 0x6B; // &B01101011
    }

    //std::cout << "================================================================" << std::endl;
    //std::cout << "Similarity Graph" << std::endl;
    //std::cout << "================================================================" << std::endl;
    //dumpTable(similarityGraph, width, height);
    //std::cout << "================================================================" << std::endl;

    return similarityGraph;
}

//  Simplificación del Similarity Graph obtenido por medio del algoritmo HQX
void simplifyFullyBlockSimilarityGraph(unsigned int* similarityGraph, int width, int height)
{
    const unsigned int cornerUpLeft    = 0xD0; // &B11010000
    const unsigned int cornerUpRight   = 0x68; // &B01101000
    const unsigned int cornerDownLeft  = 0x16; // &B00010110
    const unsigned int cornerDownRight = 0x0B; // &B00001011

    unsigned int w[4];

    for(int y = 0; y < height - 1; ++y)
    {
        for(int x = 0; x < width - 1; ++x)
        {
            w[0] = similarityGraph[x + y * width];
            w[1] = similarityGraph[(x + 1) + y * width];
            w[2] = similarityGraph[x + (y + 1) * width];
            w[3] = similarityGraph[(x + 1) + (y + 1) * width];

            if(((w[0] & cornerUpLeft) == cornerUpLeft) &&
               ((w[1] & cornerUpRight) == cornerUpRight) &&
               ((w[2] & cornerDownLeft) == cornerDownLeft) &&
               ((w[3] & cornerDownRight) == cornerDownRight))
            {
                similarityGraph[x + y * width] &=             0x7F; // &B01111111
                similarityGraph[(x + 1) + y * width] &=       0xDF; // &B11011111
                similarityGraph[x + (y + 1) * width] &=       0xFB; // &B11111011
                similarityGraph[(x + 1) + (y + 1) * width] &= 0xFE; // &B11111110
            }
        }
    }
}

void simplifyCrossesSimilarityGraph(unsigned int* similarityGraph, int width, int height)
{
    unsigned int* weights = new unsigned int[(width - 1) * (height - 1) * 2];

    memset(weights, 0, sizeof(weights));

    unsigned int* similarityGraphWithoutBorders = new unsigned int[width * height];

    memcpy(similarityGraphWithoutBorders, similarityGraph, sizeof(unsigned int[width * height]));

    for(int x = 0; x < width; ++x)
    {
        similarityGraphWithoutBorders[x]                            &= 0xF8; // &B11111000
        similarityGraphWithoutBorders[x + (height - 1) * width]     &= 0x1F; // &B00011111
    }

    for(int y = 0; y < height; ++y)
    {
        similarityGraphWithoutBorders[y * width]                    &= 0xD6; // &B11010110
        similarityGraphWithoutBorders[width - 1 + y * width]        &= 0x6B; // &B01101011
    }

    //similarityGraphWithoutBorders[0]                                &= 0xFE; // &B11111110
    //similarityGraphWithoutBorders[width - 1]                        &= 0xFB; // &B11111011
    //similarityGraphWithoutBorders[(height - 1) * width]             &= 0xDF; // &B11011111
    //similarityGraphWithoutBorders[width - 1 + (height - 1) * width] &= 0x7F; // &B01111111

    std::cout << "================================================================" << std::endl;
    std::cout << "Similarity Graph++" << std::endl;
    std::cout << "================================================================" << std::endl;
    dumpTable(similarityGraphWithoutBorders, width, height);
    std::cout << "================================================================" << std::endl;

    getWeightCurvesFromCrossesInSimilarityGraph(weights, similarityGraphWithoutBorders, width, height);
    getWeightSparsePixelsFromCrossesInSimilarityGraph(weights, similarityGraphWithoutBorders, width, height);
    getWeightIslandsFromCrossesInSimilarityGraph(weights, similarityGraphWithoutBorders, width, height);

    delete [] similarityGraphWithoutBorders;

    const unsigned int cornerUpLeft    = 0x80; // &B10000000
    const unsigned int cornerUpRight   = 0x20; // &B00100000
    const unsigned int cornerDownLeft  = 0x04; // &B00000100
    const unsigned int cornerDownRight = 0x01; // &B00000001

    unsigned *sg = similarityGraph;

    for(int y = 0; y < height - 1; ++y)
    {
        for(int x = 0; x < width - 1; ++x)
        {
            if((sg[x + y * width] & cornerUpLeft) &&
               (sg[(x + 1) + y * width] & cornerUpRight) &&
               (sg[x + (y + 1) * width] & cornerDownLeft) &&
               (sg[(x + 1) + (y + 1) * width] & cornerDownRight))
            {
                std::cout << "Cruce en (" << x << ", " << y << "). Peso Aspa 1: " << weights[(x + y * width) * 2] << "; Peso Aspa 2: " << weights[((x + y * width) * 2) + 1] << std::endl;

                if(weights[(x + y * width) * 2] < weights[((x + y * width) * 2) + 1])
                {
                    sg[x + y * width] &= 0x7F;
                    sg[(x + 1) + (y + 1) * width] &= 0xFE;
                    //if(x == 5 && y == 0)
                    std::cout << "Removiendo aspa 1" << std::endl;
                }
                else if(weights[(x + y * width) * 2] > weights[((x + y * width) * 2) + 1])
                {
                    sg[(x + 1) + y * width] &= 0xDF;
                    sg[x + (y + 1) * width] &= 0xFB;
                    //if(x == 5 && y == 0)
                    std::cout << "Removiendo aspa 2" << std::endl;
                }
                else
                {
                    sg[x + y * width] &= 0x7F;
                    sg[(x + 1) + (y + 1) * width] &= 0xFE;
                    sg[(x + 1) + y * width] &= 0xDF;
                    sg[x + (y + 1) * width] &= 0xFB;
                }
            }
        }
    }


    delete [] weights;
}

void getWeightCurvesFromCrossesInSimilarityGraph(unsigned int* weights, unsigned int* similarityGraph, int width, int height)
{
    const unsigned int cornerUpLeft    = 0x80; // &B10000000
    const unsigned int cornerUpRight   = 0x20; // &B00100000
    const unsigned int cornerDownLeft  = 0x04; // &B00000100
    const unsigned int cornerDownRight = 0x01; // &B00000001

    unsigned int* sg = similarityGraph;

    unsigned int w[4];
    unsigned int origins[4];

    origins[0] = 7;
    origins[1] = 5;
    origins[2] = 2;
    origins[3] = 0;

    int xx, yy, fromDirection = 0; // 0, 1, 2
                                   // 3, x, 4
                                   // 5, 6, 7

    unsigned int numNodes;
    unsigned int pattern;


    for(int y = 0; y < height - 1; ++y)
    {
        for(int x = 0; x < width - 1; ++x)
        {
            w[0] = sg[x + y * width];
            w[1] = sg[(x + 1) + y * width];
            w[2] = sg[x + (y + 1) * width];
            w[3] = sg[(x + 1) + (y + 1) * width];

            if((sg[x + y * width] & cornerUpLeft) &&
               (sg[(x + 1) + y * width] & cornerUpRight) &&
               (sg[x + (y + 1) * width] & cornerDownLeft) &&
               (sg[(x + 1) + (y + 1) * width] & cornerDownRight))
            {
                for(unsigned int i = 0; i < 4; ++i)
                {
                    numNodes = 1;

                    if(x == 2 && y == 9 && i == 0)
                    {
                        std::cout << "Encontrado nodo de valencia 2 " << (int)(valencePattern[w[i]]) << std::endl;
                    }

                    if(valencePattern[w[i]] == 2)
                    {
                        switch(i)
                        {
                            case 0:
                                xx = x;
                                yy = y;
                                break;
                            case 1:
                                xx = x + 1;
                                yy = y;
                                break;
                            case 2:
                                xx = x;
                                yy = y + 1;
                                break;
                            case 3:
                                xx = x + 1;
                                yy = y + 1;
                        }

                        fromDirection = origins[i];

                        std::cout << "Encontrado nodo de valencia 2 en " << xx << ", " << yy << ", " << i << "; Desde: " << std::hex << fromDirection << "; Conexiones:" << std::hex << sg[xx + yy * width] << std::endl;

                        pattern = 0;

                        do
                        {
                            //std::cout << "Comprobando...";
                            pattern = sg[xx + yy * width];

                            //std::cout << "[" << std::hex << pattern << "]";



                            for(int d = 0; d < 8; ++d)
                            {
                                if((pattern & 1) && (d != fromDirection))
                                {
                                    fromDirection = 7 - d;

                                    switch(fromDirection)
                                    {
                                        case 0:
                                            xx += 1;
                                            yy += 1;
                                            break;
                                        case 1:
                                            yy += 1;
                                            break;
                                        case 2:
                                            xx -= 1;
                                            yy += 1;
                                            break;
                                        case 3:
                                            xx += 1;
                                            break;
                                        case 4:
                                            xx -= 1;
                                            break;
                                        case 5:
                                            xx += 1;
                                            yy -= 1;
                                            break;
                                        case 6:
                                            yy -= 1;
                                            break;
                                        case 7:
                                            xx -= 1;
                                            yy -= 1;
                                            break;
                                    }

                                    ++numNodes;
                                    if(xx >= 0 && xx < width && yy >= 0 && yy < height)

                                        std::cout << "--> " << xx << ", " << yy  << " : Nodos:" << numNodes << "; Desde: " << std::hex << fromDirection << "; Conexiones:" << std::hex << sg[xx + yy * width] << "; Valencia:" << (int)(valencePattern[sg[xx + yy * width]]) << "||" << (valencePattern[sg[xx + yy * width]] == 2 && xx >= 0 && xx < width && yy >= 0 && yy < height) << std::endl;
                                    break;
                                }
                                else
                                {
                                    pattern >>= 1;
                                }
                            }
                        } while(valencePattern[sg[xx + yy * width]] == 2);
                        std::cout << "Fin del camino" << std::endl;
                    }
                    w[i] = numNodes;
                }

                if(x == 5 && y == 0)
                {
                    std::cout << "w[0] = " << w[0] << "; "
                            << "w[1] = " << w[1] << "; "
                            << "w[2] = " << w[2] << "; "
                            << "w[3] = " << w[3] << "; " << std::endl;

                }

                if(w[0] + w[3] >= w[1] + w[2])
                {
                    weights[(x + (y * width)) * 2] += (w[0] + w[3] - w[1] - w[2]);
                }
                else
                {
                    weights[((x + (y * width)) * 2) + 1] += (w[1] + w[2] - w[0] - w[3]);
                }
            }
        }
    }
}

void getWeightSparsePixelsFromCrossesInSimilarityGraph(unsigned int* weights, unsigned int* similarityGraph, int width, int height)
{
    const unsigned int cornerUpLeft    = 0x80; // &B10000000
    const unsigned int cornerUpRight   = 0x20; // &B00100000
    const unsigned int cornerDownLeft  = 0x04; // &B00000100
    const unsigned int cornerDownRight = 0x01; // &B00000001

    unsigned int* sg = similarityGraph;

    int x, y, xx, yy, x2, y2, xd, yd; // 0, 1, 2
                                      // 3, x, 4
                                      // 5, 6, 7

    unsigned int n, d;

    unsigned int conns;

    unsigned int* window = new unsigned int[64];

    unsigned int sizeCompDiagTopLeftToBottomRight;
    unsigned int sizeCompDiagTopRightToBottomLeft;

    std::queue<std::pair<unsigned int, unsigned int> > q;

    for(y = 0; y < height - 1; ++y)
    {
        for(x = 0; x < width - 1; ++x)
        {
            if((sg[x + y * width] & cornerUpLeft) &&
               (sg[(x + 1) + y * width] & cornerUpRight) &&
               (sg[x + (y + 1) * width] & cornerDownLeft) &&
               (sg[(x + 1) + (y + 1) * width] & cornerDownRight))
            {
                // Limpiamos la ventana.
                memset(window, 0, sizeof(unsigned int[64]));

                for(n = 1; n <= 2; ++n)
                {
                    xx = n == 1 ? x : x + 1;
                    yy = y;
                    xd = n == 1 ? 3 : 4;
                    yd = 3;

                    window[xd + yd * 8] = n;

                    q.push(std::make_pair(xx, yy));
if(x == 1 && y == 0)
                    std::cout << "Comprobando tamaño de componentes en (" << xx << ", " << yy << "). " << std::endl;

                    while(!(q.empty()))
                    {
                        xx = q.front().first;
                        yy = q.front().second;
                        q.pop();

                        conns = sg[xx + yy * width];

                        if(x == 1 && y == 0)
                            std::cout << "Rastreando nodo (" << xx << ", " << yy << "). Conns:" << std::hex << conns << ". Tamaño cola:" << q.size() << std::endl;

                        for(d = 0; d < 8; ++d)
                        {
                            if(conns & 1)
                            {
                                switch(d)
                                {
                                    case 0:

                                        x2 = xx - 1;
                                        y2 = yy - 1;

                                        break;
                                    case 1:
                                        x2 = xx;
                                        y2 = yy - 1;
                                        break;
                                    case 2:
                                        x2 = xx + 1;
                                        y2 = yy - 1;
                                        break;
                                    case 3:
                                        x2 = xx - 1;
                                        y2 = yy;
                                        break;
                                    case 4:
                                        x2 = xx + 1;
                                        y2 = yy;
                                        break;
                                    case 5:
                                        x2 = xx - 1;
                                        y2 = yy + 1;
                                        break;
                                    case 6:
                                        x2 = xx;
                                        y2 = yy + 1;
                                        break;
                                    case 7:
                                        x2 = xx + 1;
                                        y2 = yy + 1;
                                        break;
                                }

                                xd = x2 - x + 3;
                                yd = y2 - y + 3;

                                if(x == 1 && y == 0)
                                    std::cout << std::dec << "Probando (" << x2 << ", " << y2 << ") en (" << xd << ", " << yd << "). ;";

                                if(xd >= 0 && xd < 8 && yd >=0 && yd < 8)
                                {
                                    if(x == 1 && y == 0)
                                        std::cout << " W:" << window[xd + yd * 8] << std::endl;
                                    if(window[xd + yd * 8] == 0)
                                    {
                                        window[xd + yd * 8] = n;
                                        q.push(std::make_pair(x2, y2));
                                        if(x == 1 && y == 0)
                                        std::cout << "Moviendo a (" << x2 << ", " << y2 << "). Cola:" << q.size() << std::endl;
                                        //int q;
                                        //std::cin >> q;
                                    }
                                }
                                else
                                {
                                    if(x == 1 && y == 0)
                                    std::cout << std::endl;

                                }
                            }
                            conns >>= 1;
                        }
                    }
                }

                sizeCompDiagTopLeftToBottomRight = 0;
                sizeCompDiagTopRightToBottomLeft = 0;

                for(n = 0; n < 64; ++n)
                {
                    if(x == 1 && y == 0)
                        std::cout << "W[" << n << "] = " << window[n] << std::endl;
                    if(window[n] == 1)
                    {
                        ++sizeCompDiagTopLeftToBottomRight;
                    }
                    else if(window[n] == 2)
                    {
                        ++sizeCompDiagTopRightToBottomLeft;
                    }
                }

                if(sizeCompDiagTopLeftToBottomRight >= sizeCompDiagTopRightToBottomLeft)
                {
                    weights[((x + (y * width)) * 2) + 1] += (sizeCompDiagTopLeftToBottomRight - sizeCompDiagTopRightToBottomLeft);
                }
                else
                {
                    weights[(x + (y * width)) * 2] += (sizeCompDiagTopRightToBottomLeft - sizeCompDiagTopLeftToBottomRight);
                }

            }
        }
    }
}

void getWeightIslandsFromCrossesInSimilarityGraph(unsigned int* weights, unsigned int* similarityGraph, int width, int height)
{
    const unsigned int cornerUpLeft    = 0x80; // &B10000000
    const unsigned int cornerUpRight   = 0x20; // &B00100000
    const unsigned int cornerDownLeft  = 0x04; // &B00000100
    const unsigned int cornerDownRight = 0x01; // &B00000001

    unsigned int* sg = similarityGraph;

    int x, y;

    for(y = 0; y < height - 1; ++y)
    {
        for(x = 0; x < width - 1; ++x)
        {
            if((x == 1 || x == 2) && y == 8)
            {
             std::cout << "(" << x << "," << y << ") --> " << sg[x + y * width] << ", " << sg[(x + 1) + y * width] << ", " << sg[x + (y + 1) * width] << ", " << sg[(x + 1) + (y + 1) * width] << std::endl;
             std::cout << (sg[x + y * width] & cornerUpLeft) << ", " << (sg[(x + 1) + y * width] & cornerUpRight) << ", " << (sg[x + (y + 1) * width] & cornerDownLeft) << ", " << (sg[(x + 1) + (y + 1) * width] & cornerDownRight)<< std::endl;
            }

            if((sg[x + y * width] & cornerUpLeft) &&
               (sg[(x + 1) + y * width] & cornerUpRight) &&
               (sg[x + (y + 1) * width] & cornerDownLeft) &&
               (sg[(x + 1) + (y + 1) * width] & cornerDownRight))
            {
                std::cout << "Comprobando islas en (" << x << ", " << y << ")" << std::endl;
                if(valencePattern[sg[x + y * width]] == 1 || valencePattern[sg[(x + 1) + (y + 1) * width]] == 1)
                {
                    std::cout << "Aumentando peso en aspa 1" << std::endl;
                    weights[(x + (y * width)) * 2] += 5;
                }

                if(valencePattern[sg[(x + 1) + y * width]] == 1 || valencePattern[sg[x + (y + 1) * width]] == 1)
                {
                    std::cout << "Aumentando peso en aspa 2" << std::endl;
                    weights[((x + (y * width)) * 2) + 1] += 5;
                }
            }
        }
    }
}

void drawSimilarityGraph(SDL_Surface* screen, SDL_Surface* sfOrigin, unsigned int* similarityGraph)
{
    int widthPixelArt = sfOrigin->w;
    int heightPixelArt = sfOrigin->h;

    int widthGraph = (widthPixelArt * 28) - 14 - 7;
    int heightGraph = (heightPixelArt * 28) - 14 - 7;

    int offsWidthScreen = (screen->w - widthGraph) >> 1;
    int offsHeightScreen = (screen->h - heightGraph) >> 1;

    //int filledEllipseRGBA(SDL_Surface* dst,
    //                  Sint16 x, Sint16 y,
    //                  Sint16 rx, Sint16 ry,
    //                  Uint8 r, Uint8 g, Uint8 b, Uint8 a);

    int x, y;
    unsigned int* img = (unsigned int*)(sfOrigin->pixels);
    unsigned int  rgb = 0xFF0000FF;
    unsigned int  r = (rgb & 0xFF0000) >> 16;
    unsigned int  g = (rgb & 0xFF00) >> 8;
    unsigned int  b = (rgb & 0xFF);
    unsigned int  a = (rgb & 0xFF000000) >> 24;

    unsigned int pattern;

    const Uint8 widthLine = 5;

    for(y = 0; y < heightPixelArt; ++y)
    {
        std::cout << std::endl;
        for(x = 0; x < widthPixelArt; ++x)
        {
            pattern = *similarityGraph++;
            std::cout << pattern << ", ";



            //if(x == 1 && y == 1) pattern = 255; else pattern = 0;

            if(y > 0)
            {
                if(x > 0 && (pattern & 0x1))
                {
                    thickLineRGBA(screen, (x * 28) + offsWidthScreen - 14, (y * 28) + offsHeightScreen - 14,
                                          (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                                          widthLine, r, g, b, a);
                    //lineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                    //                 (x * 28) + offsWidthScreen - 14, (y * 28) + offsHeightScreen - 14,
                    //                 r, g, b, a);
                }

                if(pattern & 0x2)
                {
                    thickLineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                                     (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen - 14,
                                     widthLine, r, g, b, a);
                    //lineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                    //                 (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen - 14,
                    //                 r, g, b, a);
                }

                if(x < (widthPixelArt - 1) && (pattern & 0x4))
                {
                    thickLineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                                     (x * 28) + offsWidthScreen + 14, (y * 28) + offsHeightScreen - 14,
                                     widthLine, r, g, b, a);
                    //lineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                    //                 (x * 28) + offsWidthScreen + 14, (y * 28) + offsHeightScreen - 14,
                    //                 r, g, b, a);
                }
            }

            if(x > 0 && (pattern & 0x8))
            {
                thickLineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                                 (x * 28) + offsWidthScreen - 14, (y * 28) + offsHeightScreen,
                                 widthLine, r, g, b, a);
                //lineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                //                 (x * 28) + offsWidthScreen - 14, (y * 28) + offsHeightScreen,
                //                 r, g, b, a);
            }

            if(x < (widthPixelArt - 1) && (pattern & 0x10))
            {
                thickLineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                                 (x * 28) + offsWidthScreen + 14, (y * 28) + offsHeightScreen,
                                 widthLine, r, g, b, a);
                //lineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                //                 (x * 28) + offsWidthScreen + 14, (y * 28) + offsHeightScreen,
                //                 r, g, b, a);
            }

            if(y < (heightPixelArt - 1))
            {
                if(x > 0 && (pattern & 0x20))
                {
                    thickLineRGBA(screen, (x * 28) + offsWidthScreen - 14, (y * 28) + offsHeightScreen + 14,
                                          (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                                          widthLine, r, g, b, a);
                    //lineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                    //                 (x * 28) + offsWidthScreen - 14, (y * 28) + offsHeightScreen + 14,
                    //                 r, g, b, a);
                }

                if(pattern & 0x40)
                {
                    thickLineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                                     (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen + 14,
                                     widthLine, r, g, b, a);
                    //lineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                    //                 (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen + 14,
                    //                 r, g, b, a);
                }

                if(x < (widthPixelArt - 1) && (pattern & 0x80))
                {
                    thickLineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                                     (x * 28) + offsWidthScreen + 14, (y * 28) + offsHeightScreen + 14,
                                     widthLine, r, g, b, a);
                    //lineRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen,
                    //                 (x * 28) + offsWidthScreen + 14, (y * 28) + offsHeightScreen + 14,
                    //                 r, g, b, a);
                }
            }
        }
    }

    for(y = 0; y < heightPixelArt; ++y)
    {
        for(x = 0; x < widthPixelArt; ++x)
        {
            rgb = *img++;
            r = (rgb & 0xFF0000) >> 16;
            g = (rgb & 0xFF00) >> 8;
            b = (rgb & 0xFF);
            a = (rgb & 0xFF000000) >> 24;
            filledEllipseRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen, 9, 9, r, g, b, a);
            ellipseRGBA(screen, (x * 28) + offsWidthScreen, (y * 28) + offsHeightScreen, 9, 9, 0, 0, 0, a);
        }
    }
}

Polygon* extractVoronoiGraph(unsigned int* similarityGraph, int width, int height)
{
    Polygon* voronoiGraph = new Polygon[width * height];
    int x, y, prevline, nextline;
    unsigned int* sg = similarityGraph;
    unsigned int w[10];

    Point voronoiPoint, pointAux;
    //unsigned int conns;

    Polygon voronoiCell;

    for(y = 0; y < height; ++y)
    {
        if(y > 0)
        {
            prevline = -width;
        }
        else
        {
            prevline = 0;
        }

        if(y < height - 1)
        {
            nextline = width;
        }
        else
        {
            nextline = 0;
        }

        for(x = 0; x < width; ++x)
        {
            w[2] = *(sg + prevline);
            w[5] = *sg;
            w[8] = *(sg + nextline);

            if(x > 0)
            {
                w[1] = *(sg + prevline - 1);
                w[4] = *(sg - 1);
                w[7] = *(sg + nextline - 1);
            }
            else
            {
                w[1] = w[2];
                w[4] = w[5];
                w[7] = w[8];
            }

            if(x < width - 1)
            {
                w[3] = *(sg + prevline + 1);
                w[6] = *(sg + 1);
                w[9] = *(sg + nextline + 1);
            }
            else
            {
                w[3] = w[2];
                w[6] = w[5];
                w[9] = w[8];
            }

            voronoiCell.clear();

            //  Según organización de las conexiones, hay que construir la celula...
            // Voy a ponerlas todas como puntos de entre (0,0) y (8, 8). Posteriormente optimizaré esto.
            if(y > 0)
            {
                //  Si arriba hay diagonal abajo a la izquierda
                if(w[2] & 0x20)
                {
                    voronoiPoint = std::make_pair(1, 1);
                    voronoiCell.push_back(voronoiPoint);

                }
                else if(w[5] & 0x01) // Si no existiera esa conexión, comprobemos si la central tiene una arriba-izquierda
                {
                    voronoiPoint = std::make_pair(1, -1);
                    voronoiCell.push_back(voronoiPoint);
                }
                else
                {
                    voronoiPoint = std::make_pair(0, 0);
                    voronoiCell.push_back(voronoiPoint);
                }

                //  Si arriba hay diagonal abajo a la derecha
                if(w[2] & 0x80)
                {
                    voronoiPoint = std::make_pair(3, 1);
                    voronoiCell.push_back(voronoiPoint);

                }
                else if(w[5] & 0x04) // Si no existiera esa conexión, comprobemos si la central tiene una arriba-derecha
                {
                    voronoiPoint = std::make_pair(3, -1);
                    voronoiCell.push_back(voronoiPoint);
                }
                else
                {
                    voronoiPoint = std::make_pair(4, 0);
                    voronoiCell.push_back(voronoiPoint);
                }
            }
            else
            {
                voronoiCell.push_back(std::make_pair(0, 0));

                voronoiPoint = std::make_pair(4, 0);
                voronoiCell.push_back(voronoiPoint);
            }

            if(x < width - 1)
            {
                //  Si a la derecha hay diagonal arriba-izquierda
                if(w[6] & 0x01)
                {
                    pointAux = std::make_pair(3, 1);
                    if(voronoiPoint != pointAux)
                    {
                        voronoiPoint = pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
                else if(w[5] & 0x04) // Si no existiera esa conexión, comprobemos si la central tiene una arriba-derecha
                {
                    //pointAux = std::make_pair(3, 1);
                    //if(voronoiPoint != pointAux)
                    //{
                        voronoiPoint = std::make_pair(5, 1); //pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    //}
                }
                else
                {
                    pointAux = std::make_pair(4, 0);
                    if(voronoiPoint != pointAux)
                    {
                        voronoiPoint = pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }

                //  Si a la derecha hay diagonal abajo-izquierda
                if(w[6] & 0x20)
                {
                    //if(voronoiPoint != std::make_pair(3, 1))
                    {
                        voronoiPoint = std::make_pair(3, 3);
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
                else if(w[5] & 0x80) // Si no existiera esa conexión, comprobemos si la central tiene una abajo-derecha
                {
                    //pointAux = std::make_pair(3, 1);
                    //if(voronoiPoint != pointAux)
                    //{
                        voronoiPoint = std::make_pair(5, 3); //pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    //}
                }
                else
                {
                    //if(voronoiPoint != std::make_pair(3, 0))
                    {
                        voronoiPoint = std::make_pair(4, 4);
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
            }
            else
            {
                pointAux = std::make_pair(4, 0);
                if(voronoiPoint != pointAux)
                {
                    voronoiPoint = pointAux;
                    voronoiCell.push_back(voronoiPoint);
                }

                pointAux = std::make_pair(4, 4);
                if(voronoiPoint != pointAux)
                {
                    voronoiPoint = pointAux;
                    voronoiCell.push_back(voronoiPoint);
                }
            }

            if(y < height - 1)
            {
                //  Si abajo hay diagonal arriba-derecha
                if(w[8] & 0x04)
                {
                    pointAux = std::make_pair(3, 3);
                    if(voronoiPoint != pointAux)
                    {
                        voronoiPoint = pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
                else if(w[5] & 0x80) // Si no existiera esa conexión, comprobemos si la central tiene una abajo-derecha
                {
                    //pointAux = std::make_pair(3, 1);
                    //if(voronoiPoint != pointAux)
                    //{
                        voronoiPoint = std::make_pair(3, 5); //pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    //}
                }
                else
                {
                    pointAux = std::make_pair(4, 4);
                    if(voronoiPoint != pointAux)
                    {
                        voronoiPoint = pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }

                //  Si abajo hay diagonal arriba-izquierda
                if(w[8] & 0x01)
                {
                    //if(voronoiPoint != std::make_pair(3, 1))
                    {
                        voronoiPoint = std::make_pair(1, 3);
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
                else if(w[5] & 0x20) // Si no existiera esa conexión, comprobemos si la central tiene una abajo-izquierda
                {
                    //pointAux = std::make_pair(3, 1);
                    //if(voronoiPoint != pointAux)
                    //{
                        voronoiPoint = std::make_pair(1, 5); //pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    //}
                }
                else
                {
                    //if(voronoiPoint != std::make_pair(3, 0))
                    {
                        voronoiPoint = std::make_pair(0, 4);
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
            }
            else
            {
                pointAux = std::make_pair(4, 4);
                if(voronoiPoint != pointAux)
                {
                    voronoiPoint = pointAux;
                    voronoiCell.push_back(voronoiPoint);
                }

                pointAux = std::make_pair(0, 4);
                if(voronoiPoint != pointAux)
                {
                    voronoiPoint = pointAux;
                    voronoiCell.push_back(voronoiPoint);
                }
            }

            if(x > 0)
            {
                //  Si a la izquierda hay diagonal abajo-derecha
                if(w[4] & 0x80)
                {
                    pointAux = std::make_pair(1, 3);
                    if(voronoiPoint != pointAux)
                    {
                        voronoiPoint = pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
                else if(w[5] & 0x20) // Si no existiera esa conexión, comprobemos si la central tiene una abajo-izquierda
                {
                    //pointAux = std::make_pair(3, 1);
                    //if(voronoiPoint != pointAux)
                    {
                        voronoiPoint = std::make_pair(-1, 3); //pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
                else
                {
                    pointAux = std::make_pair(0, 4);
                    if(voronoiPoint != pointAux)
                    {
                        voronoiPoint = pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }

                //  Si a la izquierda hay diagonal arriba-derecha
                if(w[4] & 0x04)
                {

                    pointAux = std::make_pair(1, 1);
                    if(voronoiCell.front() != pointAux)
                    {
                        voronoiPoint = pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
                else if(w[5] & 0x01) // Si no existiera esa conexión, comprobemos si la central tiene una arriba-izquierda
                {
                    //pointAux = std::make_pair(3, 1);
                    //if(voronoiCell.front() != pointAux)
                    {
                        voronoiPoint = std::make_pair(-1, 1); //pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
                else
                {
                    pointAux = std::make_pair(0, 0);
                    if(voronoiCell.front() != pointAux)
                    {
                        voronoiPoint = pointAux;
                        voronoiCell.push_back(voronoiPoint);
                    }
                }
            }
            else
            {
                pointAux = std::make_pair(0, 4);
                if(voronoiPoint != pointAux)
                {
                    voronoiPoint = pointAux;
                    voronoiCell.push_back(voronoiPoint);
                }

                pointAux = std::make_pair(0, 0);
                if(voronoiCell.front() != pointAux)
                {
                    voronoiPoint = pointAux;
                    voronoiCell.push_back(voronoiPoint);
                }
            }

            voronoiGraph[x + y * width] = voronoiCell;
            voronoiCell.clear();
            ++sg;
            if(x == 1 && y == 1)
            {

                std::cout << "Celda (" << x << ", " << y << ") [ " << std::hex << w[2] << std::dec << " ] : ";
                for(Polygon::iterator it = voronoiGraph[x + y * width].begin(); it != voronoiGraph[x + y * width].end(); ++it)
                {
                    std::cout << "--> (" << it->first << ", " << it->second << ") ";
                }
                std::cout << std::endl;
            }
        }
    }

    return voronoiGraph;
}

void drawVoronoiGraph(SDL_Surface* screen, SDL_Surface* sfOrigin, Polygon* voronoiGraph)
{
    int widthPixelArt = sfOrigin->w;
    int heightPixelArt = sfOrigin->h;

    const int zoom = 3;

    //int widthGraph = (widthPixelArt * zoom) * 4; // - 14 - 7;
    //int heightGraph = (heightPixelArt * zoom) * 4;

    int offsWidthScreen = 80; //(screen->w - widthGraph) >> 1;
    int offsHeightScreen = 10; //(screen->h - heightGraph) >> 1;

    //int filledEllipseRGBA(SDL_Surface* dst,
    //                  Sint16 x, Sint16 y,
    //                  Sint16 rx, Sint16 ry,
    //                  Uint8 r, Uint8 g, Uint8 b, Uint8 a);

    int x, y;
    unsigned int* img = (unsigned int*)(sfOrigin->pixels);
    unsigned int  rgb = 0xFF0000FF;
    unsigned int  r = (rgb & 0xFF0000) >> 16;
    unsigned int  g = (rgb & 0xFF00) >> 8;
    unsigned int  b = (rgb & 0xFF);
    unsigned int  a = (rgb & 0xFF000000) >> 24;


    Sint16 xv[18], yv[18], numPoints, dx, dy;

    Polygon voroniCell;

    for(y = 0; y < heightPixelArt; ++y)
    {
        //std::cout << std::endl;
        dy = y << 2;

        for(x = 0; x < widthPixelArt; ++x)
        {
            dx = x << 2;

            rgb = *img++;
            r = (rgb & 0xFF0000) >> 16;
            g = (rgb & 0xFF00) >> 8;
            b = (rgb & 0xFF);
            a = (rgb & 0xFF000000) >> 24;

            voroniCell = voronoiGraph[x + y * widthPixelArt];

            numPoints = 0;

            for(Polygon::iterator it = voroniCell.begin(); it != voroniCell.end(); ++it)
            {
                xv[numPoints] = (((it->first) + dx) * zoom) + offsWidthScreen;
                yv[numPoints] = (((it->second) + dy) * zoom) + offsHeightScreen;
                ++numPoints;
            }

            filledPolygonRGBA(screen, xv, yv, numPoints, r, g, b, a);
            polygonRGBA(screen, xv, yv, numPoints, 255, 0, 0, 255);
        }
    }
}

std::set<Edge> extractSpLines(Polygon* voronoiGraph, SDL_Surface* sfOrigin)
{
    std::set<Edge> visibleEdges = extractVisibleEdges(voronoiGraph, sfOrigin);

    std::list<Polygon> curves;

    for(std::set<Edge>::iterator it = visibleEdges.begin(); it != visibleEdges.end(); ++it)
    {

    }


    return visibleEdges;
}

std::set<Edge> extractVisibleEdges(Polygon* voronoiGraph, SDL_Surface* sfOrigin)
{
    int x, y;
    int width  = sfOrigin->w;
    int height = sfOrigin->h;
    Polygon voronoiCell;
    Point firstPoint;
    Point secondPoint;
    Edge edge;

    Polygon::iterator it;

    unsigned int rgb;

    unsigned int* img = (unsigned int*)(sfOrigin->pixels);

    std::map<Point, std::set<unsigned int> > nodesColors;
    std::map<Edge, std::set<unsigned int> > edgesColors;

    std::set<unsigned int> setColors;

    int dx, dy;

    for(y = 0; y < height; ++y)
    {
        dy = y << 2;

        for(x = 0; x < width; ++x)
        {
            dx = x << 2;

            voronoiCell = voronoiGraph[x + y * width];

            it = voronoiCell.begin();

            rgb = *img;

            firstPoint = *it;

            firstPoint.first += dx;
            firstPoint.second += dy;

            setColors.clear();

            if(nodesColors.count(firstPoint) == 0)
            {
                nodesColors[firstPoint] = setColors;
            }

            nodesColors[firstPoint].insert(rgb);

            ++it;

            for(; it != voronoiCell.end(); ++it)
            {
                secondPoint = *it;

                secondPoint.first += dx;
                secondPoint.second += dy;

                if(nodesColors.count(secondPoint) == 0)
                {
                    setColors.clear();
                    nodesColors[secondPoint] = setColors;
                }

                nodesColors[secondPoint].insert(rgb);

                if(firstPoint.first < secondPoint.first)
                {
                    edge = std::make_pair(firstPoint, secondPoint);
                }
                else if(firstPoint.first > secondPoint.first)
                {
                    edge = std::make_pair(secondPoint, firstPoint);
                }
                else if(firstPoint.second < secondPoint.second)
                {
                    edge = std::make_pair(firstPoint, secondPoint);
                }
                else if(firstPoint.second > secondPoint.second)
                {
                    edge = std::make_pair(secondPoint, firstPoint);
                }
                else // No se debiera presentar nunca este caso, pero por si acaso...
                {
                    edge = std::make_pair(firstPoint, secondPoint);
                }

                if(edgesColors.count(edge) == 0)
                {
                    setColors.clear();
                    edgesColors[edge] = setColors;
                }

                edgesColors[edge].insert(rgb);
                firstPoint = secondPoint;
            }

            secondPoint = voronoiCell.front();
            secondPoint.first += dx;
            secondPoint.second += dy;
            if(firstPoint.first < secondPoint.first)
            {
                edge = std::make_pair(firstPoint, secondPoint);
            }
            else if(firstPoint.first > secondPoint.first)
            {
                edge = std::make_pair(secondPoint, firstPoint);
            }
            else if(firstPoint.second < secondPoint.second)
            {
                edge = std::make_pair(firstPoint, secondPoint);
            }
            else if(firstPoint.second > secondPoint.second)
            {
                edge = std::make_pair(secondPoint, firstPoint);
            }
            else // No se debiera presentar nunca este caso, pero por si acaso...
            {
                edge = std::make_pair(firstPoint, secondPoint);
            }

            if(edgesColors.count(edge) == 0)
            {
                setColors.clear();
                edgesColors[edge] = setColors;
            }

            edgesColors[edge].insert(rgb);

            ++img;
        }
    }

    std::set<Edge> visibleEdges;

    int widthX4 = width << 2;
    int heightX4 = height << 2;

    for(std::map<Edge, std::set<unsigned int> >::iterator itEdge = edgesColors.begin(); itEdge != edgesColors.end(); ++itEdge)
    {
        //int x = itEdge->size();

        if((itEdge->second.size() > 1) || (itEdge->first.first.first == 0 && itEdge->first.second.first == 0)
                                        || (itEdge->first.first.second == 0 && itEdge->first.second.second == 0)
                                       || (itEdge->first.first.first == widthX4 && itEdge->first.second.first == widthX4)
                                        || (itEdge->first.first.second == heightX4 && itEdge->first.second.second == heightX4))
        {
            visibleEdges.insert(itEdge->first);
        }

    }

    return visibleEdges;
}

void drawVisibleEdges(SDL_Surface* screen, SDL_Surface* sfOrigin, std::set<Edge>& visibleEdges)
{
    //int widthPixelArt = sfOrigin->w;
    //int heightPixelArt = sfOrigin->h;

    const int zoom = 3;

    //int widthGraph = (widthPixelArt * zoom) * 4; // - 14 - 7;
    //int heightGraph = (heightPixelArt * zoom) * 4;

    int offsWidthScreen = 80; //(screen->w - widthGraph) >> 1;
    int offsHeightScreen = 10; //(screen->h - heightGraph) >> 1;
/*
    //int filledEllipseRGBA(SDL_Surface* dst,
    //                  Sint16 x, Sint16 y,
    //                  Sint16 rx, Sint16 ry,
    //                  Uint8 r, Uint8 g, Uint8 b, Uint8 a);

    int x, y;
    unsigned int* img = (unsigned int*)(sfOrigin->pixels);
    unsigned int  rgb = 0xFF0000FF;
    unsigned int  r = (rgb & 0xFF0000) >> 16;
    unsigned int  g = (rgb & 0xFF00) >> 8;
    unsigned int  b = (rgb & 0xFF);
    unsigned int  a = (rgb & 0xFF000000) >> 24;

*/
    Sint16 x1, x2, y1, y2;

    for(std::set<Edge>::iterator it = visibleEdges.begin(); it != visibleEdges.end(); ++it)
    {
        x1 = (it->first.first * zoom) + offsWidthScreen;
        y1 = (it->first.second * zoom) + offsHeightScreen;
        x2 = (it->second.first * zoom) + offsWidthScreen;
        y2 = (it->second.second * zoom) + offsHeightScreen;

        thickLineRGBA(screen, x1, y1, x2, y2, 4, 0, 255, 0, 255);

        //xv[numPoints] = (((it->first) + dx) * zoom) + offsWidthScreen;
        //yv[numPoints] = (((it->second) + dy) * zoom) + offsHeightScreen;
    }
/*
    Polygon voroniCell;

    for(y = 0; y < heightPixelArt; ++y)
    {
        for(x = 0; x < widthPixelArt; ++x)
        {
            dx = x << 2;

            rgb = *img++;
            r = (rgb & 0xFF0000) >> 16;
            g = (rgb & 0xFF00) >> 8;
            b = (rgb & 0xFF);
            a = (rgb & 0xFF000000) >> 24;

            voroniCell = voronoiGraph[x + y * widthPixelArt];

            numPoints = 0;

            for(Polygon::iterator it = voroniCell.begin(); it != voroniCell.end(); ++it)
            {
                xv[numPoints] = (((it->first) + dx) * zoom) + offsWidthScreen;
                yv[numPoints] = (((it->second) + dy) * zoom) + offsHeightScreen;
                ++numPoints;
            }

            filledPolygonRGBA(screen, xv, yv, numPoints, r, g, b, a);
            polygonRGBA(screen, xv, yv, numPoints, 255, 0, 0, 255);
        }
    }
*/
}

std::set<Polygon> extractCurves(Polygon* voronoiGraph, SDL_Surface* sfOrigin)
{
    std::set<Polygon> curves;

    int x, y;
    int width  = sfOrigin->w;
    int height = sfOrigin->h;
    Polygon voronoiCell;
    Point firstPoint, secondPoint, pointAux;
    Edge edge;

    Polygon::iterator it;

    unsigned int rgb;

    unsigned int* img = (unsigned int*)(sfOrigin->pixels);

    std::map<Point, std::set<unsigned int> > nodesColors;
    std::map<Edge, std::set<unsigned int> > edgesColors;
    std::map<Point, std::list<Edge> > nodesEdges;

    std::set<unsigned int> setColors;
    std::list<Edge> setEdges;
    Polygon polygonAux;

    int dx, dy;


    //  Por cada punto...
    for(y = 0; y < height; ++y)
    {
        dy = y << 2;

        for(x = 0; x < width; ++x)
        {
            dx = x << 2;


            // Se trata de ir almacenando los puntos de la célula en el mapa nodesColors, indicando con cuantos colores
            // limitan. Cada punto del gráfico bitmap se multiplica por 4, y los puntos de la celda se consideran desde
            // ahí. Es decir El punto 0,0 en el gráfico pixels, abarca la región (0,0)-(4.4) en el diagrama de
            // Voronoi. Los puntos que forman las celdas siempre estarán en los puntos exactos del diagrama.
            voronoiCell = voronoiGraph[x + y * width];


            //  Nos preparamos para meter el primer punto. Este lo hacemos separado. El resto los metemos desde un
            // bucle, ya que además iremos metiendo segmentos uniendo cada vertice con el lleído previamente.
            // Vale, podría meter este también en un bucle, pero así me ahorro un if.
            it = voronoiCell.begin();

            rgb = *img;

            firstPoint = *it;

            firstPoint.first += dx;
            firstPoint.second += dy;

            setColors.clear();

            if(nodesColors.count(firstPoint) == 0)
            {
                nodesColors[firstPoint] = setColors;
            }

            nodesColors[firstPoint].insert(rgb);

            ++it;

            for(; it != voronoiCell.end(); ++it)
            {
                //  Ahora vamos repitiendo el proceso con los siguientes puntos
                secondPoint = *it;

                secondPoint.first += dx;
                secondPoint.second += dy;

                if(nodesColors.count(secondPoint) == 0)
                {
                    setColors.clear();
                    nodesColors[secondPoint] = setColors;
                }

                nodesColors[secondPoint].insert(rgb);


                //  Y ahora formamos el segmento.
                if(firstPoint.first < secondPoint.first)
                {
                    edge = std::make_pair(firstPoint, secondPoint);
                }
                else if(firstPoint.first > secondPoint.first)
                {
                    edge = std::make_pair(secondPoint, firstPoint);
                }
                else if(firstPoint.second < secondPoint.second)
                {
                    edge = std::make_pair(firstPoint, secondPoint);
                }
                else if(firstPoint.second > secondPoint.second)
                {
                    edge = std::make_pair(secondPoint, firstPoint);
                }
                else // No se debiera presentar nunca este caso, pero por si acaso...
                {
                    edge = std::make_pair(firstPoint, secondPoint);
                }

                // Y ahora insertamos el segmento en map edgesColors, en el que a su vez llevamos una cuenta
                // de con cuantos colores choca.

                if(edgesColors.count(edge) == 0)
                {
                    setColors.clear();
                    edgesColors[edge] = setColors;
                }

                edgesColors[edge].insert(rgb);

                // Insertamos el edge en el map nodesEdges, dos veces uno por cada uno de los dos puntos.

if(edge.first.first == 0 && edge.first.second == 0)
{
    std::cout << "xxx Vertice (" << edge.first.first << ", " << edge.first.second << ")" << std::endl;
    std::cout << "2º Vertice (" << edge.second.first << ", " << edge.second.second << ")" << std::endl;
    std::cout << "Numero segmentos del nodo:" << nodesEdges.count(edge.first) << std::endl;
    //std::cout << "Segmento: (" << (nodesEdges[firstPoint].begin())->first.first << ", " << (nodesEdges[firstPoint].begin())->first.second << ") - (" << (nodesEdges[firstPoint].begin())->second.first << ", " << (nodesEdges[firstPoint].begin())->second.second << ")" << std::endl;
}

                if(nodesEdges.count(edge.first) == 0)
                {
                    setEdges.clear();
                    nodesEdges[edge.first] = setEdges;
                }

                if(nodesEdges.count(edge.second) == 0)
                {
                    setEdges.clear();
                    nodesEdges[edge.second] = setEdges;
                }

if(edge.first.first == 0 && edge.first.second == 0)
{
    std::cout << "insertando 1 xxx Vertice (" << edge.first.first << ", " << edge.first.second << ")" << std::endl;
    std::cout << "2º Vertice (" << edge.second.first << ", " << edge.second.second << ")" << std::endl;
    std::cout << "Numero segmentos del nodo:" << nodesEdges.count(edge.first) << std::endl;
    //std::cout << "Segmento: (" << (nodesEdges[firstPoint].begin())->first.first << ", " << (nodesEdges[firstPoint].begin())->first.second << ") - (" << (nodesEdges[firstPoint].begin())->second.first << ", " << (nodesEdges[firstPoint].begin())->second.second << ")" << std::endl;
}

                nodesEdges[edge.first].push_back(edge);
                pointAux = edge.first;
                edge.first = edge.second;
                edge.second = pointAux;
                nodesEdges[edge.first].push_back(edge);
if(edge.first.first == 0 && edge.first.second == 0)
{
    std::cout << "insertando 2 xxx Vertice (" << edge.first.first << ", " << edge.first.second << ")" << std::endl;
    std::cout << "2º Vertice (" << edge.second.first << ", " << edge.second.second << ")" << std::endl;
    std::cout << "Numero segmentos del nodo:" << nodesEdges.count(edge.first) << std::endl;
    //std::cout << "Segmento: (" << (nodesEdges[firstPoint].begin())->first.first << ", " << (nodesEdges[firstPoint].begin())->first.second << ") - (" << (nodesEdges[firstPoint].begin())->second.first << ", " << (nodesEdges[firstPoint].begin())->second.second << ")" << std::endl;
}

                firstPoint = secondPoint;
            }

            //  Finalmente repetimos toda la operación enlazando el primer punto con el último.

            secondPoint = voronoiCell.front();
            secondPoint.first += dx;
            secondPoint.second += dy;
            if(firstPoint.first < secondPoint.first)
            {
                edge = std::make_pair(firstPoint, secondPoint);
            }
            else if(firstPoint.first > secondPoint.first)
            {
                edge = std::make_pair(secondPoint, firstPoint);
            }
            else if(firstPoint.second < secondPoint.second)
            {
                edge = std::make_pair(firstPoint, secondPoint);
            }
            else if(firstPoint.second > secondPoint.second)
            {
                edge = std::make_pair(secondPoint, firstPoint);
            }
            else // No se debiera presentar nunca este caso, pero por si acaso...
            {
                edge = std::make_pair(firstPoint, secondPoint);
            }

            if(edgesColors.count(edge) == 0)
            {
                setColors.clear();
                edgesColors[edge] = setColors;
            }

            edgesColors[edge].insert(rgb);

if(edge.first.first == 0 && edge.first.second == 0)
{
    std::cout << "yyy Vertice (" << edge.first.first << ", " << edge.first.second << ")" << std::endl;
    std::cout << "2º Vertice (" << edge.second.first << ", " << edge.second.second << ")" << std::endl;
    std::cout << "Numero segmentos del nodo:" << nodesEdges.count(edge.first) << std::endl;
    //std::cout << "Segmento: (" << (nodesEdges[firstPoint].begin())->first.first << ", " << (nodesEdges[firstPoint].begin())->first.second << ") - (" << (nodesEdges[firstPoint].begin())->second.first << ", " << (nodesEdges[firstPoint].begin())->second.second << ")" << std::endl;
}

            if(nodesEdges.count(edge.first) == 0)
            {
                setEdges.clear();
                nodesEdges[edge.first] = setEdges;
            }

            if(nodesEdges.count(edge.second) == 0)
            {
                setEdges.clear();
                nodesEdges[edge.second] = setEdges;
            }

            nodesEdges[edge.first].push_back(edge);

if(edge.first.first == 0 && edge.first.second == 0)
{
    std::cout << "insertando 1 yyy Vertice (" << edge.first.first << ", " << edge.first.second << ")" << std::endl;
    std::cout << "2º Vertice (" << edge.second.first << ", " << edge.second.second << ")" << std::endl;
    std::cout << "Numero segmentos del nodo:" << nodesEdges.count(edge.first) << std::endl;
    //std::cout << "Segmento: (" << (nodesEdges[firstPoint].begin())->first.first << ", " << (nodesEdges[firstPoint].begin())->first.second << ") - (" << (nodesEdges[firstPoint].begin())->second.first << ", " << (nodesEdges[firstPoint].begin())->second.second << ")" << std::endl;
}

            pointAux = edge.first;
            edge.first = edge.second;
            edge.second = pointAux;
            nodesEdges[edge.first].push_back(edge);
if(edge.first.first == 0 && edge.first.second == 0)
{
    std::cout << "insertando 2 yyy Vertice (" << edge.first.first << ", " << edge.first.second << ")" << std::endl;
    std::cout << "2º Vertice (" << edge.second.first << ", " << edge.second.second << ")" << std::endl;
    std::cout << "Numero segmentos del nodo:" << nodesEdges.count(edge.first) << std::endl;
    //std::cout << "Segmento: (" << (nodesEdges[firstPoint].begin())->first.first << ", " << (nodesEdges[firstPoint].begin())->first.second << ") - (" << (nodesEdges[firstPoint].begin())->second.first << ", " << (nodesEdges[firstPoint].begin())->second.second << ")" << std::endl;
}
            ++img;
        }
    }

    std::set<Edge> visibleEdges;

    int widthX4 = width << 2;
    int heightX4 = height << 2;

    for(std::map<Edge, std::set<unsigned int> >::iterator itEdge = edgesColors.begin(); itEdge != edgesColors.end(); ++itEdge)
    {
        //int x = itEdge->size();

        if((itEdge->second.size() > 1) || (itEdge->first.first.first == 0 && itEdge->first.second.first == 0)
                                        || (itEdge->first.first.second == 0 && itEdge->first.second.second == 0)
                                       || (itEdge->first.first.first == widthX4 && itEdge->first.second.first == widthX4)
                                        || (itEdge->first.first.second == heightX4 && itEdge->first.second.second == heightX4))
        {
            visibleEdges.insert(itEdge->first);
        }

    }

    std::cout << "Numero segmentos extraidos:" << visibleEdges.size() << std::endl;

    //  Vale, busquemos segmentos conectados, y juntémoslos para formar curvas.
    // Necesito guardar las que se van usando en otro set.
    std::set<Edge> edgesUsed;

    for(std::set<Edge>::iterator it = visibleEdges.begin(); it != visibleEdges.end(); ++it)
    {
        if(edgesUsed.find(*it) == edgesUsed.end())
        {
            edgesUsed.insert(*it);
            polygonAux.clear();

            firstPoint = (*it).first;
            secondPoint = (*it).second;

if(firstPoint.first == 0 && firstPoint.second == 0)
{
    std::cout << "Vertice (" << firstPoint.first << ", " << firstPoint.second << ")" << std::endl;
    std::cout << "2º Vertice (" << secondPoint.first << ", " << secondPoint.second << ")" << std::endl;
    std::cout << "Numero segmentos del nodo:" << nodesEdges.count(firstPoint) << std::endl;
    std::cout << "Segmento: (" << (nodesEdges[firstPoint].begin())->first.first << ", " << (nodesEdges[firstPoint].begin())->first.second << ") - (" << (nodesEdges[firstPoint].begin())->second.first << ", " << (nodesEdges[firstPoint].begin())->second.second << ")" << std::endl;
}

            polygonAux.push_back(firstPoint);
            polygonAux.push_back(secondPoint);

            pointAux = secondPoint;
            secondPoint = firstPoint;

            while(pointAux != firstPoint && nodesEdges.count(pointAux) == 2)
            {
                std::cout << "xxxx" << std::endl;
                for(std::list<Edge>::iterator itEdge = nodesEdges[pointAux].begin(); itEdge != nodesEdges[pointAux].end(); ++itEdge)
                {
                    if((*itEdge).second != secondPoint)
                    {
                        secondPoint = pointAux;
                        pointAux = (*itEdge).second;
                        polygonAux.push_back(pointAux);
                        break;
                    }
                }
            }

            //  Si no hemos cerrado la curva...
            if(pointAux != firstPoint)
            {
                firstPoint = (*it).second;
                secondPoint = (*it).first;
                pointAux = secondPoint;
                secondPoint = firstPoint;
                while(nodesEdges.count(pointAux) == 2)
                {
                    for(std::list<Edge>::iterator itEdge = nodesEdges[pointAux].begin(); itEdge != nodesEdges[pointAux].end(); ++itEdge)
                    {
                        if((*itEdge).second != secondPoint)
                        {
                            secondPoint = pointAux;
                            pointAux = (*itEdge).second;
                            polygonAux.push_back(pointAux);
                            break;
                        }
                    }
                }
            }

            curves.insert(polygonAux);
        }
    }
    std::cout << "Numero segmentos usados:" << edgesUsed.size() << std::endl;
    return curves;
}

void drawCurves(SDL_Surface* screen, SDL_Surface* sfOrigin, std::set<Polygon>& curves)
{
//int widthPixelArt = sfOrigin->w;
    //int heightPixelArt = sfOrigin->h;

    const int zoom = 3;

    //int widthGraph = (widthPixelArt * zoom) * 4; // - 14 - 7;
    //int heightGraph = (heightPixelArt * zoom) * 4;

    int offsWidthScreen = 80; //(screen->w - widthGraph) >> 1;
    int offsHeightScreen = 10; //(screen->h - heightGraph) >> 1;
/*
    //int filledEllipseRGBA(SDL_Surface* dst,
    //                  Sint16 x, Sint16 y,
    //                  Sint16 rx, Sint16 ry,
    //                  Uint8 r, Uint8 g, Uint8 b, Uint8 a);

    int x, y;
    unsigned int* img = (unsigned int*)(sfOrigin->pixels);
    unsigned int  rgb = 0xFF0000FF;
    unsigned int  r = (rgb & 0xFF0000) >> 16;
    unsigned int  g = (rgb & 0xFF00) >> 8;
    unsigned int  b = (rgb & 0xFF);
    unsigned int  a = (rgb & 0xFF000000) >> 24;

*/
    Sint16 x1, x2, y1, y2;
    Sint16 *vx, *vy;
    int numVertex, n;
    Polygon polygon;
    Polygon::iterator itVertex;

    for(std::set<Polygon>::iterator it = curves.begin(); it != curves.end(); ++it)
    {
        polygon = *it;
        numVertex = polygon.size();
        //std::cout << "Vertices:" << numVertex << std::endl;
        if(numVertex > 2)
        {
            vx = new Sint16[numVertex];
            vy = new Sint16[numVertex];

            n = 0;

            for(itVertex = polygon.begin(); itVertex != polygon.end(); ++itVertex)
            {
                vx[n] = ((*itVertex).first * zoom) + offsWidthScreen;
                vy[n++] = ((*itVertex).second * zoom) + offsHeightScreen;
            }
            bezierRGBA(screen, vx, vy, numVertex, 3, 0, 255, 0, 0);
            delete [] vx;
            delete [] vy;
        }
        else
        {
            itVertex = polygon.begin();
            x1 = ((*itVertex).first * zoom) + offsWidthScreen;
            y1 = ((*itVertex).second * zoom) + offsHeightScreen;
            ++itVertex;
            x2 = ((*itVertex).first * zoom) + offsWidthScreen;
            y2 = ((*itVertex).second * zoom) + offsHeightScreen;

            //int bezierRGBA(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy,
            //int n, int s, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
            thickLineRGBA(screen, x1, y1, x2, y2, 4, 0, 255, 0, 255);
        }


        //xv[numPoints] = (((it->first) + dx) * zoom) + offsWidthScreen;
        //yv[numPoints] = (((it->second) + dy) * zoom) + offsHeightScreen;
    }
}
