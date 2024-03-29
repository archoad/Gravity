# définition des cibles particulières
.PHONY: clean, mrproper

# désactivation des règles implicites
.SUFFIXES:

UNAME_S:=$(shell uname -s)

CC=gcc
CL=clang
STRIP=strip
CFLAGS= -O3 -Wall -W -Wstrict-prototypes -Werror -Wextra -Wuninitialized
ifeq ($(UNAME_S),Linux)
	IFLAGSDIR= -I/usr/include
	LFLAGSDIR= -L/usr/lib
	COMPIL=$(CC)
endif
ifeq ($(UNAME_S),Darwin)
	IFLAGSDIR= -I/opt/local/include
	LFLAGSDIR= -L/opt/local/lib
	COMPIL=$(CL)
endif
GL_FLAGS= -lGL -lGLU -lglut
MATH_FLAGS= -lm
PNG_FLAGS= -lpng
GMP_FLAGS= -lgmp

all: dest_sys gravity3d universe3d boids3d

boids3d: boids3d.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(MATH_FLAGS) $(GL_FLAGS) $(PNG_FLAGS) $< -o $@
	@$(STRIP) $@

gravity3d: gravity3d.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(MATH_FLAGS) $(GL_FLAGS) $(PNG_FLAGS) $< -o $@
	@$(STRIP) $@

universe3d: universe3d.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(MATH_FLAGS) $(GL_FLAGS) $(PNG_FLAGS) $< -o $@
	@$(STRIP) $@


dest_sys:
	@echo "Destination system:" $(UNAME_S)

clean:
	@rm -f gravity3d
	@rm -f universe3d
	@rm -f boids3d
	@rm -f *.o
