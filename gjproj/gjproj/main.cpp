#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "open-simplex-noise.h"

#define simplexScale 80
#define amplitude 0.2f

#define g 100.0f

class Chunk {
public:
	static const int width = 16;
	static const int height = 256;
	int data[width * height];
	void generate(osn_context* osCont, int chunk) {
		memset(data, 0, sizeof(int) * width * height);
		for (int x = 0; x < width; x++) {
			int h = (open_simplex_noise2(osCont, (double)(x+(chunk*width)) / simplexScale, 0)*amplitude+1)/2*height;
			int dirth = (open_simplex_noise2(osCont, (double)(x + (chunk * width)) / simplexScale, 0) * amplitude + 1) / 2 * 10;
			for (int y = 0; y < h; y++) {
				data[y * width + x] = 1;
			}
			for (int y = h; y < h+dirth; y++) {
				data[y * width + x] = 2;
			}
		}
	}
};

class World {
public:
	std::vector<Chunk*> chunks;
	std::vector<bool> loaded;
	int width = 0;
	osn_context* oSimpContext;
	World(int seed) : loaded(), chunks(), oSimpContext(nullptr){
		open_simplex_noise(seed, &oSimpContext);
		printf("loaded World");
	}
	~World() {
		for (int i = 0; i < chunks.size(); i++) {
			if(loaded[i]) delete chunks[i];
			//if(oSimpContext) open_simplex_noise_free(oSimpContext);
		}
	}
	void load(int sx, int screenWidth) {
		for (int i = 0; i < (sx - screenWidth / 2) / Chunk::width; i++) {
			if (loaded[i]) {
				delete chunks[i];
				loaded[i] = false;
			}
		}
		for (int i = (sx - screenWidth/2)/Chunk::width; i < (sx + screenWidth / 2) / Chunk::width + 1; i++) {
			if (i >= chunks.size() || !loaded[i]) {
				for (int w = 0; w < i - chunks.size(); i++) {
					if (i >= chunks.size()) {
						chunks.push_back(nullptr);
						loaded.push_back(false);
						width++;
					}
				}
				Chunk* c = new Chunk();
				c->generate(oSimpContext, i);
				chunks.push_back(c);
				loaded.push_back(true);
				width++;
			}

		}
	}
	int tileAt(int x, int y) {
		if (x >= 0 && x / Chunk::width <= width && loaded[x / Chunk::width] && y < Chunk::height && y >= 0) {
			return chunks[x / Chunk::width]->data[y * Chunk::width + x % Chunk::width];
		}
		return -1;
	}
};

class Player {
public:
	float posx;
	float posy;
	float velx = 10.0f;
	float vely = 0;
	float width = 2;
	float height = 3;
	int health = 5;
	int healthMax = 5;
	bool moveLeft = false;
	bool moveRight = false;
	bool moveUp = false;
	bool moveDown = false;
	bool landed = false;
	olc::Sprite sprite;
	Player(int px, int py, const char* fname) : sprite(fname), posx(px), posy(py)  {

	}
	void update(World* world, float fElapsedTime) {
		if (moveLeft) {
			velx -= 100 * fElapsedTime;
		}
		if (moveRight) {
			velx += 100 * fElapsedTime;
		}
		if (moveDown) {
			vely -= 100 * fElapsedTime;
		}
		if (moveUp && landed) {
			vely += 50;
			landed = false;
		}
		if (world->tileAt(posx, posy - 1) == 0 || world->tileAt(posx, posy - 1) == -1) {
			vely -= g * fElapsedTime;
			landed = false;
		}
		else if (vely < 0) {
			vely = 0;
			landed = true;
		}
		if (velx > 0 && !(world->tileAt(posx + 1, posy) == 0)) {
			velx = 0;
		}
		if (velx < 0 && !(world->tileAt(posx - 1, posy) == 0)) {
			velx = 0;
		}
		if (vely > 0 && !(world->tileAt(posx, posy + 1) == 0)) {
			vely = 0;
		}
		velx *= 0.95f;
		vely *= 0.95f;

		posx += velx * fElapsedTime;
		posy += vely * fElapsedTime;

	}

};

