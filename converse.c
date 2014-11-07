#include <stdio.h>
#include <string.h>
#include <cdk.h>

int main (int argc, char *argv[]) {
	CDKSCREEN    *screen;
	WINDOW       *window;
	CDKSCROLL    *scrollList;
	char **item = 0;
	int count, i;
	char buffer[65536];

	window = initscr();
	screen = initCDKScreen(window);

	scrollList = newCDKScroll(screen, 0, 0, FULL, FULL, NONE, "", item, count, false, A_REVERSE, true, true);

	//refreshCDKScreen (screen);

//fgets(buffer, sizeof buffer, stdin);
	for (i = 0; i < 100; i++) {
		addCDKScrollItem(scrollList, "hi");
		addCDKScrollItem(scrollList, "h0");
		addCDKScrollItem(scrollList, "ho");
	}

	activateCDKScroll(scrollList, 0);

	destroyCDKSwindow(scrollList);
	destroyCDKScreen(screen);
	endCDK();

	return 0;
}
