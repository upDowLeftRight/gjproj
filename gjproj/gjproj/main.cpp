#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"


class ourGame : public olc::PixelGameEngine {
public:
	int n = 0;
	bool stop = false;
	bool OnUserCreate() {
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) {
		n++;
		if (n >= 400 * 300) {
			stop = true;
		}
		if (!stop) {
			for (int i = 2; i < n; i++) {
				if ((n / i) * i == n) {
					Draw(n % 400, n / 400, olc::Pixel(0, 0, 0));
					return true;
				}
				printf("%i\n", n);
			}
			Draw(n % 400, n / 400, olc::Pixel(255, 255, 255));
		}
		return true;
	}

	bool OnUserDestroy() {
		return true;
	}


};

int main() {
	ourGame game = ourGame();
	if (game.Construct(400, 300, 2, 2)) {
		game.Start();
	}
	return 0;
}