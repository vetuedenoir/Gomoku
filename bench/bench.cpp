#include "ai/MasterAI.hpp"
#include "ai/SearchPosition.hpp"
#include "game/board/GameBoard.hpp"
#include "logger/Logger.hpp"
#include "config/config.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int kBoardSize = 19;

struct BenchStats
{
	int nodesVisited        = 0;
	int nodesEvaluated      = 0;
	int nodesPruned         = 0;
	int maxDepthSeen        = 0;
	int ttHits              = 0;
	int ttCutoffs           = 0;
	int ttStores            = 0;
	int ttOrderingHits      = 0;
	int ttRootHits          = 0;
	int ttRootOrderingHits  = 0;
	int ttRootExactSeeds    = 0;
	int forcedNodes         = 0;
};

struct BenchResult
{
	std::string name;
	t_cell      bestMove  = {-1, -1};
	int         bestScore = 0;
	int64_t     timeUs    = 0;
	BenchStats  stats;
};

struct BenchPosition
{
	std::string name;
	Color       toMove         = Color::Black;
	int         capturesBlack  = 0;
	int         capturesWhite  = 0;
	GameBoard   board{kBoardSize, Color::Black};
};

struct RunReport
{
	int                      depth = 0;
	std::vector<BenchResult> positions;
};

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------

void usage(const char* argv0)
{
	std::cerr
		<< "Usage:\n"
		<< "  " << argv0 << " --depth N --positions FILE --out FILE\n"
		<< "  " << argv0 << " --compare REF.json NEW.json\n"
		<< "\n"
		<< "Fixed-depth search, no time limit. Each position is written with\n"
		<< "bestMove, bestScore, SearchStats, and elapsed microseconds.\n";
}

bool flagEq(const char* a, const char* b)
{
	return std::string(a) == b;
}

// ---------------------------------------------------------------------------
// Positions file
//
//   # comments and blank lines ignored
//   name <id>
//   toMove B|W
//   captures <byBlack> <byWhite>     # optional
//   then either 19-char ASCII rows (. B W) and/or stone lines:
//     B <x> <y>
//     W <x> <y>
// ---------------------------------------------------------------------------

std::string trim(const std::string& s)
{
	size_t b = 0;
	while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
		++b;
	size_t e = s.size();
	while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
		--e;
	return s.substr(b, e - b);
}

bool isAsciiRow(const std::string& s)
{
	if (s.empty() || s.size() > static_cast<size_t>(kBoardSize))
		return false;
	for (char c : s)
	{
		if (c != '.' && c != 'B' && c != 'W')
			return false;
	}
	return true;
}

Color parseColor(const std::string& token, const std::string& ctx)
{
	if (token == "B" || token == "b" || token == "Black" || token == "black")
		return Color::Black;
	if (token == "W" || token == "w" || token == "White" || token == "white")
		return Color::White;
	throw std::runtime_error(ctx + ": unknown color '" + token + "'");
}

void flushPosition(std::vector<BenchPosition>& out, BenchPosition& cur, bool& open)
{
	if (!open)
		return;
	if (cur.name.empty())
		throw std::runtime_error("position is missing a 'name' line");
	cur.board.setCurrentColor(cur.toMove);
	out.push_back(std::move(cur));
	cur = BenchPosition{};
	open = false;
}

