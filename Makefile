
default, all, measure:
	g++ --std=c++17 -Wall Measure.cpp -O3 -o Measure

leakcheck:
	g++ --std=c++17 -Wall Measure.cpp -O3 -o Measure
	valgrind  --leak-check=full ./Measure

clean:
	rm Measure
