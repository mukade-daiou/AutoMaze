# include <Siv3D.hpp>
const Array<Vec2>dir{ Vec2{-1,0},Vec2{0,-1},Vec2{1,0},Vec2{0,1} };//lurd
struct Player {
	Texture texture{ U"🀄"_emoji };
	Vec2 pos,from,to;
	bool autoMove = false, debug = false;
	int size;
	double speed = 0.5;
	Stopwatch stopwatch;
	Array<Vec2>route;
	Player(Vec2 _pos, int _size) :pos(_pos), size(_size) {}
	void draw() {
		texture.resized(size).drawAt(pos);
		if(debug&&!route.empty())
			for (int i = 0; i < route.size() - 1; ++i)
				Line{route[i],route[i+1]}.draw(3,Palette::Red);
	}
	void Move(const Array<Line>lines) {
		if (!autoMove) {
			const double delta = (Scene::DeltaTime() * 200);
			if (KeyLeft.pressed() && canMove(lines, 0))
				pos.x -= delta*speed;
			if (KeyUp.pressed() && canMove(lines, 1))
				pos.y -= delta*speed;
			if (KeyRight.pressed() && canMove(lines, 2))
				pos.x += delta*speed;
			if (KeyDown.pressed() && canMove(lines, 3))
				pos.y += delta*speed;
		}
		else {
			const double t = Min(stopwatch.sF()*speed, 1.0);
			pos = from.lerp(to, t);
			if (t == 1.0&&!route.empty()) {
				from = pos;
				to = route.front();
				route.pop_front();
				stopwatch.restart();
			}
		}
	}
	void switchAuto(Array<Vec2>_route) {
		if (autoMove) {
			speed = 5.0;
			route = _route;
			from = pos;
			to = route.front();
			route.pop_front();
			stopwatch.restart();
		}
		else {
			speed = 0.5;
			stopwatch.pause();
			route.clear();
		}
	}
	bool canMove(const Array<Line>lines,int d) {
		bool res = true;
		const double delta = (Scene::DeltaTime() * 200);
		RectF region = RectF{ Arg::center(pos+dir[d]*delta*speed),(double)size};
		for (auto& line : lines)
			if (line.intersects(region))
				res = false;
		return res;
	}
};
struct Field {
	const Vec2 SIZE{ 800,600 }, StartPos{30,50};
	const Texture goalTexture{ U"🚩"_emoji };
	HashTable<Vec2, Array<Vec2>>graph;
	int size;
	Array<Line>lines{
		Line{StartPos,StartPos + Vec2{SIZE.x,0}},
		Line{StartPos,StartPos + Vec2{0,SIZE.y}},
		Line{StartPos + Vec2{0,SIZE.y},StartPos + SIZE},
		Line{StartPos + Vec2{SIZE.x,0},StartPos+SIZE}
	};
	Field() {}
	bool addLine(Vec2 p1,Vec2 p2) {
		Vec2 from, to;
		if (p1.x < p2.x || p1.y < p2.y)
			from = p1, to = p2;
		else
			from = p2, to = p1;
		if (lines.includes(Line{ from,to }))
			return false;
		else {
			lines << Line{ from,to };
			return true;
		}
	}
	void createMaze(int _size=20,int mode=1) {
		size = _size;
		for(int y=size;y<SIZE.y;y+=size)
			for (int x = size; x < SIZE.x; x += size) {
				Vec2 pos=Vec2(x,y)+StartPos;
				if (y == size) {
					while (true) {
						Vec2 newPos=pos+dir.choice()*size;
						if (addLine(pos, newPos))
							break;
					}
				}
				else {
					while (true) {
						Vec2 d = dir.choice();
						if (d == Vec2(0, -1))
							continue;
						if (addLine(pos, pos+d*size))
							break;
					}
				}
		}
		createGraph();
	}
	void createGraph() {
		for(int y=size;y<=SIZE.y;y+=size)
			for (int x = size; x <= SIZE.x; x += size) {
				Vec2 pos = Vec2(x, y) + StartPos;
				Vec2 vertex = pos - Vec2{ size / 2,size / 2 };
				//r
				if (x < SIZE.x && !lines.includes(Line{ pos + dir[1] * size,pos })) {
					if (!graph.contains(vertex))
						graph.emplace(vertex, Array<Vec2>());
					if(!graph.contains(vertex+dir[2]*size))
						graph.emplace(vertex+dir[2]*size, Array<Vec2>());
					graph[vertex] << vertex + dir[2] * size;
					graph[vertex + dir[2] * size] << vertex;
				}
				//d
				if (y < SIZE.y  && !lines.includes(Line{pos+dir[0]*size,pos})) {
					if (!graph.contains(vertex))
						graph.emplace(vertex, Array<Vec2>());
					if (!graph.contains(vertex + dir[3] * size))
						graph.emplace(vertex + dir[3] * size, Array<Vec2>());
					graph[vertex] << vertex + dir[3] * size;
					graph[vertex + dir[3] * size] << vertex;
				}
			}
	}
	void draw() {
		for (auto& line : lines) {
			line.draw(2,Palette::Skyblue);
			goalTexture.resized(size*0.7).draw(StartPos+SIZE-Vec2(size,size)*0.8);
		}
		/*if (debug)
			for (auto [key, value] : graph)
				for (auto v : value)
					Line(key, v).draw(Palette::Yellow);
					*/

	}
	Array<Vec2> BFS(Vec2 _pos) {
		Vec2 pos =_pos-StartPos;
		pos.x = (int)(pos.x / size) * size + size / 2;
		pos.y = (int)(pos.y / size) * size + size / 2;
		pos += StartPos;
		Array<Vec2> queue{ pos }, route, visited{ pos };
		HashTable<Vec2, Vec2>prevVec2;
		while (!queue.empty()) {
			Vec2 p = queue.front();
			queue.pop_front();
			for (auto& v : graph[p]) {
				if (visited.includes(v))continue;
				visited << v;
				prevVec2.emplace(v,p);
				queue << v;
			}
		}
		Vec2 nowpos = StartPos + SIZE - Vec2{ size / 2,size / 2 };
		while (nowpos != pos) {
			route.push_front(nowpos);
			nowpos = prevVec2[nowpos];
		}
		return route;
	}
};
void Main()
{
	Scene::SetResizeMode(ResizeMode::Keep);
	Scene::Resize(1000, 700);
	Window::Resize(1000, 700);
	Field field{};
	field.createMaze();
	Player pl{ field.StartPos + Vec2 {field.size/2,field.size/2},static_cast<int>(field.size*0.7)};
	while (System::Update())
	{
		if (SimpleGUI::CheckBox(pl.autoMove, U"AUTO", Vec2{ 850,150 }))
			pl.switchAuto(field.BFS(pl.pos));
		SimpleGUI::CheckBox(pl.debug, U"DEBUG", Vec2{ 850,250 });
		pl.Move(field.lines);
		field.draw();
		pl.draw();
	}
}