std::vector<BenchPosition> loadPositions(const std::string& path)
{
	std::ifstream in(path);
	if (!in)
		throw std::runtime_error("cannot open positions file: " + path);

	std::vector<BenchPosition> out;
	BenchPosition cur;
	bool open = false;
	int asciiRow = 0;
	int lineNo = 0;
	std::string raw;

	auto begin = [&]() {
		if (open)
			flushPosition(out, cur, open);
		cur = BenchPosition{ "", Color::Black, 0, 0, GameBoard(kBoardSize, Color::Black) };
		open = true;
		asciiRow = 0;
	};

	while (std::getline(in, raw))
	{
		++lineNo;
		const std::string line = trim(raw);
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream iss(line);
		std::string tok;
		iss >> tok;

		if (tok == "name")
		{
			begin();
			std::string rest;
			std::getline(iss, rest);
			cur.name = trim(rest);
			if (cur.name.empty())
				throw std::runtime_error("line " + std::to_string(lineNo) + ": empty name");
			continue;
		}

		if (!open)
			throw std::runtime_error("line " + std::to_string(lineNo) + ": expected 'name' first");

		if (tok == "toMove")
		{
			std::string c;
			iss >> c;
			cur.toMove = parseColor(c, "line " + std::to_string(lineNo));
			continue;
		}
		if (tok == "captures")
		{
			if (!(iss >> cur.capturesBlack >> cur.capturesWhite))
				throw std::runtime_error("line " + std::to_string(lineNo) + ": captures <black> <white>");
			continue;
		}

		if (isAsciiRow(line))
		{
			if (asciiRow >= kBoardSize)
				throw std::runtime_error("line " + std::to_string(lineNo) + ": too many ASCII rows");
			for (int x = 0; x < static_cast<int>(line.size()); ++x)
			{
				const char c = line[static_cast<size_t>(x)];
				if (c == 'B' && !cur.board.placeStoneOfColor(x, asciiRow, CellStatus::Black))
					throw std::runtime_error("line " + std::to_string(lineNo) + ": cannot place B at " +
						std::to_string(x) + "," + std::to_string(asciiRow));
				if (c == 'W' && !cur.board.placeStoneOfColor(x, asciiRow, CellStatus::White))
					throw std::runtime_error("line " + std::to_string(lineNo) + ": cannot place W at " +
						std::to_string(x) + "," + std::to_string(asciiRow));
			}
			++asciiRow;
			continue;
		}

		if (tok == "B" || tok == "W")
		{
			int x = -1, y = -1;
			if (!(iss >> x >> y))
				throw std::runtime_error("line " + std::to_string(lineNo) + ": expected '" + tok + " x y'");
			if (x < 0 || y < 0 || x >= kBoardSize || y >= kBoardSize)
				throw std::runtime_error("line " + std::to_string(lineNo) + ": stone out of board");
			const CellStatus st = (tok == "B") ? CellStatus::Black : CellStatus::White;
			if (!cur.board.placeStoneOfColor(x, y, st))
				throw std::runtime_error("line " + std::to_string(lineNo) + ": occupied or invalid cell");
			continue;
		}

		throw std::runtime_error("line " + std::to_string(lineNo) + ": cannot parse '" + line + "'");
	}

	flushPosition(out, cur, open);
	if (out.empty())
		throw std::runtime_error("no positions in " + path);
	return out;
}

// ---------------------------------------------------------------------------
// JSON
// ---------------------------------------------------------------------------

std::string jsonEscape(const std::string& s)
{
	std::string o;
	o.reserve(s.size());
	for (char c : s)
	{
		switch (c)
		{
			case '\\': o += "\\\\"; break;
			case '"':  o += "\\\""; break;
			case '\n': o += "\\n";  break;
			case '\t': o += "\\t";  break;
			default:   o += c;      break;
		}
	}
	return o;
}

