LFLAGS= pthread

FLAGS=

FLAGS += $(patsubst %,-l%,$(LFLAGS))
FLAGS += -std=c++11

.PHONY : fresh clean default touchAll

default: mainServer client subServer

mainServer: SuperServer.cpp SuperServer.h
	@echo "${GREEN}Compilando $< ${NC}"
	@g++ SuperServer.cpp $(FLAGS) -o mainServer

client: Client.cpp
	@echo "${GREEN}Compilando $< ${NC}"
	@g++ Client.cpp $(FLAGS) -o client

subServer: SubServer.cpp SubServer.h
	@echo "${GREEN}Compilando $< ${NC}"
	@g++ SubServer.cpp $(FLAGS) -o subServer

clean:
	@rm -f subServer mainServer client





#  .---------- COLORS FOR ECHO---------. #
##########################################

#  Regular
BLACK = \033[0;30m#Black
RED = \033[0;31m#Red
GREEN = \033[0;32m#Green
YELLOW = \033[0;33m#Yellow
BLUE = \033[0;34m#Blue
PURPLE = \033[0;35m#Purple
CYAN = \033[0;36m#Cyan
WHITE = \033[0;37m#White


NC = \033[0m # o Color -> reset the color


#  Bold

B_BLACK = \033[1;30m
B_RED = \033[1;31m
B_GREEN = \033[1;32m
B_YELLOW = \033[1;33m
B_BLUE = \033[1;34m
B_PURPLE = \033[1;35m
B_CYAN = \033[1;36m
B_WHITE = \033[1;37m


#Light / high intesity

L_BLACK = \033[0;90m
L_RED = \033[0;91m
L_GREEN = \033[0;92m
L_YELLOW = \033[0;93m
L_BLUE = \033[0;94m
L_PURPLE = \033[0;95m
L_CYAN = \033[0;96m
L_WHITE = \033[0;97m

# Bold light/hight intesity

BL_BLACK = \033[1;90m
BL_RED = \033[1;91m
BL_GREEN = \033[1;92m
BL_YELLOW = \033[1;93m
BL_BLUE = \033[1;94m
BL_PURPLE = \033[1;95m
BL_CYAN = \033[1;96m
BL_WHITE = \033[1;97m