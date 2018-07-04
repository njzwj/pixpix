/*
Copyright 2018 njzwj. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software 
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
*/

#ifndef __CLI_GRAPH_
#define __CLI_GRAPH_

#include <stdio.h>
#include <Windows.h>
#include <conio.h>

/*!
    \brief Output image through console, supports rgb
    \param w width of img
    \param h height of img
    \param img unsigned char array containing rgb
 */
void cli_graph(unsigned w, unsigned h, const unsigned char* img) {
    /* gray scale */
    static char gray[] = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    static int gray_num = sizeof(gray);
    /* controls rgb */
    static unsigned char r = 4, g = 2, b = 1;
    static HANDLE handle_out;                                               /* console write handle */
    static CONSOLE_SCREEN_BUFFER_INFO screen_info;                          /* screen info */
    static CHAR_INFO *buf = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * w * h);  /* screen buffer */
    static SMALL_RECT srect;                                                /* drawing space */
    handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(handle_out, &screen_info);

    const unsigned char *p = img;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int lumi = 0.2989 * p[0] + 0.5870 * p[1] + 0.1140 * p[2];
            int idx = lumi / 255.0 * 48;                                                            /* get luminance */
            int mx = p[0]>p[1]?p[0]:p[1]; mx = mx>p[2]?mx:p[2];
            int tresh = mx / 1.5;                                                                   /* set rgb treshold */
            int color = (p[0]>tresh?r:0) | (p[1]>tresh?g:0) | (p[2]>tresh?b:0) | (lumi>127?8:0);    /* set rgb */
            p += 3;
            buf[y*w+x].Char.AsciiChar = gray[lumi>127?idx:gray_num-49+idx];
            buf[y*w+x].Attributes = color;
        }
    }
    srect = {0, 0, (short)(w-1), (short)(h-1)};

    WriteConsoleOutput(handle_out,                  /* output handle */
                       buf,                         /* buffer */
                       (COORD){(short)w, (short)h}, /* coord size */
                       (COORD){0,0},                /* original coord */
                       &srect                       /* drawing size */
    );
}

#endif