void writeJson(const RunReport& report, const std::string& path)
{
	std::ostream* os = &std::cout;
	std::ofstream file;
	if (path == "/dev/null")
	{
		file.open("/dev/null");
		os = &file;
	}
	else if (path != "-")
	{
		file.open(path);
		if (!file)
			throw std::runtime_error("cannot write " + path);
		os = &file;
	}
	std::ostream& out = *os;

	out << "{\n  \"depth\": " << report.depth << ",\n  \"positions\": [\n";
	for (size_t i = 0; i < report.positions.size(); ++i)
	{
		const BenchResult& r = report.positions[i];
		const BenchStats& s = r.stats;
		out << "    {\n"
			<< "      \"name\": \"" << jsonEscape(r.name) << "\",\n"
			<< "      \"bestMove\": {\"x\": " << static_cast<int>(r.bestMove.x)
			<< ", \"y\": " << static_cast<int>(r.bestMove.y) << "},\n"
			<< "      \"bestScore\": " << r.bestScore << ",\n"
			<< "      \"timeUs\": " << r.timeUs << ",\n"
			<< "      \"stats\": {\n"
			<< "        \"nodesVisited\": " << s.nodesVisited << ",\n"
			<< "        \"nodesEvaluated\": " << s.nodesEvaluated << ",\n"
			<< "        \"nodesPruned\": " << s.nodesPruned << ",\n"
			<< "        \"maxDepthSeen\": " << s.maxDepthSeen << ",\n"
			<< "        \"ttHits\": " << s.ttHits << ",\n"
			<< "        \"ttCutoffs\": " << s.ttCutoffs << ",\n"
			<< "        \"ttStores\": " << s.ttStores << ",\n"
			<< "        \"ttOrderingHits\": " << s.ttOrderingHits << ",\n"
			<< "        \"ttRootHits\": " << s.ttRootHits << ",\n"
			<< "        \"ttRootOrderingHits\": " << s.ttRootOrderingHits << ",\n"
			<< "        \"ttRootExactSeeds\": " << s.ttRootExactSeeds << ",\n"
			<< "        \"forcedNodes\": " << s.forcedNodes << "\n"
			<< "      }\n"
			<< "    }" << (i + 1 < report.positions.size() ? "," : "") << "\n";
	}
	out << "  ]\n}\n";
}

class JsonParser
{
	public:
		explicit JsonParser(std::string text) : _s(std::move(text)) {}

		RunReport parseReport()
		{
			RunReport r;
			expect('{');
			bool first = true;
			while (true)
			{
				skip();
				if (peek() == '}')
				{
					get();
					break;
				}
				if (!first)
					expect(',');
				first = false;
				const std::string key = parseString();
				expect(':');
				if (key == "depth")
					r.depth = static_cast<int>(parseInt());
				else if (key == "positions")
					r.positions = parsePositions();
				else
					skipValue();
			}
			return r;
		}

	private:
		std::string _s;
		size_t      _i = 0;

		char peek() const { return _i < _s.size() ? _s[_i] : '\0'; }
		char get() { return _i < _s.size() ? _s[_i++] : '\0'; }

		void skip()
		{
			while (std::isspace(static_cast<unsigned char>(peek())))
				get();
		}

		void expect(char c)
		{
			skip();
			if (get() != c)
				throw std::runtime_error(std::string("JSON: expected '") + c + "'");
		}

		std::string parseString()
		{
			skip();
			expect('"');
			std::string o;
			while (true)
			{
				const char c = get();
				if (c == '\0')
					throw std::runtime_error("JSON: unterminated string");
				if (c == '"')
					break;
				if (c == '\\')
				{
					const char e = get();
					if (e == '"' || e == '\\' || e == '/') o += e;
					else if (e == 'n') o += '\n';
					else if (e == 't') o += '\t';
					else o += e;
				}
				else
					o += c;
			}
			return o;
		}

		int64_t parseInt()
		{
			skip();
			size_t start = _i;
			if (peek() == '-')
				get();
			if (!std::isdigit(static_cast<unsigned char>(peek())))
				throw std::runtime_error("JSON: expected integer");
			while (std::isdigit(static_cast<unsigned char>(peek())))
				get();
			return std::strtoll(_s.c_str() + start, nullptr, 10);
		}

