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
		{190, 190, 0},
		{0, 0, 0},
		{119, 0, 130},
};

static const u8 colorcount = sizeof(colors) / sizeof(colors[0]);

class graph_t: public Fl_Widget {
public:
	graph_t(int x, int y, int w, int h): Fl_Widget(x, y, w, h, "") {
		swaps = NULL;
		frags = NULL;
		maxall = 0;
	}

	virtual ~graph_t() {
		delete [] frags;
		delete [] swaps;
		delete [] maxes;
		delete [] avgs;
		delete [] labels;
	}

	void draw() {
		// The scale
		fl_color(FL_BLACK);
		fl_line_style(FL_SOLID | FL_CAP_ROUND, 2);
		const u32 fontsize = 14;
		fl_font(FL_HELVETICA, fontsize);

		const u32 border = 10;

		u32 boxx, boxy, boxx2, boxy2;
		boxx = boxy = border;
		boxx2 = w() - border;
		boxy2 = h() - border * 10;

		fl_begin_loop();
		fl_vertex(boxx, boxy);
		fl_vertex(boxx, boxy2);
		fl_vertex(boxx2, boxy2);
		fl_vertex(boxx2, boxy);
		fl_end_loop();

		u32 tmp = fl_width("Memory operations");
		fl_draw("Memory operations", (w() - tmp) / 2, boxy2 + 4 * border);

		fl_draw("Holes (fragments)", 2 * border, 2 * border + fontsize);
		fl_draw("The bars below the dashed line signify swapping. "
			"Parenthesis: (avg/max/swap%)",
			2 * border, boxy2 + border * 4 + fontsize + 5);

		const u32 swaph = 8;

		const u32 areax = boxx + border;
		const u32 areay = boxy + border / 2;
		const u32 areax2 = boxx2 - border;
		const u32 areay2 = boxy2 - border - swaph * pairs;
		const u32 areaw = areax2 - areax;
		const u32 areah = areay2 - areay;

		u32 i;
		for (i = 0; i < pairs; i++) {
			fl_color(colors[i][0], colors[i][1], colors[i][2]);
			u32 x;
			x = (w() / pairs) * i + 2 * border;

			fl_rectf(x, h() - border * 3, 2 * border, 2 * border);

			fl_color(FL_BLACK);

			char buf[160];
			snprintf(buf, 160, "%s (%.2f/%u/%g%%)", labels[i], avgs[i],
				maxes[i], swapavg[i]);

			fl_draw(buf, x + 3 * border, h() - border - fl_descent() - 2);
		}

		fl_color(FL_BLACK);
		fl_line_style(FL_SOLID | FL_CAP_ROUND, 2);

		// 3 X Markers
		const float xmax = frags[0].size();
		const float xskip = xmax / 4.0f;
		for (i = 1; i < 4; i++) {
			const float xper = (xskip * i) / xmax;
			const float flx = xper * areaw + areax;

			fl_line(flx, boxy2 - border / 2,
				flx, boxy2 + border / 2);

			char buf[80];
			u64 amount = xskip * i * 10;
			char suffix[3] = "\0\0";

			if (amount > 1000000) {
				amount /= 1000000;
				suffix[0] = ' ';
				suffix[1] = 'M';
			} else if (amount > 1000) {
				amount /= 1000;
				suffix[0] = ' ';
				suffix[1] = 'k';
			}

			sprintf(buf, "%lu%s", amount, suffix);
			tmp = fl_width(buf) / 2;
			fl_draw(buf, flx - tmp, boxy2 + border + fontsize);
		}

		// 3 Y Markers
		const float ymax = maxall;
		const float yskip = maxall / 4.0f;
		for (i = 1; i < 4; i++) {
			const float yper = 1.0f - ((yskip * i) / ymax);
			const float fly = yper * areah + areay;

			fl_line(boxx - border / 2, fly,
				boxx + border / 2, fly);

			char buf[80];
			sprintf(buf, "%u", (u32) (yskip * i));
			tmp = fl_descent();
			fl_draw(buf, boxx + border * 1.5f,
				fly + fontsize / 2 - fl_descent());
		}

		// The data
		for (i = 0; i < pairs; i++) {
			fl_color(colors[i][0], colors[i][1], colors[i][2]);
			fl_line_style(FL_SOLID | FL_CAP_ROUND, 0);

			u32 j;
			const u32 max = frags[i].size();
			fl_begin_line();
			for (j = 0; j < max; j++) {
				const float flx = (j / (float) (max - 1)) * areaw;
				const float fly = 1.0f - (frags[i][j] / (float) maxall);

				fl_vertex(flx + areax,
					fly * areah + areay);
			}
			fl_end_line();

			fl_line_style(FL_SOLID, 0);
			u32 swaplen = areaw / max;
			if (!swaplen) swaplen = 1;
			for (j = 0; j < max; j++) {
				if (!swaps[i][j]) continue;
				const float flx = (j / (float) (max - 1)) * areaw + areax;

				const float fly = boxy2 - (pairs - i) * swaph - border;

				fl_line(flx, fly,
					flx, fly + swaph);
			}
		}

		fl_color(FL_BLACK);
		char dashes[] = {4, 4, 0};
		fl_line_style(FL_DASH | FL_CAP_ROUND, 0, dashes);
		fl_line(boxx, areay2, boxx2, areay2);

		fl_line_style(0);
	}

