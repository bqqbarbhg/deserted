#if 0
#include "postprocess.h"
#include "sandbox.h"

#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "../ext/stb_image_write.h"

sb::Pass *test, *blur, *edges, *copy, *testcoh, *accumulate;
pp::Texture *tex, *tex2;
pp::Texture *flip[4];
pp::Texture *acc[3];
int flipIx = 0;

extern GLFWwindow *window;

void setup(pp::Context *ctx, sb::Sandbox *sb)
{
	test = sb->loadPass("shader/test_vertex.glsl", "shader/chroma_coherency.glsl");
	blur = sb->loadPass("shader/test_vertex.glsl", "shader/smart_blur.glsl");
	edges = sb->loadPass("shader/test_vertex.glsl", "shader/find_smooth.glsl");
	testcoh = sb->loadPass("shader/test_vertex.glsl", "shader/test_coherent.glsl");
	copy = sb->loadPass("shader/test_vertex.glsl", "shader/copy.glsl");
	accumulate = sb->loadPass("shader/test_vertex.glsl", "shader/accumulate.glsl");
	tex = sb->loadImage("image/test2.jpg");
	for (int i = 0; i < 4; i++)
		flip[i] = ctx->createRenderTarget(1024, 1024);
	for (int i = 0; i < 3; i++)
		acc[i] = ctx->createRenderTarget(1024, 1024);
}

struct Col
{
	union {
		struct {
			uint8_t r, g, b, a;
		};
		struct {
			uint8_t c[4];
		};
		struct {
			uint32_t hex;
		};
	};
};

struct ColOrder
{
	Col col;
	uint32_t num;
};

bool operator<(const ColOrder& a, const ColOrder& b)
{
	return a.num > b.num;
}

int absInt(int x)
{
	return x >= 0 ? x : -x;
}

int colDiff(Col a, Col b)
{
	return absInt((int)a.r - (int)b.r) + absInt((int)a.g - (int)b.g) + absInt((int)a.b - (int)b.b);
}

struct Cell
{
	std::vector<uint8_t> groups;
	std::vector<uint32_t> globalGroups;

	std::vector<uint8_t> edges[4];
};

struct CellMap
{
	Cell *cells;
	uint32_t width;
	uint32_t height;
	uint32_t cellWidth;
	uint32_t cellHeight;

	uint32_t globalGroupCounter;
};

int correlateCells(Cell &a, Cell &b, int dir, uint32_t group)
{
	auto &edgeA = a.edges[dir];
	auto &edgeB = b.edges[(dir + 2) % 4];

	int counts[256] = { 0 };

	int maxCount = 0;
	uint8_t maxGroup = 0;

	int selfCount = 0;
	for (uint32_t i = 0; i < edgeA.size(); i++)
	{
		if (edgeA[i] == group)
		{
			uint8_t g = edgeB[i];
			int count = ++counts[g];
			selfCount++;

			if (count > maxCount)
			{
				maxCount = count;
				maxGroup = g;
			}
		}
	}

	int otherCount = 0;
	for (auto c : edgeB)
	{
		if (c == maxGroup)
			otherCount++;
	}

	if (maxCount - selfCount > 2 || otherCount - selfCount > 2)
		return -1;

	return maxGroup;
}

bool getCell(Cell *&c, CellMap& map, uint32_t x, uint32_t y)
{
	if (x < map.width && y < map.height)
	{
		Cell *cc = &map.cells[x + y * map.width];
		if (cc->groups.empty())
			return false;

		c = cc;
		return true;
	}
	else
		return false;
}

struct Point
{
	int x, y, localGroup;
};

struct TotalColor
{
	uint64_t r = 0, g = 0, b = 0;
	uint64_t count = 0;
};

void cellFloodFill(CellMap& map, uint32_t startX, uint32_t startY, uint32_t globalGroup, int startLocalGroup)
{
	std::vector<Point> queue;
	Point startP;
	startP.x = startX;
	startP.y = startY;
	startP.localGroup = startLocalGroup;
	queue.push_back(startP);

	int dx[] = { 1, 0, -1, 0 };
	int dy[] = { 0, 1, 0, -1 };

	while (!queue.empty())
	{
		Point p = queue.back();
		queue.pop_back();

		Cell *cell;
		if (!getCell(cell, map, p.x, p.y))
			return;

		for (int dir = 0; dir < 4; dir++)
		{
			Cell *other;
			if (!getCell(other, map, p.x + dx[dir], p.y + dy[dir]))
				continue;

			int otherGroup = correlateCells(*cell, *other, dir, p.localGroup);
			if (otherGroup < 0)
				continue;

			if (other->globalGroups[otherGroup] != 0)
				continue;

			other->globalGroups[otherGroup] = globalGroup;

			Point n;
			n.x = p.x + dx[dir];
			n.y = p.y + dy[dir];
			n.localGroup = otherGroup;
			queue.push_back(n);
		}
	}
}

