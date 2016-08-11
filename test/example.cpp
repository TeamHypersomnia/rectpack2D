#include <cstdio>
#include "pack/pack.h"

using namespace std;

#define RECTS 200

int main () {
	rect_xywhf rects[RECTS], *ptr_rects[RECTS];

	for(int i = 0; i < RECTS; ++i) {
		rects[i] = rect_xywhf(0, 0, 7+rand()%50,  7+rand()%50);
		ptr_rects[i] = rects+i;
	}

	vector<bin> bins;

	if(pack(ptr_rects, RECTS, 400, bins)) {
		printf("bins: %d\n", bins.size()); 

		for(int i = 0; i < bins.size(); ++i) {
			printf("\n\nbin: %dx%d, rects: %d\n", bins[i].size.w, bins[i].size.h, bins[i].rects.size());

			for(int r = 0; r < bins[i].rects.size(); ++r) {
				rect_xywhf* rect = bins[i].rects[r];

				printf("rect %d: x: %d, y: %d, w: %d, h: %d, was flipped: %s\n", r, rect->x, rect->y, rect->w, rect->h, rect->flipped ? "yes" : " no"); 
			}
		}
	}
	else {
		printf("failed: there's a rectangle with width/height bigger than max_size!\n");
	}


	system("pause");
	return 0;
}