		void skipValue()
		{
			skip();
			const char c = peek();
			if (c == '"') { parseString(); return; }
			if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) { parseInt(); return; }
			if (c == '{')
			{
				get();
				bool first = true;
				while (true)
				{
					skip();
					if (peek() == '}') { get(); return; }
					if (!first) expect(',');
					first = false;
					parseString();
					expect(':');
					skipValue();
				}
			}
			if (c == '[')
			{
				get();
				bool first = true;
				while (true)
				{
					skip();
					if (peek() == ']') { get(); return; }
					if (!first) expect(',');
					first = false;
					skipValue();
				}
			}
			throw std::runtime_error("JSON: unexpected value");
		}

		t_cell parseMove()
		{
			t_cell m{-1, -1};
			expect('{');
			bool first = true;
			while (true)
			{
				skip();
				if (peek() == '}') { get(); break; }
				if (!first) expect(',');
				first = false;
				const std::string key = parseString();
				expect(':');
				if (key == "x") m.x = static_cast<int_fast16_t>(parseInt());
				else if (key == "y") m.y = static_cast<int_fast16_t>(parseInt());
				else skipValue();
			}
			return m;
		}

		BenchStats parseStats()
		{
			BenchStats s;
			expect('{');
			bool first = true;
			while (true)
			{
				skip();
				if (peek() == '}') { get(); break; }
				if (!first) expect(',');
				first = false;
				const std::string key = parseString();
				expect(':');
				const int v = static_cast<int>(parseInt());
				if (key == "nodesVisited") s.nodesVisited = v;
				else if (key == "nodesEvaluated") s.nodesEvaluated = v;
				else if (key == "nodesPruned") s.nodesPruned = v;
				else if (key == "maxDepthSeen") s.maxDepthSeen = v;
				else if (key == "ttHits") s.ttHits = v;
				else if (key == "ttCutoffs") s.ttCutoffs = v;
				else if (key == "ttStores") s.ttStores = v;
				else if (key == "ttOrderingHits") s.ttOrderingHits = v;
				else if (key == "ttRootHits") s.ttRootHits = v;
				else if (key == "ttRootOrderingHits") s.ttRootOrderingHits = v;
				else if (key == "ttRootExactSeeds") s.ttRootExactSeeds = v;
				else if (key == "forcedNodes") s.forcedNodes = v;
			}
			return s;
		}

		BenchResult parseResult()
		{
			BenchResult r;
			expect('{');
			bool first = true;
			while (true)
			{
				skip();
				if (peek() == '}') { get(); break; }
				if (!first) expect(',');
				first = false;
				const std::string key = parseString();
				expect(':');
				if (key == "name") r.name = parseString();
				else if (key == "bestMove") r.bestMove = parseMove();
				else if (key == "bestScore") r.bestScore = static_cast<int>(parseInt());
				else if (key == "timeUs") r.timeUs = parseInt();
				else if (key == "stats") r.stats = parseStats();
				else skipValue();
			}
			return r;
		}

		std::vector<BenchResult> parsePositions()
		{
			std::vector<BenchResult> out;
			expect('[');
			bool first = true;
			while (true)
			{
				skip();
				if (peek() == ']') { get(); break; }
				if (!first) expect(',');
				first = false;
				out.push_back(parseResult());
			}
			return out;
		}
};

