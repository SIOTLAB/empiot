CC=gcc 
CFLAGS=-Wall

make:
	g++ -o empiot empiot.cpp raw_thread.cpp energy_thread.cpp stop_conditions.cpp I2C.cpp INA219.cpp -lwiringPi -lbcm2835 -lpthread -lrt

clean:
	rm empiot
	echo Clean done