	void reserve(const u32 pairs) {
		swaps = new vector<bool>[pairs];
		frags = new vector<u16>[pairs];
		maxes = new u16[pairs]();
		avgs = new float[pairs]();
		labels = new const char*[pairs];
		swapavg = new float[pairs]();

		this->pairs = pairs;

		u32 i;
		for (i = 0; i < pairs; i++) {
			swaps[i].reserve(10000000);
			frags[i].reserve(10000000);
		}
	}

	void add(const u32 idx, const u16 frag, const u8 swap) {
		frags[idx].push_back(frag);
		swaps[idx].push_back(swap);

		if (maxes[idx] < frag)
			maxes[idx] = frag;
		if (maxall < frag)
			maxall = frag;

		u32 size = frags[idx].size();

		avgs[idx] = (avgs[idx] * (size - 1) + frag) / size;
	}

	void calculate() {
		u32 i, j;
		for (i = 0; i < pairs; i++) {
			const u32 max = swaps[0].size();
			u32 on = 0;
			for (j = 0; j < max; j++) {
				if (swaps[i][j])
					on++;
			}
			swapavg[i] = 100.0f * ((float) on) / max;
		}
	}

	void setLabel(const u32 idx, const char * const label) {
		labels[idx] = label;
	}

private:
	vector<u16> *frags;
	vector<bool> *swaps;
	u8 pairs;
	u16 *maxes;
	float *avgs;
	const char **labels;
	u16 maxall;
	float *swapavg;
};

int main(int argc, char **argv) {

	if (argc < 2 || argc % 2 == 0) {
		die("Usage: %s [label file.gz] [label2 file2.gz]...\n", argv[0]);
	}

	const u32 pairs = argc / 2;
	if (pairs > colorcount) die("We can only handle %u pairs\n", colorcount);

	Fl_Window *win = new Fl_Double_Window(1920, 950);
	win->color(FL_WHITE);
	graph_t *graph = new graph_t(0, 0, win->w(), win->h());
	graph->color(FL_WHITE);
	graph->reserve(pairs);
	win->end();

	u32 i;
	for (i = 1; i < (u32) argc; i += 2) {
		const u32 set = i / 2;

		graph->setLabel(set, argv[i]);
		void * const f = gzopen(argv[i + 1], "rb");
		if (!f) die("Failed opening %s\n", argv[i + 1]);

		printf("Reading data from %s... ", argv[i + 1]);
		fflush(stdout);

		char buf[160];
		while (gzgets(f, buf, 160)) {
			if (buf[0] == '-')
				continue;

			u16 frag;
			u8 swap;

			frag = strtoul(buf, NULL, 10);
			char *space = strchr(buf + 2, ' ') + 1;
			swap = *space == '1';

			graph->add(set, frag, swap);
		}

		puts("Done.");

		gzclose(f);
	}

	graph->calculate();

	win->show();

	return Fl::run();
}
