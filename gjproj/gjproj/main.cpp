#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "open-simplex-noise.h"

#define simplexScale 110
#define amplitude 0.4f

class Chunk {
public:
	static const int width = 16;
	static const int height = 256;
	int data[width * height];
	void generate(osn_context* osCont, int chunk) {
		memset(data, 0, sizeof(int) * width * height);
		for (int x = 0; x < width; x++) {
			int h = (open_simplex_noise2(osCont, (double)(x+(chunk*width)) / simplexScale, 0)*amplitude+1)/2*height;
			printf("%i\n", h);
			for (int y = 0; y < h; y++) {
				data[y * width + x] = 1;
			}
		}
	}
};



class ourGame : public olc::PixelGameEngine {
public:
	int n = 0;
	bool stop = false;
	std::vector<olc::Sprite*> tiles;
	int map[9] = { 0,1,2,3,2,2,3,0,1 };
	float aspectRatio;
	osn_context* oSimpContext;
	std::vector<Chunk*> chunks;
	bool OnUserCreate() {
		open_simplex_noise(rand(), &oSimpContext);
		aspectRatio = ScreenWidth() / (float)ScreenHeight();
		tiles = std::vector<olc::Sprite*>();
		tiles.push_back(new olc::Sprite("assets/b.png"));
		tiles.push_back(new olc::Sprite("assets/w.png"));
		tiles.push_back(new olc::Sprite("assets/3.png"));
		tiles.push_back(new olc::Sprite("assets/4.png"));
		for (int i = 0; i < 20; i++) {
			Chunk* c = new Chunk();
			c->generate(oSimpContext, i);
			chunks.push_back(c);
		}
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
					Draw(x, ScreenHeight()-y, tile->GetPixel(spritePX, spritePY));
				}
			}
		}
	}

	bool OnUserUpdate(float fElapsedTime) {
		for (int i = 0; i < chunks.size(); i++) {
			drawTileMap(chunks[i]->data, chunks[i]->width, chunks[i]->height, 60-i*chunks[i]->width, 120, 120, 120 * aspectRatio, false);
		}
		/*
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
		*/
		return true;
	}

	bool OnUserDestroy() {
		for (int i = 0; i < tiles.size(); i++) {
			delete tiles[i];
		}
		for (int i = 0; i < chunks.size(); i++) {
			delete chunks[i];
		}
		open_simplex_noise_free(oSimpContext);
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