all:
	g++ -o "det" -ggdb `pkg-config --cflags opencv` detect_extract_match.cpp `pkg-config --libs opencv`
