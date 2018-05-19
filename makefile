SOURCES := $(shell find . -name '*.cpp')
HEADERS := $(shell find . -name '*.h')
OUTPUT_FILE := thieves_guild

all: clean $(OUTPUT_FILE)

$(OUTPUT_FILE): $(SOURCES) $(HEADERS)
	mpic++ -Wall -o $(OUTPUT_FILE) $(SOURCES)

clean:
	$(RM) $(OUTPUT_FILE)
