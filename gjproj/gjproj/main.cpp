#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "open-simplex-noise.h"

#define simplexScale 30
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
			if (x % 4 == 0 && (open_simplex_noise2(osCont, (double)(x + (chunk * width)) / 400, 400))>0.05) {
				for (int i = 0; i <4+ (rand() % 12); i++) {
					data[(i+h + dirth) * width + x] = 3;
				}
			}
		}
		caclcols(chunk);
		
	}
	void caclcols(int chunk) {
		boxes.clear();

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++)
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
	void setTileAt(int x, int y, int type) {
		if (x >= 0 && x / Chunk::width <= width && loaded[x / Chunk::width] && y < Chunk::height && y >= 0) {
			chunks[x / Chunk::width]->data[y * Chunk::width + x % Chunk::width]=type;
			chunks[x / Chunk::width]->caclcols(x / Chunk::width);
		}
	}
};

class Item {
public:
	olc::Sprite icon;
	float width = 2;
	float height = 2;
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
	bool add(Item* itemType, int ammount){
		bool added = false;
		for (int i = 0; i < slots.size(); i++) {
			//printf("%X, %X\n", itemType, slots[i].first);
			if (itemType == slots[i].first) {
				slots[i].second += ammount;
				added = true;
				return true;
			}
		}
		for (int i = 0; i < slots.size(); i++) {
			if (none == slots[i].first) {
				slots[i].first = itemType;
				slots[i].second = ammount;
				added = true;
				break;
			}
		}
		return added;
	}
	Item* remove(int count) {
		if (slots[activeSlot].second < count) return none;
		slots[activeSlot].second -= count;
		Item* i = slots[activeSlot].first;
		if (slots[activeSlot].second == 0)slots[activeSlot].first = none;
		return i;
	}
	Item* remove(int count, Item* item) {
		int slot = 0;
		for (int i = 0; i < slots.size(); i++) {
			if (slots[i].first == item) {
				slot = i;
			}
		}
		if (slots[slot].second < count) return none;
		slots[slot].second -= count;
		Item* i = slots[slot].first;
		if (slots[slot].second == 0)slots[slot].first = none;
		return i;
	}
	Item* get() {
		return slots[activeSlot].first;
	}
	int find(Item* i) {
		for (auto& slot : slots) {
			if (slot.first == i) {
				return slot.second;
			}
		}
		return 0;
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
		if ((int)posxn / Chunk::width-1 < 0 || !world->loaded[(int)posxn / Chunk::width-1]) {
			return;
		}
		Rect hb;
		hb.down = posyn;
		hb.left = posxn;
		hb.right = posxn + 1.8;
		hb.up = posyn + 2.6;
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
			hb.right = posxn + 1.8;
			hb.up = posyn + 2.6;
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
		//printf("x: %i, y: %i", posx, posy);
	}

};

class Crafter {
public:
	int selectedCraft;
	std::vector<int> craftable;
	std::vector<std::pair<Item*, std::vector<std::pair<Item*, int>>*>> recipies;
	std::vector<Item*>* itemVector;
	Crafter(): craftable() {}
	Crafter(std::vector<Item*>* itemVector): craftable(){
		update(itemVector);
	}
	void update(std::vector<Item*>* itemVector) {
		this->itemVector = itemVector;
		std::vector<std::pair<Item*, int>>* i = new std::vector<std::pair<Item*, int>>();
		std::pair<Item*, int> p;
		std::pair<Item*, std::vector<std::pair<Item*, int>>*> pp;

		p.first = itemVector->at(3);
		p.second = 20;
		i->push_back(p);
		pp.first = itemVector->at(4);
		pp.second = i;
		recipies.push_back(pp);

		i = new std::vector<std::pair<Item*, int>>();
		p.first = itemVector->at(1);
		p.second = 20;
		i->push_back(p);
		p.first = itemVector->at(4);
		p.second = 1;
		i->push_back(p);
		pp.first = itemVector->at(5);
		pp.second = i;
		recipies.push_back(pp);
	}
	void getCraftable(Inventory* inv) {
		craftable.clear();
		int n = 0;
		for (auto& recipie : recipies) {
			bool bcraftable = true;
			for (int i = 0; i < recipie.second->size(); i++) {
				if (inv->find(recipie.second->at(i).first) < recipie.second->at(i).second) {
					bcraftable = false;
				}
			}
			if (bcraftable) {
				craftable.push_back(n);
			}
			n++;
		}
		if (craftable.size() == 0) {
			selectedCraft = 0;
		}
		else if (craftable.size() >= selectedCraft) {
			selectedCraft = craftable.size() - 1;
		}
	}
	void craft(Inventory* inv) {
		if (selectedCraft < craftable.size()) {
			inv->add(recipies.at(craftable[selectedCraft]).first, 1);
			for (int i = 0; i < recipies.at(craftable[selectedCraft]).second->size(); i++) {
				inv->remove(recipies.at(craftable[selectedCraft]).second->at(i).second, recipies.at(craftable[selectedCraft]).second->at(i).first);
			}
		}
		getCraftable(inv);
	}
	void addselcraft() {
		if (craftable.size() == 0) {
			selectedCraft = 0;
		}
		selectedCraft = (selectedCraft + 1) % craftable.size();
	}
};