class ourGame : public olc::PixelGameEngine {
public:
	bool stop = false;
	std::vector<olc::Sprite*> tiles;
	float aspectRatio;
	World world;
	Player player;
	ourGame() : olc::PixelGameEngine(), world(0), aspectRatio(1), tiles(), player(20, 150, "assets/entities/player.png"){

	}
	bool OnUserCreate() {
		aspectRatio = ScreenWidth() / (float)ScreenHeight();
		tiles.push_back(new olc::Sprite("assets/blocks/air.png"));
		tiles.push_back(new olc::Sprite("assets/test.png"));
		tiles.push_back(new olc::Sprite("assets/blocks/dirt.png"));
		tiles.push_back(new olc::Sprite("assets/blocks/iron.png"));
		//world = World(0);
		world.load(player.posx, 40);
		return true;
	}

	void drawTileMap(int* tileMap, int tileMapW, int tileMapH, float centerW, float centerH, float drawW, float drawH, float xoff, float yoff, bool fillClear = false) {
		int xStart = (0       > centerW  + xoff + drawW / 2)          ? 0              :  (centerW + xoff + drawW / 2) * ScreenWidth()  / drawW;
		int xStop = (1 < (tileMapW + centerW + xoff + drawW / 2)/drawW) ? ScreenWidth()  : (tileMapW + centerW + xoff + drawW / 2) * ScreenWidth()  / drawW;
		int yStart = (0       > centerH  + yoff + drawH / 2)          ? 0              :  (centerH + yoff + drawH / 2) * ScreenHeight() / drawH;
		int yStop = (1 < (tileMapH + centerH + yoff + drawH / 2)/drawH) ? ScreenHeight() : (tileMapH + centerH + yoff + drawH / 2) * ScreenHeight() / drawH;
		for (int y = yStart; y < yStop; y++) {
			for (int x = xStart; x < xStop; x++) {
				float tilePosX = x * drawW / ScreenWidth() + centerW - xoff - drawW / 2;
				float tilePosY = y * drawH / ScreenHeight() + centerH - yoff - drawH / 2;
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

	void drawTile(int tile, float centerW, float centerH, float drawW, float drawH, float xoff, float yoff) {
		drawTile(tiles[tile], centerW, centerH, drawW, drawH, xoff, yoff);
	}
	void drawTile(olc::Sprite* tile, float centerW, float centerH, float drawW, float drawH, float xoff, float yoff) {
		int xStart = (0 > centerW + xoff + drawW / 2) ? 0 : (centerW + xoff + drawW / 2) * ScreenWidth() / drawW;
		int xStop = (1 < (1 + centerW + xoff + drawW / 2) / drawW) ? ScreenWidth() : (1 + centerW + xoff + drawW / 2) * ScreenWidth() / drawW;
		int yStart = (0 > centerH + yoff + drawH / 2) ? 0 : (centerH + yoff + drawH / 2) * ScreenHeight() / drawH;
		int yStop = (1 < (1 + centerH + yoff + drawH / 2) / drawH) ? ScreenHeight() : (1 + centerH + yoff + drawH / 2) * ScreenHeight() / drawH;
		for (int y = yStart; y < yStop; y++) {
			for (int x = xStart; x < xStop; x++) {
				float tilePosX = x * drawW / ScreenWidth() + centerW - xoff;
				float tilePosY = y * drawH / ScreenHeight() + centerH - yoff;
				int spritePX = (tilePosX - (int)tilePosX) * tile->width;
				int spritePY = (tilePosY - (int)tilePosY) * tile->height;
				Draw(x, y, tile->GetPixel(spritePX, spritePY));
			}
		}
	}

	bool OnUserUpdate(float fElapsedTime) {
		int screenTileW = 30;
		player.moveDown = GetKey(olc::Key::DOWN).bHeld || GetKey(olc::Key::S).bHeld;
		player.moveUp = GetKey(olc::Key::UP).bHeld || GetKey(olc::Key::W).bHeld;
		player.moveLeft = GetKey(olc::Key::LEFT).bHeld || GetKey(olc::Key::A).bHeld;
		player.moveRight = GetKey(olc::Key::RIGHT).bHeld || GetKey(olc::Key::D).bHeld;



		player.update(&world, fElapsedTime);

		for (int i = 0; i < world.chunks.size(); i++) {
			if (world.loaded[i]) {
				drawTileMap(world.chunks[i]->data, Chunk::width, Chunk::height, 0, 0, screenTileW, screenTileW / aspectRatio, i*Chunk::width - player.posx, - player.posy, false);

			}
		}
		int t = 1;
		world.load(player.posx, screenTileW);
		drawTile(&player.sprite, 0, 0, screenTileW/2, screenTileW / aspectRatio/3, -1.0f/3, -1);
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
		return true;
	}



};

int main() {
	ourGame game = ourGame();
	if (game.Construct(800, 600, 1, 1)) {
		game.Start();
	}
	return 0;
}