#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "open-simplex-noise.h"

#define simplexScale 80
#define amplitude 0.2f

#define g 100.0f

struct Rect {
	float left;
	float right;
	float up;
	float down;
};

bool rrColide(Rect* r1, Rect* r2) {
	if (r1->up < r2->down) {
		return false;
	}
	else if (r1->down > r2->up) {
		return false;
	}
	else if (r1->left > r2->right) {
		return false;
	}
	else if (r1->right < r2->left) {
		return false;
	}
	return true;
}

class Chunk {
public:
	static const int width = 16;
	static const int height = 256;
	int data[width * height];
	std::vector<Rect> boxes;
	void generate(osn_context* osCont, int chunk) {
		boxes = std::vector<Rect>();
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
		
		for (int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++)
				if (data[y * width + x] != 0) {
					if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
						Rect box;
						box.left = x + chunk * width;
						box.right = x + chunk * width + 1;
						box.up = y + 1;
						box.down = y;
						boxes.push_back(box);
					}
					else {
						if (data[y * width + x + 1] == 0 ||
							data[y * width + x - 1] == 0 ||
							data[y * width + x + width] == 0 ||
							data[y * width + x - width] == 0) {

							Rect box;
							box.left = x + chunk * width;
							box.right = x + chunk * width + 1;
							box.up = y + 1;
							box.down = y;
							boxes.push_back(box);

						}
					}
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

class Item {
public:
	olc::Sprite icon;
	int placeType;
	int attackDamage;
	Item(const char* fname, int pt, int ad): icon(fname), placeType(pt), attackDamage(ad) {}
	~Item() {
	}
};

class Inventory {
public:
	std::vector<std::pair<Item*, int>> slots;
	int activeSlot = 0;
	olc::Sprite* ico;
	olc::Sprite* icosel;
	Item* none;
	Inventory(): slots(9), activeSlot(0){
		ico = new olc::Sprite("assets/ui/hotbar.png");
		icosel = new olc::Sprite("assets/ui/hotbar selected.png");
	}
	~Inventory() {
		delete ico;
		delete icosel;
	}
	bool add(){
		return false;
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
	Inventory inv;
	Player(int px, int py, const char* fname) : sprite(fname), posx(px), posy(py), inv() {

	}
	void update(World* world, float fElapsedTime) {
		if (moveLeft) {
			velx = -20;
		}
		else if (moveRight) {
			velx = 20;
		}
		else {
			velx = 0;
		}
		if (moveDown) {
			vely -= 10;
		}
		if (moveUp && landed) {
			vely += 30;
			landed = false;
		}
		
		if (world->tileAt(posx, posy - 1) == 0 && world->tileAt(posx-1, posy - 1) == 0) {
			vely -= g * fElapsedTime;
			landed = false;
		}
		else if (vely < 0) {
			vely = 0;
			landed = true;
		}
		//velx *= 0.9f;
		//vely *= 0.95f;

		float posxn = posx + velx * fElapsedTime;
		float posyn = posy + vely * fElapsedTime;

		Rect hb;
		hb.down = posyn;
		hb.left = posxn;
		hb.right = posxn + 2;
		hb.up = posyn + 3;
		int i = 0;
		bool colide = false;
		for (auto& ch : world->chunks) {
			if (world->loaded[i]) {
				for (auto& hb2 : ch->boxes) {
					if (rrColide(&hb, &hb2)) {
						//printf("stuck");
						colide = true;
						break;
					}
				}
				if (colide) break;
			}
			i++;
		}
		if (!colide) {
			posx = posxn;
			posy = posyn;
		}
		else {
			colide = false;
			vely = 0;
			landed = true;
			float posxn = posx + velx * fElapsedTime;
			float posyn = posy + vely * fElapsedTime;

			Rect hb;
			hb.down = posyn;
			hb.left = posxn;
			hb.right = posxn + 2;
			hb.up = posyn + 3;
			int i = 0;
			bool colide = false;
			for (auto& ch : world->chunks) {
				if (world->loaded[i]) {
					for (auto& hb2 : ch->boxes) {
						if (rrColide(&hb, &hb2)) {
							//printf("stuck");
							colide = true;
							break;
						}
					}
					if (colide) break;
				}
				i++;

			}
			if (!colide) {
				posx = posxn;
				posy = posyn;
			}
		}
		printf("x: %i, y: %i", posx, posy);
	}

};

class ourGame : public olc::PixelGameEngine {
public:
	bool stop = false;
	std::vector<olc::Sprite*> tiles;
	std::vector<Item*> items;
	float aspectRatio;
	World world;
	Player player;
	float xmax = 0;
	ourGame() : olc::PixelGameEngine(), world(0), aspectRatio(1), tiles(), player(20, 150, "assets/entities/player.png"){
		tiles.push_back(new olc::Sprite("assets/blocks/air.png"));
		tiles.push_back(new olc::Sprite("assets/blocks/stone.png"));
		tiles.push_back(new olc::Sprite("assets/blocks/dirt.png"));
		tiles.push_back(new olc::Sprite("assets/blocks/iron.png"));

		items.push_back(new Item("assets/blocks/w.png", 0, 0));//air
		items.push_back(new Item("assets/blocks/stone.png", 1, 0));//stone
		items.push_back(new Item("assets/blocks/dirt.png", 2, 0));//dirt
		items.push_back(new Item("assets/items/woodenSword.png", 0, 5));//air

		player.inv.none = items[0];

		player.inv.slots[0].first = items[0];
		player.inv.slots[0].second = 0;
		player.inv.slots[1].first = items[0];
		player.inv.slots[1].second = 0;
		player.inv.slots[2].first = items[0];
		player.inv.slots[2].second = 0;
		player.inv.slots[3].first = items[0];
		player.inv.slots[3].second = 0;
		player.inv.slots[4].first = items[0];
		player.inv.slots[4].second = 0;
		player.inv.slots[5].first = items[0];
		player.inv.slots[5].second = 0;
		player.inv.slots[6].first = items[0];
		player.inv.slots[6].second = 0;
		player.inv.slots[7].first = items[0];
		player.inv.slots[7].second = 0;
		player.inv.slots[8].first = items[0];
		player.inv.slots[8].second = 0;
	}
	bool OnUserCreate() {
		aspectRatio = ScreenWidth() / (float)ScreenHeight();
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
				float tilePosX = x * drawW / ScreenWidth() + centerW - xoff - drawW / 2;
				float tilePosY = y * drawH / ScreenHeight() + centerH - yoff - drawH / 2;
				int spritePX = (tilePosX - (int)tilePosX) * tile->width;
				int spritePY = (tilePosY - (int)tilePosY) * tile->height;
				Draw(x, y, tile->GetPixel(spritePX, spritePY));
			}
		}
	}

	bool OnUserUpdate(float fElapsedTime) {
		int screenTileW = 50;
		if (player.posx > xmax) xmax = player.posx;
		player.moveDown = GetKey(olc::Key::DOWN).bHeld || GetKey(olc::Key::S).bHeld;
		player.moveUp = GetKey(olc::Key::UP).bHeld || GetKey(olc::Key::W).bHeld || GetKey(olc::Key::SPACE).bHeld;
		player.moveLeft = GetKey(olc::Key::LEFT).bHeld || GetKey(olc::Key::A).bHeld;
		player.moveRight = GetKey(olc::Key::RIGHT).bHeld || GetKey(olc::Key::D).bHeld;
		if (GetKey(olc::Key::K1).bHeld) player.inv.activeSlot = 0;
		if (GetKey(olc::Key::K2).bHeld) player.inv.activeSlot = 1;
		if (GetKey(olc::Key::K3).bHeld) player.inv.activeSlot = 2;
		if (GetKey(olc::Key::K4).bHeld) player.inv.activeSlot = 3;
		if (GetKey(olc::Key::K5).bHeld) player.inv.activeSlot = 4;
		if (GetKey(olc::Key::K6).bHeld) player.inv.activeSlot = 5;
		if (GetKey(olc::Key::K7).bHeld) player.inv.activeSlot = 6;
		if (GetKey(olc::Key::K8).bHeld) player.inv.activeSlot = 7;
		if (GetKey(olc::Key::K9).bHeld) player.inv.activeSlot = 8;


		printf("update\n");
		player.update(&world, fElapsedTime);

		world.load(xmax, screenTileW);
		for (int i = 0; i < world.chunks.size(); i++) {
			if (world.loaded[i]) {
				drawTileMap(world.chunks[i]->data, Chunk::width, Chunk::height, 0, 0, screenTileW, screenTileW / aspectRatio, i*Chunk::width - xmax, - player.posy, false);

			}
		}
		drawTile(&player.sprite, 0, 0, screenTileW/2, screenTileW / aspectRatio/3, (player.posx - xmax)/2, -2/3.0);

		for (int i = 0; i < player.inv.slots.size(); i++) {
			if (i == player.inv.activeSlot) {
				drawTile(player.inv.icosel, 0, 0, 13, 13 / aspectRatio, i - 4.5f, 2.5);
			}
			else {
				drawTile(player.inv.ico, 0, 0, 13, 13 / aspectRatio, i-4.5f,2.5);
			}
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
	int sx = 500;
	int sy = 300;
	int ps = 2;
	printf("Welcome to The Quest Must go on.");
	printf("we hope you enjoy playing our game.");
	printf("To start please select a screen width(this game uses per pixel software rendering and performance is abismal at even moderate resolution): ");
	scanf_s("%i", &sx);
	printf("Please select a screen height: ");
	scanf_s("%i", &sy);
	printf("please select a pixelSize(increses the number of pixels taken up by each rendered pixel, helps se the game without a magnifier of low frame rate): ");
	scanf_s("%i", &ps);
	ourGame game = ourGame();
	if (game.Construct(sx, sy, ps, ps)) {
		game.Start();
	}
	return 0;
}