class mob {
public:
	olc::Sprite* tex;
	float xpos;
	float ypos;
	int health;

	virtual void update(void* og, float etime) {}

};

class ghost: public mob{
public:
	float damageCoolDown;
	ghost(float x, float y) {
		xpos = x;
		ypos = y;
		health = 5;
		tex = new olc::Sprite("assets/entities/ghost.png");
	}

	void update(void* og, float etime);
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
	olc::Sprite* heart;
	Crafter crafty;
	std::vector<mob*> mobs;
	ourGame() : olc::PixelGameEngine(), world(0), aspectRatio(1), tiles(), player(20, 150, "assets/entities/player.png"){
		tiles.push_back(new olc::Sprite("assets/blocks/air.png"));
		tiles.push_back(new olc::Sprite("assets/blocks/stone.png"));
		tiles.push_back(new olc::Sprite("assets/blocks/dirt.png"));
		tiles.push_back(new olc::Sprite("assets/blocks/wood.png"));

		items.push_back(new Item("assets/blocks/w.png", 0, 0));//air
		items.push_back(new Item("assets/blocks/stone.png", 1, 0));//stone
		items.push_back(new Item("assets/blocks/dirt.png", 2, 0));//dirt
		items.push_back(new Item("assets/blocks/wood.png", 3, 0));//air
		items.push_back(new Item("assets/ui/woodSword.png", 0, 1));//air
		items.push_back(new Item("assets/ui/stoneSword.png", 0, 2));//air
		items.push_back(new Item("assets/ui/ironSword.png", 0, 3));//air

		heart = new olc::Sprite("assets/ui/heart.png");

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

		SetPixelMode(olc::Pixel::Mode::ALPHA);

		crafty.update(&items);
	}
	bool OnUserCreate() {
		mobs.push_back(new ghost(40, 140));
		aspectRatio = ScreenWidth() / (float)ScreenHeight();
		//world = World(0);
		world.load(player.posx, 40);
		return true;
	}