RunReport loadReport(const std::string& path)
{
	std::ifstream in(path);
	if (!in)
		throw std::runtime_error("cannot open " + path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return JsonParser(ss.str()).parseReport();
}

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

BenchStats fromSearch(const SearchStats& s)
{
	BenchStats o;
	o.nodesVisited       = s.nodesVisited;
	o.nodesEvaluated     = s.nodesEvaluated;
	o.nodesPruned        = s.nodesPruned;
	o.maxDepthSeen       = s.maxDepthSeen;
	o.ttHits             = s.ttHits;
	o.ttCutoffs          = s.ttCutoffs;
	o.ttStores           = s.ttStores;
	o.ttOrderingHits     = s.ttOrderingHits;
	o.ttRootHits         = s.ttRootHits;
	o.ttRootOrderingHits = s.ttRootOrderingHits;
	o.ttRootExactSeeds   = s.ttRootExactSeeds;
	o.forcedNodes        = s.forcedNodes;
	return o;
}

RunReport runBench(int depth, std::vector<BenchPosition> positions)
{
	MasterAI19 ai(depth, ACTIVE_ZONE_RADIUS, Color::Black);
	RunReport report;
	report.depth = depth;
	report.positions.reserve(positions.size());

	using Clock = std::chrono::steady_clock;

	for (size_t i = 0; i < positions.size(); ++i)
	{
		BenchPosition& p = positions[i];
		ai.clearTranspositionTable();
		ai.setAIColor(p.toMove);
		ai.setStonesCapturedByAI(p.toMove == Color::Black ? p.capturesBlack : p.capturesWhite);
		ai.setStonesCapturedByOPP(p.toMove == Color::Black ? p.capturesWhite : p.capturesBlack);

		SearchPosition19 pos = SearchPosition19::fromBoard(p.board, p.capturesBlack, p.capturesWhite);

		const auto t0 = Clock::now();
		const t_cell move = ai.findBestMove(pos, p.toMove);
		const auto t1 = Clock::now();
		const int64_t us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

		const SearchStats& st = ai.lastSearchStats();
		BenchResult r;
		r.name      = p.name;
		r.bestMove  = move;
		r.bestScore = st.bestScore;
		r.timeUs    = us;
		r.stats     = fromSearch(st);
		report.positions.push_back(r);

		const double ms = static_cast<double>(us) / 1000.0;
		std::cerr << "[" << std::setw(3) << (i + 1) << "/" << std::setw(3) << positions.size()
			<< "] " << std::left << std::setw(22) << p.name << std::right
			<< "  move=(" << static_cast<int>(move.x) << "," << static_cast<int>(move.y) << ")"
			<< "  score=" << st.bestScore
			<< "  " << std::fixed << std::setprecision(1) << ms << " ms"
			<< "  nodes=" << st.nodesVisited
			<< "\n";
	}
	return report;
}

// ---------------------------------------------------------------------------
// Compare
// ---------------------------------------------------------------------------

std::string groupThousands(int64_t n)
{
	const bool neg = n < 0;
	if (neg) n = -n;
	std::string digits = std::to_string(n);
	std::string out;
	int k = 0;
	for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i)
	{
		if (k && k % 3 == 0)
			out.insert(out.begin(), ' ');
		out.insert(out.begin(), digits[static_cast<size_t>(i)]);
		++k;
	}
	if (neg)
		out.insert(out.begin(), '-');
	return out;
}

std::string pctStr(double oldV, double newV)
{
	if (oldV == 0.0)
		return newV == 0.0 ? "(identique)" : "(n/a)";
	const double pct = (newV - oldV) / oldV * 100.0;
	std::ostringstream ss;
	ss << std::showpos << std::fixed << std::setprecision(1) << pct << " %";
	return ss.str();
}

