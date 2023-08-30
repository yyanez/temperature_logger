all: release

clean:
	rm -f bin/release/temperature_logger bin/debug/temperature_logger
    
release:
	mkdir -p bin/release/
	g++ -o bin/release/temperature_logger src/main_temperature_logger.cpp -lwiringPi -lwiringPiDev -O3
 
debug:
	mkdir -p bin/debug/
	g++ -o bin/debug/temperature_logger src/main_temperature_logger.cpp -DDEBUG -lwiringPi -lwiringPiDev -O0 
 