	void screenToBlock(float centerW, float centerH, float drawW, float drawH, float xoff, float yoff, int sx, int sy, float* tx, float* ty) {
		*tx = sx * drawW / ScreenWidth() + centerW - xoff - drawW / 2;
		*ty = sy * drawH / ScreenHeight() + centerH - yoff - drawH / 2;
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

		if (GetMouse(0).bHeld) {
			//printf("m0\n");
			float cpx;
			float cpy;
			screenToBlock(0, 0, screenTileW, screenTileW / aspectRatio, -xmax, player.posy, GetMouseX(), GetMouseY(), &cpx, &cpy);
			//cpy = Chunk::height - cpy;
			//printf("m0, %f, %f\n", cpx,cpy);
			if (world.tileAt(cpx, -cpy) != 0 || world.tileAt(cpx, -cpy) != -1) {
				player.inv.add(items[world.tileAt(cpx, -cpy)], 1);
				world.setTileAt(cpx, -cpy, 0);
				crafty.getCraftable(&player.inv);
			}
		}

		if (GetMouse(1).bHeld) {
			//printf("m0\n");
			float cpx;
			float cpy;
			screenToBlock(0, 0, screenTileW, screenTileW / aspectRatio, -xmax, player.posy, GetMouseX(), GetMouseY(), &cpx, &cpy);
			//cpy = Chunk::height - cpy;
			//printf("m0, %f, %f\n", cpx,cpy);
			if (world.tileAt(cpx, -cpy) == 0 && player.inv.get()->placeType != 0) {
				world.setTileAt(cpx, -cpy, player.inv.remove(1)->placeType);
				crafty.getCraftable(&player.inv);
			}
		}

		if (GetKey(olc::ENTER).bPressed) {
			crafty.craft(&player.inv);
		}
		if (GetKey(olc::Key::Z).bPressed) {
			crafty.addselcraft();
		}
		//printf("update\n");
		player.update(&world, fElapsedTime);

		for (int i = 0; i < mobs.size(); i++) {
			mobs[i]->update(this, fElapsedTime);
		}

		world.load(xmax, screenTileW);
		for (int i = 0; i < world.chunks.size(); i++) {
			if (world.loaded[i]) {
				drawTileMap(world.chunks[i]->data, Chunk::width, Chunk::height, 0, 0, screenTileW, screenTileW / aspectRatio, i*Chunk::width - xmax, - player.posy, false);

			}
		}
		drawTile(&player.sprite, 0, 0, screenTileW/2, screenTileW / aspectRatio/3, (player.posx - xmax)/2, -2/3.0);

		for (int i = 0; i < mobs.size(); i++) {
			drawTile(mobs[i]->tex, 0, 0, screenTileW / 2, screenTileW / aspectRatio / 2, (player.posx - xmax) / 2, -2 / 2.0);
		}

		olc:Item* tmpItem;

		tmpItem = player.inv.slots[player.inv.activeSlot].first;

		if (tmpItem != items[0]) {

			drawTile(&tmpItem->icon, 0, 0, screenTileW / tmpItem->width, screenTileW / aspectRatio / tmpItem->height, (player.posx - xmax + 2) / tmpItem->width, -2.0 / (float)tmpItem->height);

		}
		for (int i = 0; i < player.inv.slots.size(); i++) {
			if (i == player.inv.activeSlot) {
				drawTile(player.inv.icosel, 0, 0, 13, 13 / aspectRatio, i - 4.5f, 2.5);
			}
			else {
				drawTile(player.inv.ico, 0, 0, 13, 13 / aspectRatio, i-4.5f,2.5);
			}
			drawTile(&player.inv.slots[i].first->icon, 0, 0, 19, 19 / aspectRatio, (i - 4.3) * 19.0f / 13, 2.7f * 19.0f / 13);
		}

		for (int i = 0; i < player.health; i++) {
				drawTile(heart, 0, 0, 13, 13 / aspectRatio, i - 2.5f, -3.5);
		}

		for (int i = 0; i < crafty.craftable.size(); i++) {
			//printf("%i\n", crafty.craftable[i]);
			//printf("%X, %X", )
			if (i == crafty.selectedCraft) {
				drawTile(player.inv.icosel, 0, 0, 13, 13 / aspectRatio, -6, i - 3.5);
			}
			else {
				drawTile(player.inv.ico, 0, 0, 13, 13 / aspectRatio, -6, i - 3.5);
			}
			drawTile(&crafty.recipies.at(crafty.craftable[i]).first->icon, 0, 0, 13, 13 / aspectRatio, -6, i-3.5);
		}

		return true;
	}

	bool OnUserDestroy() {
		for (int i = 0; i < tiles.size(); i++) {
			delete tiles[i];
		}

		for (int i = 0; i < items.size(); i++) {
			delete items[i];
		}
		delete heart;

		return true;
	}



};

void ghost::update(void* og, float etime) {
	float px = ((ourGame*)og)->player.posx + 0.9;
	float py = ((ourGame*)og)->player.posy + 1.3;
	float dx = px - xpos;
	float dy = py - ypos;
	float scale = sqrtf(dx * dx + dy * dy);
	dx /= scale;
	dy /= scale;
	xpos += 10 * etime * dx;
	ypos += 10 * etime * dy;
	if (scale < 1) {
		((ourGame*)og)->player.health -= 1;
	}
}

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