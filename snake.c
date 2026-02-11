#include <curses.h>

#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <malloc.h>

#include <unistd.h>

#define SCREEN_WIDTH 32
#define SCREEN_HEIGHT 32

struct ivec2 {
    int32_t x, y;
};

struct uvec2 {
    uint32_t x, y;
};

struct segment {
    struct uvec2 pos;

    struct segment* next;
    struct segment* prev;
};

struct body {
    struct segment* head;
    struct segment* tail;
};

struct snake {
    struct body body;
    struct ivec2 direction;

    struct uvec2 apple_pos;
    uint32_t score;

    int game_over;
};

static void add_head(struct body* body, const struct uvec2* pos) {
    struct segment* head = malloc(sizeof(struct segment));
    memcpy(&head->pos, pos, sizeof(struct uvec2));

    head->next = body->head;
    head->prev = NULL;

    body->head = head;
}

static void remove_tail(struct body* body) {
    struct segment* prev;

    if (!body->tail) {
        return;
    }

    prev = body->tail->prev;
    if (prev) {
        prev->next = NULL;
    }

    free(body->tail);
    body->tail = prev;
}

static void init_snake(struct snake* snake) {
    struct uvec2 start;

    memset(snake, 0, sizeof(struct snake));

    start.x = SCREEN_WIDTH / 2;
    start.y = SCREEN_HEIGHT / 2;
    add_head(&snake->body, &start);

    snake->apple_pos.x = rand() % SCREEN_WIDTH;
    snake->apple_pos.y = rand() % SCREEN_HEIGHT;

    snake->direction.x = 1;
    snake->direction.y = 0;
}

static void destroy_snake(struct snake* snake) {
    struct segment* segment;

    /* destroy body first */
    segment = snake->body.head;
    while (segment) {
        struct segment* next = segment->next;

        free(segment);
        segment = next;
    }
}

enum {
    SNAKE_BODY = 1,
    SNAKE_HEAD = 2,
    APPLE = 3,
};

static void init_curses() {
    initscr();
    cbreak();
    noecho();

    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    start_color();

    init_pair(SNAKE_BODY, COLOR_WHITE, COLOR_GREEN);
    init_pair(SNAKE_HEAD, COLOR_WHITE, COLOR_WHITE);
    init_pair(APPLE, COLOR_WHITE, COLOR_RED);
}

static void shutdown_curses() { endwin(); }

static void process_key(int ch) {}

static void process_input() {
    int ch;
    while ((ch = getch()) != ERR) {
        process_key(ch);
    }
}

static void tick() { /* todo: tick */ }

static void render() {
    /* todo: render */

    refresh();
}

static void time_diff(const struct timespec* t0, const struct timespec* t1,
                      struct timespec* delta) {
    delta->tv_sec = t1->tv_sec - t0->tv_sec;
    delta->tv_nsec = t1->tv_nsec - t0->tv_nsec;

    if (delta->tv_nsec < 0) {
        delta->tv_sec--;
        delta->tv_nsec += 1e9;
    }
}

int main(int argc, const char** argv) {
    struct timespec t0, t1, delta;
    struct snake snake;

    uint32_t delay_ms = 100;
    uint64_t delay_ns = (uint64_t)delay_ms * 1e6;

    srand(time(NULL));

    init_snake(&snake);
    init_curses();

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0);
    while (!snake.game_over) {
        tick();
        render();

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
        time_diff(&t0, &t1, &delta);
        memcpy(&t0, &t1, sizeof(struct timespec));

        if (delta.tv_sec < 1 && delta.tv_nsec < delay_ns) {
            uint64_t to_sleep_ns = delay_ns - delta.tv_nsec;
            usleep(to_sleep_ns / 1e3); /* in microseconds */
        }
    }

    shutdown_curses();

    printf("Game over! Score: %u", snake.score);

    destroy_snake(&snake);
    return 0;
}