void cellFloodFillStart(CellMap& map, uint32_t x, uint32_t y)
{
	Cell *cell;
	if (!getCell(cell, map, x, y))
		return;

	for (int i = 0; i < cell->groups.size(); i++)
	{
		int g = cell->groups[i];
		if (cell->globalGroups[g] == 0)
		{
			uint32_t group = ++map.globalGroupCounter;
			cell->globalGroups[g] = group;
			cellFloodFill(map, x, y, group, g);
		}
	}
}

void assignGlobalGroups(CellMap& map)
{
	for (uint32_t y = 0; y < map.height; y++)
	{
		for (uint32_t x = 0; x < map.height; x++)
		{
			cellFloodFillStart(map, x, y);
		}
	}
}

void loop(pp::Context *ctx, sb::Sandbox *sb)
{
	int rex = flipIx;
	flipIx = flipIx ^ 1;
	int wix = flipIx;

	ctx->setTexture("tex", tex);

	ctx->clear(flip[0], 0.0f, 0.0f, 0.0f, 0.0f);
	sb->renderPass(edges, flip[0]);

	static int srcI;

	static int prevPress = 0;
	if (glfwGetMouseButton(window, 0))
	{
		if (!prevPress)
		{
			srcI ^= 1;
			prevPress = 1;
		}
	}
	else
		prevPress = 0;

	float flipVec[1];
	flipVec[0] = srcI ? 0.0f : 0.0f;
	ctx->setVector("flip", flipVec, 1);

	ctx->setTexture("tex", flip[0]);

	ctx->clear(flip[1], 0.0f, 0.0f, 0.0f, 1.0f);
	sb->renderPass(test, flip[1]);

	ctx->setTexture("tex", flip[1]);
	ctx->clear(flip[2], 0.0f, 0.0f, 0.0f, 1.0f);
	sb->renderPass(copy, flip[2]);

	ctx->setTexture("tex", flip[2]);
	ctx->clear(flip[3], 0.0f, 0.0f, 0.0f, 1.0f);
	sb->renderPass(testcoh, flip[3]);

#if 0
	ctx->setTexture("tex", flip[3]);
	ctx->setTexture("old", acc[rex]);
	ctx->clear(acc[wix], 0.0f, 0.0f, 0.0f, 1.0f);
	sb->renderPass(accumulate, acc[wix]);

	ctx->setTexture("tex", acc[wix]);
	ctx->clear(acc[2], 0.0f, 0.0f, 0.0f, 1.0f);
	sb->renderPass(copy, acc[2]);
#endif

	uint8_t *pixelData = new uint8_t[flip[1]->getWidth() * flip[1]->getHeight() * 4];
	ctx->readPixels(flip[3], pixelData);

	std::unordered_map<uint32_t, int> colCount;
	std::vector<ColOrder> colOrder;

	uint32_t numCells = 16;
	uint32_t cellSize = 1024 / 16;

	std::vector<Cell> cells;
	cells.resize(numCells * numCells);

	for (uint32_t cy = 0; cy < numCells; cy++)
	{
		for (uint32_t cx = 0; cx < numCells; cx++)
		{
			uint32_t bx = cx * cellSize;
			uint32_t by = cy * cellSize;
			uint32_t ex = bx + cellSize;
			uint32_t ey = by + cellSize;

			std::vector<Col> cols;

			for (uint32_t y = by; y < by + cellSize; y++)
			{
				for (uint32_t x = bx; x < bx + cellSize; x++)
				{
					uint8_t *p = &pixelData[(y * 1024 + x) * 4];
					Col c;
					c.r = p[0];
					c.g = p[1];
					c.b = p[2];
					c.a = p[3];
					cols.push_back(c);
				}
			}

			colCount.clear();
			for (Col c : cols)
				colCount[c.hex]++;

			Cell& cell = cells[cx + cy * numCells];
			if (colCount.size() <= 8)
			{
				cell.globalGroups.resize(colCount.size(), 0);
				cell.groups.resize(cellSize * cellSize);

				for (uint32_t y = by; y < by + cellSize; y++)
				{
					for (uint32_t x = bx; x < bx + cellSize; x++)
					{
						uint8_t *p = &pixelData[(y * 1024 + x) * 4];
						Col c;
						c.r = p[0];
						c.g = p[1];
						c.b = p[2];
						c.a = p[3];

						auto it = colCount.find(c.hex);
						uint32_t ix = std::distance(colCount.begin(), it);
						cell.groups[(x - bx) + (y - by) * cellSize] = ix;
					}
				}


				for (int dir = 0; dir < 4; dir++)
					cell.edges[dir].resize(cellSize);

				for (int i = 0; i < cellSize; i++)
				{
					cell.edges[0][i] = cell.groups[cellSize - 1 + i * cellSize];
					cell.edges[1][i] = cell.groups[i + (cellSize - 1) * cellSize];
					cell.edges[2][i] = cell.groups[0 + i * cellSize];
					cell.edges[3][i] = cell.groups[i + 0];
				}
			}
		}
	}


	CellMap cm;
	cm.cells = cells.data();
	cm.cellWidth = cellSize;
	cm.cellHeight = cellSize;
	cm.width = numCells;
	cm.height = numCells;
	cm.globalGroupCounter = 0;

	assignGlobalGroups(cm);

	std::vector<TotalColor> groupColors;
	groupColors.resize(cm.globalGroupCounter + 1);

	for (uint32_t cy = 0; cy < numCells; cy++)
	{
		for (uint32_t cx = 0; cx < numCells; cx++)
		{
			uint32_t bx = cx * cellSize;
			uint32_t by = cy * cellSize;
			Cell& cell = cells[cx + cy * numCells];

			if (!cell.groups.empty())
			{
				for (uint32_t y = by; y < by + cellSize; y++)
				{
					for (uint32_t x = bx; x < bx + cellSize; x++)
					{
						uint8_t *p = &pixelData[(y * 1024 + x) * 4];
						uint32_t ix = cell.globalGroups[cell.groups[(x - bx) + (y - by) * cellSize]];
						Col c;
						c.r = p[0];
						c.g = p[1];
						c.b = p[2];
						c.a = p[3];

						TotalColor &tc = groupColors[ix];
						tc.r += c.r;
						tc.g += c.g;
						tc.b += c.b;
						tc.count++;
					}
				}
			}
		}
	}

	std::vector<Col> groupCol;
	for (auto d : groupColors)
	{
		Col c; c.hex = 0;
		if (d.count > 0)
		{
			c.r = d.r / d.count;
			c.g = d.g / d.count;
			c.b = d.b / d.count;
			c.a = 255;
		}
		groupCol.push_back(c);
	}

	for (uint32_t cy = 0; cy < numCells; cy++)
	{
		for (uint32_t cx = 0; cx < numCells; cx++)
		{
			uint32_t bx = cx * cellSize;
			uint32_t by = cy * cellSize;
			Cell& cell = cells[cx + cy * numCells];

			if (!cell.groups.empty())
			{
				for (uint32_t y = by; y < by + cellSize; y++)
				{
					for (uint32_t x = bx; x < bx + cellSize; x++)
					{
						uint8_t *p = &pixelData[(y * 1024 + x) * 4];
						uint32_t ix = cell.globalGroups[cell.groups[(x - bx) + (y - by) * cellSize]];
						Col c = groupCol[ix];

						p[0] = c.r;
						p[1] = c.g;
						p[2] = c.b;
						p[3] = c.a;
					}
				}
			}
		}
	}

	if (glfwGetMouseButton(window, 1))
	{
		stbi_write_png("components.png", 1024, 1024, 4, pixelData, 0);
	}

	pp::Texture *rec = ctx->createImage(pixelData, 1024, 1024);
	delete[] pixelData;



	ctx->setTexture("tex", srcI ? rec : flip[3]);
	ctx->clear(NULL, 0.0f, 0.0f, 0.0f, 1.0f);
	sb->renderPass(copy, NULL);
ggj
	ctx->destroyTexture(rec);
}

#endif
