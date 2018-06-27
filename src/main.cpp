#include <cstdio>
#include <cstdlib>
#include "cli_graph.h"
#include "svpng.h"

int main() {
    const int W = 64, H = 32;
    unsigned char img[W * H * 3], *p = img;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            p[0] = 255 / H * y; p[1] = 255 / W * x; p[2] = 128;
            p += 3;
        }
    }
    cli_graph(W, H, img);
    getchar();

    FILE *fp = fopen("./test.png", "wb");
    svpng(fp, W, H, img, 0);
    fclose(fp);

    return 0;
}