int compareReports(const RunReport& ref, const RunReport& neu)
{
	const size_t n = std::min(ref.positions.size(), neu.positions.size());
	int exact = 0;
	int64_t nodesRef = 0, nodesNew = 0;
	int64_t timeRef = 0, timeNew = 0;

	std::vector<std::string> mismatches;
	for (size_t i = 0; i < n; ++i)
	{
		const BenchResult& a = ref.positions[i];
		const BenchResult& b = neu.positions[i];
		nodesRef += a.stats.nodesVisited;
		nodesNew += b.stats.nodesVisited;
		timeRef  += a.timeUs;
		timeNew  += b.timeUs;

		const bool sameMove  = a.bestMove.x == b.bestMove.x && a.bestMove.y == b.bestMove.y;
		const bool sameScore = a.bestScore == b.bestScore;
		if (sameMove && sameScore)
			++exact;
		else
		{
			std::ostringstream ss;
			ss << "  - " << a.name
				<< "  ref=(" << static_cast<int>(a.bestMove.x) << "," << static_cast<int>(a.bestMove.y)
				<< ") score=" << a.bestScore
				<< "  new=(" << static_cast<int>(b.bestMove.x) << "," << static_cast<int>(b.bestMove.y)
				<< ") score=" << b.bestScore;
			mismatches.push_back(ss.str());
		}
	}

	const bool nodesOk = nodesRef == nodesNew;
	const double secRef = static_cast<double>(timeRef) / 1e6;
	const double secNew = static_cast<double>(timeNew) / 1e6;
	const double npsRef = secRef > 0.0 ? static_cast<double>(nodesRef) / secRef : 0.0;
	const double npsNew = secNew > 0.0 ? static_cast<double>(nodesNew) / secNew : 0.0;

	auto mark = [](bool ok) { return ok ? "✓" : "✗"; };

	std::cout << "Exactitude   : " << exact << "/" << n
		<< " identiques (bestMove + bestScore)     " << mark(exact == static_cast<int>(n) && ref.positions.size() == neu.positions.size())
		<< "\n";
	std::cout << "Nœuds        : " << groupThousands(nodesRef) << " → " << groupThousands(nodesNew)
		<< "   " << (nodesOk ? "(identique)" : ("(" + pctStr(static_cast<double>(nodesRef), static_cast<double>(nodesNew)) + ")"))
		<< "           " << mark(nodesOk) << "\n";
	std::cout << "Temps        : " << std::fixed << std::setprecision(2)
		<< secRef << " s → " << secNew << " s        (" << pctStr(secRef, secNew) << ")\n";
	std::cout << "Nœuds/s      : " << groupThousands(static_cast<int64_t>(npsRef / 1000.0)) << " k → "
		<< groupThousands(static_cast<int64_t>(npsNew / 1000.0)) << " k           ("
		<< pctStr(npsRef, npsNew) << ")\n";

	if (ref.positions.size() != neu.positions.size())
	{
		std::cout << "\nTaille des suites : " << ref.positions.size()
			<< " → " << neu.positions.size() << " (différent)\n";
	}
	if (!mismatches.empty())
	{
		std::cout << "\nPositions divergentes :\n";
		for (const auto& line : mismatches)
			std::cout << line << "\n";
	}

	const bool ok = exact == static_cast<int>(n)
		&& ref.positions.size() == neu.positions.size()
		&& nodesOk;
	return ok ? 0 : 1;
}

} // namespace

int main(int argc, char** argv)
{
	Logger::setEnabled(false);

	try
	{
		if (argc < 2)
		{
			usage(argv[0]);
			return 2;
		}

		if (flagEq(argv[1], "--compare"))
		{
			if (argc != 4)
			{
				usage(argv[0]);
				return 2;
			}
			const RunReport ref = loadReport(argv[2]);
			const RunReport neu = loadReport(argv[3]);
			return compareReports(ref, neu);
		}

		int depth = 0;
		std::string positionsPath;
		std::string outPath;
		for (int i = 1; i < argc; ++i)
		{
			if (flagEq(argv[i], "--depth") && i + 1 < argc)
				depth = std::atoi(argv[++i]);
			else if (flagEq(argv[i], "--positions") && i + 1 < argc)
				positionsPath = argv[++i];
			else if (flagEq(argv[i], "--out") && i + 1 < argc)
				outPath = argv[++i];
			else if (flagEq(argv[i], "--help") || flagEq(argv[i], "-h"))
			{
				usage(argv[0]);
				return 0;
			}
			else
			{
				std::cerr << "Unknown argument: " << argv[i] << "\n";
				usage(argv[0]);
				return 2;
			}
		}

		if (depth <= 0 || positionsPath.empty() || outPath.empty())
		{
			usage(argv[0]);
			return 2;
		}

		auto positions = loadPositions(positionsPath);
		const RunReport report = runBench(depth, std::move(positions));
		writeJson(report, outPath);
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "bench: " << e.what() << "\n";
		return 1;
	}
}
