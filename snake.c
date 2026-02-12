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

#define STARTING_LENGTH 10

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

    uint32_t pending_segments;
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

    if (body->head) {
        body->head->prev = head;
    }

    body->head = head;
    if (!body->tail) {
        body->tail = head;
    }
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

static void random_point(struct uvec2* pos) {
    pos->x = rand() % SCREEN_WIDTH;
    pos->y = rand() % SCREEN_HEIGHT;
}

static void init_snake(struct snake* snake) {
    struct uvec2 start;

    memset(snake, 0, sizeof(struct snake));

    start.x = SCREEN_WIDTH / 2;
    start.y = SCREEN_HEIGHT / 2;

    add_head(&snake->body, &start);
    snake->body.pending_segments = STARTING_LENGTH - 1;

    random_point(&snake->apple_pos);

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
    TEXT = 4,
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
    init_pair(TEXT, COLOR_WHITE, COLOR_BLACK);
}

static void shutdown_curses() { endwin(); }

static void turn(struct snake* snake, int32_t xdir, int32_t ydir) {
    if (xdir == -snake->direction.x || ydir == -snake->direction.y) {
        return; /* cant turn around into itself */
    }

    snake->direction.x = xdir;
    snake->direction.y = ydir;
}

static void process_key(struct snake* snake, int ch) {
    switch (ch) {
    case 'w':
    case KEY_UP:
        turn(snake, 0, -1);
        break;
    case 's':
    case KEY_DOWN:
        turn(snake, 0, 1);
        break;
    case 'a':
    case KEY_LEFT:
        turn(snake, -1, 0);
        break;
    case 'd':
    case KEY_RIGHT:
        turn(snake, 1, 0);
        break;
    case 'q':
        snake->game_over = 1;
        break;
    }
}

static void process_input(struct snake* snake) {
    int ch;
    while ((ch = getch()) != ERR) {
        process_key(snake, ch);
    }
}

static void get_next_head_pos(const struct uvec2* prev_pos, const struct ivec2* dir,
                              struct uvec2* next_pos) {
    if (dir->x < 0 && prev_pos->x < -dir->x) {
        next_pos->x = SCREEN_WIDTH + dir->x;
    } else {
        next_pos->x = prev_pos->x + dir->x;
        next_pos->x %= SCREEN_WIDTH;
    }

    if (dir->y < 0 && prev_pos->y < -dir->y) {
        next_pos->y = SCREEN_HEIGHT + dir->y;
    } else {
        next_pos->y = prev_pos->y + dir->y;
        next_pos->y %= SCREEN_HEIGHT;
    }
}

static int check_self_collision(const struct body* body, const struct uvec2* next_pos) {
    struct segment* segment;

    for (segment = body->head; segment != NULL; segment = segment->next) {
        if (next_pos->x == segment->pos.x && next_pos->y == segment->pos.y) {
            return 1;
        }
    }

    return 0;
}

static void tick(struct snake* snake) {
    struct segment* prev_head = snake->body.head;
    struct uvec2 next_pos;

    get_next_head_pos(&prev_head->pos, &snake->direction, &next_pos);
    if (check_self_collision(&snake->body, &next_pos)) {
        snake->game_over = 1;
        return;
    }

    if (next_pos.x == snake->apple_pos.x && next_pos.y == snake->apple_pos.y) {
        snake->score++;
        snake->body.pending_segments++;

        random_point(&snake->apple_pos);
    }

    add_head(&snake->body, &next_pos);
    if (snake->body.pending_segments > 0) {
        snake->body.pending_segments--; /* still more segments to "uncoil" */
    } else {
        remove_tail(&snake->body);
    }
}

static void render_at(uint32_t x, uint32_t y, int color) {
    color_set(color, NULL);
    mvaddstr(y, x * 2, "  ");
}

static void render(struct snake* snake) {
    struct segment* segment;
    clear();

    /* apple */
    render_at(snake->apple_pos.x, snake->apple_pos.y, APPLE);

    /* body */
    uint32_t length = 0;
    for (segment = snake->body.head; segment != NULL; segment = segment->next) {
        int color = segment == snake->body.head ? SNAKE_HEAD : SNAKE_BODY;
        render_at(segment->pos.x, segment->pos.y, color);

        length++;
    }

    color_set(TEXT, NULL);
    mvprintw(SCREEN_HEIGHT, 0, "Length: %u\n", length);
    mvprintw(SCREEN_HEIGHT + 1, 0, "Pending: %u\n", snake->body.pending_segments);

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
    while (1) {
        process_input(&snake);
        tick(&snake);
        render(&snake);

        if (snake.game_over) {
            break;
        }

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
        time_diff(&t0, &t1, &delta);
        memcpy(&t0, &t1, sizeof(struct timespec));

        if (delta.tv_sec < 1 && delta.tv_nsec < delay_ns) {
            uint64_t to_sleep_ns = delay_ns - delta.tv_nsec;
            usleep(to_sleep_ns / 1e3); /* in microseconds */
        }
    }

    shutdown_curses();

    printf("Game over! Score: %u\n", snake.score);

    destroy_snake(&snake);
    return 0;
}
