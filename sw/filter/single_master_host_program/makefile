CC = g++
CFLAGS = -g -std=c++11 -Wall -I/usr/local/include/opencv4/ \
         -I/home/aignacio/projects/matplotlib-cpp \
         -I/usr/include/python3.10 \
         -I/home/aignacio/projects/cv-plot
LDFLAGS = `pkg-config --libs opencv4` -lpython3.10
SOURCES = filter_app.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = filter_app

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
