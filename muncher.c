/*
 * Muncher - play your files bit by bit
 * Copyright (C) 2015 Raphael Sousa Santos, http://www.raphaelss.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

struct mmap_data {
	unsigned char *data;
	size_t size;
	int fd;
};

static int
mmap_data_init(struct mmap_data *md, const char *path) {
	md->fd = open(path, O_RDONLY);
	if (md->fd == -1) {
		fprintf(stderr, "Error opening %s\n", path);
		return 1;
	}
	struct stat s;
	fstat(md->fd, &s);
	md->data = mmap(0, s.st_size, PROT_READ, MAP_SHARED, md->fd, 0);
	if (md->data == MAP_FAILED) {
		close(md->fd);
		fputs("Error on mmap\n", stderr);
		return 1;
	}
	md->size = s.st_size;
	return 0;
}

static void
mmap_data_release(struct mmap_data *md) {
	munmap(md->data, md->size);
	close(md->fd);
}

int
main(int argc, char **argv) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
		return 1;
	}
	struct mmap_data md;
	if (mmap_data_init(&md, argv[1])) {
		SDL_Quit();
		return 1;
	}
	unsigned width = 640, height = 480, fullscreen = 1;
	SDL_Window *window;
	if (fullscreen) {
		SDL_DisplayMode dm;
		SDL_GetCurrentDisplayMode(0, &dm);
		window = SDL_CreateWindow("Bit muncher",
		    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		    dm.w, dm.h, SDL_WINDOW_FULLSCREEN);
	} else {
		window = SDL_CreateWindow("Bit muncher",
		    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		    width, height, SDL_WINDOW_SHOWN);
	}
	if (!window) {
		fputs("Error initializing window\n", stderr);
		mmap_data_release(&md);
		SDL_Quit();
		return 1;
	}
	size_t byte_i = 0, bit_i = 0;
	SDL_Surface *window_surface = SDL_GetWindowSurface(window);
	unsigned total_pixels = window_surface->w * window_surface->h * 4;
	SDL_LockSurface(window_surface);
	SDL_UnlockSurface(window_surface);
	int run = 1;
	SDL_Event event;
	while (run) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				run = 0;
				break;
			default:
				break;
			}
		}
		SDL_LockSurface(window_surface);
		unsigned char *pixels = window_surface->pixels;
		for (size_t i = 0; i < total_pixels; i += 4) {
			memset(pixels + i, ((md.data[byte_i] & (1 << bit_i++)) != 0) * 0xFF, 4);
			bit_i %= 8;
			byte_i = (byte_i + !bit_i) % md.size;
		}
		SDL_UnlockSurface(window_surface);
		SDL_UpdateWindowSurface(window);
	}
	mmap_data_release(&md);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
