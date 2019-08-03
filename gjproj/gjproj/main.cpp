#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

struct Chunk {
public:
	static const int width = 16;
	static const int height = 256;
	int data[width * height];
};


class ourGame : public olc::PixelGameEngine {
public:
	int n = 0;
	bool stop = false;
	std::vector<olc::Sprite*> tiles;
	int map[9] = { 0,1,2,3,2,2,3,0,1 };
	float aspectRatio;
	bool OnUserCreate() {
		aspectRatio = ScreenWidth() / (float)ScreenHeight();
		tiles = std::vector<olc::Sprite*>();
		tiles.push_back(new olc::Sprite("assets/1.png"));
		tiles.push_back(new olc::Sprite("assets/2.png"));
		tiles.push_back(new olc::Sprite("assets/3.png"));
		tiles.push_back(new olc::Sprite("assets/4.png"));

		return true;
	}

	void drawTileMap(int* tileMap, int tileMapW, int tileMapH, float centerW, float centerH, float drawW, float drawH, bool fillClear = false) {
		for (int y = 0; y < ScreenHeight(); y++) {
			for (int x = 0; x < ScreenWidth(); x++) {
				float tilePosX = x * drawW / ScreenWidth() + centerW - drawW / 2;
				float tilePosY = y * drawH / ScreenHeight() + centerH - drawH / 2;
				if (tilePosX < 0 || tilePosY < 0 || tilePosX >= tileMapW || tilePosY >= tileMapH) {
					if(fillClear)
					Draw(x, y, olc::BLACK);
				}
				else {
					int tileInd = tileMap[((int)tilePosY * tileMapW + (int)tilePosX)];
					olc::Sprite* tile = tiles.at(tileInd);
					int spritePX = (tilePosX - (int)tilePosX) * tile->width;
					int spritePY = (tilePosY - (int)tilePosY) * tile->height;
					Draw(x, y, tile->GetPixel(spritePX, spritePY));
				}
			}
		}
	}

	bool OnUserUpdate(float fElapsedTime) {
		drawTileMap(map, 3, 3, 0, 0, 5, 5 * aspectRatio, false);
		
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
				//printf("%i\n", n);
			}
			Draw(n % 400, n / 400, olc::Pixel(255, 255, 255));
		}

		return true;
	}

	bool OnUserDestroy() {
		for (int i = 0; i < tiles.size(); i++) {
			delete tiles[i];
		}
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