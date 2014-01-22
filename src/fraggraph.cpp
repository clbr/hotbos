#include <stdio.h>
#include "lrtypes.h"
#include "helpers.h"
#include <vector>
#include <Fl/Fl.h>
#include <Fl/Fl_Widget.h>
#include <Fl/fl_draw.h>
#include <FL/Fl_Double_Window.h>

using namespace std;


static const u8 colors[][3] = {
		{255, 0, 0},
		{206, 107, 0},
		{0, 0, 0},
		{119, 0, 130},
};

static const u8 colorcount = sizeof(colors) / sizeof(colors[0]);

class graph_t: public Fl_Widget {
public:
	graph_t(int x, int y, int w, int h): Fl_Widget(x, y, w, h, "") {
		swaps = NULL;
		frags = NULL;
	}

	virtual ~graph_t() {}

	void draw() {
		// The scale
		fl_color(FL_BLACK);
		fl_line_style(FL_SOLID | FL_CAP_ROUND, 2);

		const u32 border = 10;

		u32 boxx, boxy, boxx2, boxy2;
		boxx = boxy = border;
		boxx2 = w() - border;
		boxy2 = h() - border * 6;

		fl_begin_loop();
		fl_vertex(boxx, boxy);
		fl_vertex(boxx, boxy2);
		fl_vertex(boxx2, boxy2);
		fl_vertex(boxx2, boxy);
		fl_end_loop();

		// The data
		u32 i;
		for (i = 0; i < pairs; i++) {
			fl_color(colors[i][0], colors[i][1], colors[i][2]);
			fl_line_style(FL_SOLID | FL_CAP_ROUND, 1);
		}
		fl_line_style(0);
	}

	void reserve(const u32 pairs) {
		swaps = new vector<bool>[pairs];
		frags = new vector<u16>[pairs];
		maxes = new u16[pairs];
		avgs = new u16[pairs];
		this->pairs = pairs;
	}

	void add(const u32 idx, const u16 frag, const u8 swap) {
		frags[idx].push_back(frag);
		swaps[idx].push_back(swap);

		if (maxes[idx] < frag)
			maxes[idx] = frag;

		u32 size = frags[idx].size();

		avgs[idx] = (avgs[idx] * (size - 1) + frag) / size;
	}

private:
	vector<u16> *frags;
	vector<bool> *swaps;
	u8 pairs;
	u16 *maxes;
	u16 *avgs;
};

int main(int argc, char **argv) {

	if (argc < 2 || argc % 2 == 0) {
		die("Usage: %s [label file.gz] [label2 file2.gz]...\n", argv[0]);
	}

	const u32 pairs = argc / 2;
	if (pairs > colorcount) die("We can only handle %u pairs\n", colorcount);

	Fl_Window *win = new Fl_Double_Window(1024, 512);
	win->color(FL_WHITE);
	graph_t *graph = new graph_t(0, 0, 1024, 512);
	graph->color(FL_WHITE);
	graph->reserve(pairs);
	win->end();

	u32 i;
	for (i = 1; i < (u32) argc; i += 2) {
		const u32 set = i / 2;

		//graph->setLabel(set, argv[i]);
		void * const f = gzopen(argv[i + 1], "rb");
		if (!f) die("Failed opening %s\n", argv[i + 1]);

		char buf[160];
		while (gzgets(f, buf, 160)) {
			if (buf[0] == '-')
				continue;

			u16 frag;
			u8 swap;
			if (sscanf(buf, "%hu, %hhu", &frag, &swap) != 2)
				die("Failed on line %s\n", buf);

			graph->add(set, frag, swap);
		}

		gzclose(f);
	}

	win->show();

	return Fl::run();
}
