build:
	g++ -std=c++20 vegastrike_animation.cpp -o vs_spredit `pkg-config gtkmm-3.0 cairomm-1.0 --cflags --libs`
test:
	g++ -std=c++20 test.cpp -o test `pkg-config gtkmm-3.0 cairomm-1.0 --cflags --libs`
clean:
	rm vs_spredit
